#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <main.h>
#include <natives.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

namespace {
HMODULE moduleHandle{};
std::atomic<unsigned> events{0};
std::atomic<bool> forwardHeld{false};
std::atomic<bool> sprintHeld{false};

constexpr unsigned Toggle = 1;
constexpr unsigned Emergency = 2;

bool enabled = false;
bool slowWorld = true;
bool useCustomModel = false;
bool modelApplied = false;

float targetSpeed = 32.0f;
float acceleration = 24.0f;
float deceleration = 36.0f;
float worldTimeScale = 0.72f;
float currentBoost = 0.0f;

Hash originalModel = 0;
Hash customModel = 0;

std::ofstream logFile;
const char* status = "F6 enable Flash mode | Story Mode only";

void log(const std::string& text) {
    if (logFile) logFile << GetTickCount64() << " " << text << std::endl;
}

bool focused() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

bool online() {
    return NETWORK::NETWORK_IS_SESSION_ACTIVE()
        || NETWORK::NETWORK_IS_IN_SESSION()
        || NETWORK::NETWORK_IS_GAME_IN_PROGRESS();
}

bool eligible(Ped ped) {
    if (!focused() || online() || HUD::IS_PAUSE_MENU_ACTIVE() || !CAM::IS_SCREEN_FADED_IN()) return false;
    if (!ped || !ENTITY::DOES_ENTITY_EXIST(ped) || ped != PLAYER::PLAYER_PED_ID() || ENTITY::IS_ENTITY_DEAD(ped)) return false;
    if (!PLAYER::IS_PLAYER_CONTROL_ON(PLAYER::PLAYER_ID())) return false;
    if (PED::IS_PED_ON_MOUNT(ped) || PED::IS_PED_IN_ANY_VEHICLE(ped, false)
        || PED::IS_PED_SWIMMING(ped) || PED::IS_PED_RAGDOLL(ped)) return false;
    return true;
}

void display(const char* message) {
    HUD::SET_TEXT_SCALE(0.32f, 0.32f);
    HUD::_SET_TEXT_COLOR(255, 220, 80, 255);
    HUD::SET_TEXT_CENTRE(false);
    HUD::_DISPLAY_TEXT(MISC::_CREATE_VAR_STRING(10, "LITERAL_STRING", message), 0.025f, 0.08f);
}

void restoreOriginalModel() {
    if (!modelApplied || !originalModel) return;

    STREAMING::REQUEST_MODEL(originalModel, false);
    const ULONGLONG deadline = GetTickCount64() + 2500;

    while (!STREAMING::HAS_MODEL_LOADED(originalModel) && GetTickCount64() < deadline) WAIT(0);

    if (STREAMING::HAS_MODEL_LOADED(originalModel)) {
        PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), originalModel, false);
        STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(originalModel);
        log("restored original model");
    }

    modelApplied = false;
}

void resetEffects(const char* reason) {
    Ped ped = PLAYER::PLAYER_PED_ID();

    if (ped && ENTITY::DOES_ENTITY_EXIST(ped)) {
        ENTITY::SET_ENTITY_MOTION_BLUR(ped, false);
    }

    MISC::SET_TIME_SCALE(1.0f);
    currentBoost = 0.0f;
    restoreOriginalModel();

    if (reason) log(std::string("reset: ") + reason);
}

void tryApplyCustomModel() {
    if (!useCustomModel || !customModel) return;

    Ped ped = PLAYER::PLAYER_PED_ID();
    if (!ped || !ENTITY::DOES_ENTITY_EXIST(ped)) return;

    originalModel = ENTITY::GET_ENTITY_MODEL(ped);

    if (!STREAMING::IS_MODEL_VALID(customModel)) {
        status = "Flash model unavailable; speed enabled without model";
        log("custom model invalid/unavailable");
        return;
    }

    STREAMING::REQUEST_MODEL(customModel, false);
    const ULONGLONG deadline = GetTickCount64() + 3000;

    while (!STREAMING::HAS_MODEL_LOADED(customModel) && GetTickCount64() < deadline) WAIT(0);

    if (!STREAMING::HAS_MODEL_LOADED(customModel)) {
        status = "Flash model timed out; speed enabled without model";
        log("custom model load timeout");
        return;
    }

    PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), customModel, false);
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(customModel);
    modelApplied = true;
    log("custom model applied");
}

void keyboard(DWORD key, WORD, BYTE, BOOL, BOOL, BOOL wasDown, BOOL up) {
    if (key == 'W') {
        forwardHeld.store(!up);
        return;
    }

    if (key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT) {
        sprintHeld.store(!up);
        return;
    }

    if (up || wasDown) return;

    if (key == VK_F6) events.fetch_or(Toggle);
    if (key == VK_F9) events.fetch_or(Emergency);
}

void scriptMain() {
    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(moduleHandle, modulePath, MAX_PATH);

    const auto directory = std::filesystem::path(modulePath).parent_path();
    const auto iniPath = directory / L"FlashRDR2.ini";

    logFile.open(directory / L"FlashRDR2.log", std::ios::app);

    targetSpeed = static_cast<float>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"MaxSpeedMS", 32, iniPath.c_str())), 8, 80));

    acceleration = static_cast<float>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"AccelerationMS2", 24, iniPath.c_str())), 4, 100));

    deceleration = static_cast<float>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"DecelerationMS2", 36, iniPath.c_str())), 4, 150));

    slowWorld = GetPrivateProfileIntW(L"Flash", L"SlowWorld", 1, iniPath.c_str()) != 0;

    const int timeScalePercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"WorldTimeScalePercent", 72, iniPath.c_str())), 35, 100);

    worldTimeScale = static_cast<float>(timeScalePercent) / 100.0f;

    const int sprintBoostPercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"SprintBoostPercent", 175, iniPath.c_str())), 100, 400);
    sprintBoostMultiplier = static_cast<float>(sprintBoostPercent) / 100.0f;

    useCustomModel = GetPrivateProfileIntW(L"Flash", L"UseCustomModel", 0, iniPath.c_str()) != 0;

    wchar_t modelNameWide[128]{};
    GetPrivateProfileStringW(
        L"Flash", L"CustomModelName", L"", modelNameWide, 128, iniPath.c_str());

    if (modelNameWide[0] != L'\0') {
        char modelNameUtf8[128]{};
        WideCharToMultiByte(
            CP_UTF8, 0, modelNameWide, -1, modelNameUtf8, 128, nullptr, nullptr);
        customModel = MISC::GET_HASH_KEY(modelNameUtf8);
    }

    log("FlashRDR2 0.2.0 started; Story Mode only");

    auto lastTick = std::chrono::steady_clock::now();

    for (;;) {
        WAIT(0);

        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTick).count();
        lastTick = now;
        dt = std::clamp(dt, 0.0f, 0.05f);

        const unsigned ev = events.exchange(0);
        Ped ped = PLAYER::PLAYER_PED_ID();

        if (ev & Emergency) {
            resetEffects("F9 emergency");
            enabled = false;
            status = "OFF - emergency reset | F6 enable";
        }

        if (ev & Toggle) {
            if (enabled) {
                resetEffects("F6 disable");
                enabled = false;
                status = "OFF - F6 enable";
            } else if (eligible(ped)) {
                enabled = true;
                currentBoost = 0.0f;
                tryApplyCustomModel();
                status = "FLASH ON | W run | Shift turbo | F6 off | F9 emergency";
                log("enabled");
            } else {
                status = "Cannot enable here | Story Mode + on foot + game focused";
            }
        }

        ped = PLAYER::PLAYER_PED_ID();

        if (!enabled) {
            display(status);
            continue;
        }

        if (!eligible(ped)) {
            resetEffects("unsafe context / player unavailable");
            enabled = false;
            status = "OFF - unsafe context | return to Story Mode";
            display(status);
            continue;
        }

        // Flash mode: keep player stamina full every frame.
        PLAYER::RESTORE_PLAYER_STAMINA(PLAYER::PLAYER_ID(), 100.0f);

        const bool accelerating =
            forwardHeld.load()
            && !PED::IS_PED_FALLING(ped)
            && !PED::IS_PED_JUMPING(ped)
            && !PED::IS_PED_CLIMBING(ped);

        const float activeTargetSpeed =
            sprintHeld.load() ? (targetSpeed * sprintBoostMultiplier) : targetSpeed;

        if (accelerating) {
            currentBoost = std::min(activeTargetSpeed, currentBoost + acceleration * dt);
        } else {
            currentBoost = std::max(0.0f, currentBoost - deceleration * dt);
        }

        if (currentBoost > 0.05f) {
            const Vector3 forward = ENTITY::GET_ENTITY_FORWARD_VECTOR(ped);
            const Vector3 velocity = ENTITY::GET_ENTITY_VELOCITY(ped, 0);

            const float planarLength = std::sqrt(
                forward.x * forward.x + forward.y * forward.y);

            if (planarLength > 0.001f) {
                const float nx = forward.x / planarLength;
                const float ny = forward.y / planarLength;
                const float z = std::clamp(velocity.z, -10.0f, 8.0f);

                ENTITY::SET_ENTITY_VELOCITY(
                    ped,
                    nx * currentBoost,
                    ny * currentBoost,
                    z
                );
            }

            ENTITY::SET_ENTITY_MOTION_BLUR(ped, true);

            if (slowWorld) {
                MISC::SET_TIME_SCALE(worldTimeScale);
            }
        } else {
            ENTITY::SET_ENTITY_MOTION_BLUR(ped, false);
            MISC::SET_TIME_SCALE(1.0f);
        }

        const float kmh = ENTITY::GET_ENTITY_SPEED(ped) * 3.6f;

        static char hud[192];
        sprintf_s(
            hud,
            "FLASH ON | %.0f km/h | hold W + Shift | F6 off | F9 emergency",
            kmh
        );

        display(hud);
    }
}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        moduleHandle = module;
        scriptRegister(module, scriptMain);
        keyboardHandlerRegister(keyboard);
    } else if (reason == DLL_PROCESS_DETACH) {
        keyboardHandlerUnregister(keyboard);
        scriptUnregister(module);
    }

    return TRUE;
}
