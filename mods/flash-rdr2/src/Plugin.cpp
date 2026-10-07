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
#include <vector>

namespace {
HMODULE moduleHandle{};

std::atomic<unsigned> events{0};
std::atomic<bool> forwardHeld{false};
std::atomic<bool> sprintHeld{false};

constexpr unsigned ToggleFlash = 1;
constexpr unsigned Emergency = 2;

bool enabled = false;
bool explosiveBullets = true;
bool useCustomModel = false;
bool modelApplied = false;

float runSpeed = 24.0f;
float turboSpeed = 52.0f;
float runAnimRate = 2.5f;
float turboAnimRate = 4.0f;

float giantScale = 1.35f;
float runForce = 80.0f;
float turboForce = 260.0f;

Hash originalModel = 0;
Hash customModel = 0;

struct TrafficShell {
    Ped mount{};
    Entity shell{};
    bool vehicleShell{};
};

std::vector<TrafficShell> trafficShells;
Hash addonCarModel = 0;
Hash fallbackVehicleModel = 0;
int trafficPercent = 55;
bool trafficEnabled = true;
bool addonCarAvailable = false;

Vector3 lastImpact{};
bool haveLastImpact = false;

std::ofstream logFile;
const char* status = "F6 Flash | F7 flight | F9 reset";

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
    if (PED::IS_PED_ON_MOUNT(ped) || PED::IS_PED_IN_ANY_VEHICLE(ped, false) || PED::IS_PED_SWIMMING(ped)) return false;
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
    Player player = PLAYER::PLAYER_ID();
    Ped ped = PLAYER::PLAYER_PED_ID();

    PLAYER::SET_PLAYER_INVINCIBLE(player, false);

    if (ped && ENTITY::DOES_ENTITY_EXIST(ped)) {
        PED::SET_PED_CAN_RAGDOLL(ped, true);
        PED::_SET_PED_SCALE(ped, 1.0f);
        PED::SET_PED_MOVE_RATE_OVERRIDE(ped, 1.0f);
        ENTITY::SET_ENTITY_MOTION_BLUR(ped, false);
        ENTITY::SET_ENTITY_VELOCITY(ped, 0.0f, 0.0f, 0.0f);
    }

    MISC::SET_TIME_SCALE(1.0f);
    haveLastImpact = false;
    restoreOriginalModel();

    if (reason) log(std::string("reset: ") + reason);
}

void tryApplyCustomModel() {
    if (!useCustomModel || !customModel) return;

    Ped ped = PLAYER::PLAYER_PED_ID();
    if (!ped || !ENTITY::DOES_ENTITY_EXIST(ped)) return;

    originalModel = ENTITY::GET_ENTITY_MODEL(ped);

    if (!STREAMING::IS_MODEL_VALID(customModel)) {
        status = "Custom model unavailable; powers ON";
        log("custom model invalid/unavailable");
        return;
    }

    STREAMING::REQUEST_MODEL(customModel, false);
    const ULONGLONG deadline = GetTickCount64() + 3000;

    while (!STREAMING::HAS_MODEL_LOADED(customModel) && GetTickCount64() < deadline) WAIT(0);

    if (!STREAMING::HAS_MODEL_LOADED(customModel)) {
        status = "Custom model timed out; powers ON";
        log("custom model load timeout");
        return;
    }

    PLAYER::SET_PLAYER_MODEL(PLAYER::PLAYER_ID(), customModel, false);
    STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(customModel);
    modelApplied = true;
    log("custom model applied");
}


bool modelReady(Hash model) {
    if (!model || !STREAMING::IS_MODEL_VALID(model)) return false;
    if (STREAMING::HAS_MODEL_LOADED(model)) return true;

    STREAMING::REQUEST_MODEL(model, false);
    const ULONGLONG deadline = GetTickCount64() + 1500;
    while (!STREAMING::HAS_MODEL_LOADED(model) && GetTickCount64() < deadline) WAIT(0);
    return STREAMING::HAS_MODEL_LOADED(model);
}

bool mountAlreadyConverted(Ped mount) {
    for (const auto& t : trafficShells) {
        if (t.mount == mount) return true;
    }
    return false;
}

void deleteTrafficShell(TrafficShell& t) {
    if (t.mount && ENTITY::DOES_ENTITY_EXIST(t.mount)) {
        ENTITY::SET_ENTITY_VISIBLE(t.mount, true);
    }

    if (t.shell && ENTITY::DOES_ENTITY_EXIST(t.shell)) {
        if (t.vehicleShell) {
            Vehicle v = static_cast<Vehicle>(t.shell);
            VEHICLE::DELETE_VEHICLE(&v);
        } else {
            Object o = static_cast<Object>(t.shell);
            OBJECT::DELETE_OBJECT(&o);
        }
    }

    t.shell = 0;
}

void cleanupTraffic() {
    for (auto& t : trafficShells) deleteTrafficShell(t);
    trafficShells.clear();
}

void pruneTraffic() {
    for (auto it = trafficShells.begin(); it != trafficShells.end();) {
        if (!it->mount || !ENTITY::DOES_ENTITY_EXIST(it->mount)) {
            if (it->shell && ENTITY::DOES_ENTITY_EXIST(it->shell)) {
                if (it->vehicleShell) {
                    Vehicle v = static_cast<Vehicle>(it->shell);
                    VEHICLE::DELETE_VEHICLE(&v);
                } else {
                    Object o = static_cast<Object>(it->shell);
                    OBJECT::DELETE_OBJECT(&o);
                }
            }
            it = trafficShells.erase(it);
        } else {
            ++it;
        }
    }
}

bool attachAddonCarShell(Ped mount) {
    if (!modelReady(addonCarModel)) return false;

    const Vector3 pos = ENTITY::GET_ENTITY_COORDS(mount, NULL, true);
    Object shell = OBJECT::CREATE_OBJECT(
        addonCarModel,
        pos.x, pos.y, pos.z,
        true, true, true, false, false
    );
    if (!shell || !ENTITY::DOES_ENTITY_EXIST(shell)) return false;

    ENTITY::SET_ENTITY_COLLISION(shell, false, false);
    ENTITY::ATTACH_ENTITY_TO_ENTITY(
        shell, mount, 0,
        0.0f, 0.15f, -0.70f,
        0.0f, 0.0f, 0.0f,
        false,
        true,
        false,
        false,
        0,
        true,
        false,
        false
    );

    ENTITY::SET_ENTITY_VISIBLE(mount, false);
    trafficShells.push_back({mount, shell, false});
    return true;
}

bool attachFallbackVehicleShell(Ped mount) {
    if (!modelReady(fallbackVehicleModel)) return false;

    const Vector3 pos = ENTITY::GET_ENTITY_COORDS(mount, NULL, true);
    Vehicle shell = VEHICLE::CREATE_VEHICLE(
        fallbackVehicleModel,
        pos.x, pos.y, pos.z,
        ENTITY::GET_ENTITY_HEADING(mount),
        false, false, false, false
    );

    if (!shell || !ENTITY::DOES_ENTITY_EXIST(shell)) return false;

    ENTITY::SET_ENTITY_COLLISION(shell, false, false);
    ENTITY::ATTACH_ENTITY_TO_ENTITY(
        shell, mount, 0,
        0.0f, -0.25f, -0.55f,
        0.0f, 0.0f, 0.0f,
        false,
        true,
        false,
        false,
        0,
        true,
        false,
        false
    );

    ENTITY::SET_ENTITY_VISIBLE(mount, false);
    trafficShells.push_back({mount, shell, true});
    return true;
}

void scanTraffic() {
    if (!trafficEnabled || online()) return;

    Ped player = PLAYER::PLAYER_PED_ID();
    Ped worldPeds[512]{};
    const int count = worldGetAllPeds(worldPeds, 512);

    for (int i = 0; i < count; ++i) {
        Ped rider = worldPeds[i];
        if (!rider || rider == player || !ENTITY::DOES_ENTITY_EXIST(rider) || ENTITY::IS_ENTITY_DEAD(rider)) continue;

        Ped mount = PED::GET_MOUNT(rider);
        if (!mount || !ENTITY::DOES_ENTITY_EXIST(mount) || mountAlreadyConverted(mount)) continue;

        const unsigned sample = (static_cast<unsigned>(rider) * 1103515245u + 12345u) % 100u;
        if (sample >= static_cast<unsigned>(trafficPercent)) continue;

        // First try the real addon car. If it is not installed, fall back to a
        // base-game buggy shell so the traffic feature is still visibly working.
        if (!attachAddonCarShell(mount)) {
            attachFallbackVehicleShell(mount);
        }
    }
}

void keyboard(DWORD key, WORD, BYTE, BOOL, BOOL, BOOL wasDown, BOOL up) {
    if (key == 'W') { forwardHeld.store(!up); return; }
    if (key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT) {
        sprintHeld.store(!up);
        return;
    }

    if (up || wasDown) return;

    if (key == VK_F6) events.fetch_or(ToggleFlash);
    if (key == VK_F9) events.fetch_or(Emergency);
    if (key == VK_F10) {
        trafficEnabled = !trafficEnabled;
        if (!trafficEnabled) cleanupTraffic();
    }
}

void updateExplosiveBullets(Ped ped) {
    if (!explosiveBullets || !PED::IS_PED_SHOOTING(ped)) return;

    Vector3 impact{};
    if (!WEAPON::GET_PED_LAST_WEAPON_IMPACT_COORD(ped, &impact)) return;

    bool isNew = !haveLastImpact
        || std::fabs(impact.x - lastImpact.x) > 0.15f
        || std::fabs(impact.y - lastImpact.y) > 0.15f
        || std::fabs(impact.z - lastImpact.z) > 0.15f;

    if (!isNew) return;

    lastImpact = impact;
    haveLastImpact = true;

    FIRE::ADD_OWNED_EXPLOSION(
        ped,
        impact.x, impact.y, impact.z,
        25,
        1.0f,
        true,
        false,
        0.25f
    );
}

void updateFlashRun(Ped ped) {
    const bool moving =
        forwardHeld.load()
        && !PED::IS_PED_JUMPING(ped)
        && !PED::IS_PED_FALLING(ped)
        && !PED::IS_PED_RAGDOLL(ped)
        && !PED::IS_PED_CLIMBING(ped);

    if (!moving) {
        PED::SET_PED_MOVE_RATE_OVERRIDE(ped, 1.0f);
        ENTITY::SET_ENTITY_MOTION_BLUR(ped, false);
        return;
    }

    const bool turbo = sprintHeld.load();

    // Same core approach used by open-source RDR2 trainers:
    // apply local forward force every frame while W is held.
    // Force Y is "forward" in the entity's local space.
    const float forceForward = turbo ? turboForce : runForce;

    // Keep native locomotion animation active while force provides the real speed.
    PED::SET_PED_MOVE_RATE_OVERRIDE(ped, turbo ? 10.0f : 7.0f);

    ENTITY::APPLY_FORCE_TO_ENTITY(
        ped,
        1,
        0.0f,
        forceForward,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0,
        true,
        true,
        true,
        true,
        true
    );

    ENTITY::SET_ENTITY_MOTION_BLUR(ped, true);
}

void scriptMain() {
    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(moduleHandle, modulePath, MAX_PATH);

    const auto directory = std::filesystem::path(modulePath).parent_path();
    const auto iniPath = directory / L"FlashRDR2.ini";

    logFile.open(directory / L"FlashRDR2.log", std::ios::app);

    runSpeed = static_cast<float>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"RunSpeedMS", 24, iniPath.c_str())), 5, 60));

    turboSpeed = static_cast<float>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"TurboSpeedMS", 52, iniPath.c_str())), 10, 120));

    explosiveBullets = GetPrivateProfileIntW(L"Flash", L"ExplosiveBullets", 1, iniPath.c_str()) != 0;
    const int giantScalePercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"GiantScalePercent", 135, iniPath.c_str())), 100, 400);
    giantScale = static_cast<float>(giantScalePercent) / 100.0f;

    runForce = static_cast<float>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"RunForce", 80, iniPath.c_str())), 5, 150));

    turboForce = static_cast<float>(std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Flash", L"TurboForce", 260, iniPath.c_str())), 10, 300));

    useCustomModel = GetPrivateProfileIntW(L"Flash", L"UseCustomModel", 0, iniPath.c_str()) != 0;

    trafficPercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(L"Traffic", L"TrafficPercent", 55, iniPath.c_str())),
        0, 100
    );

    wchar_t trafficModelWide[128]{};
    GetPrivateProfileStringW(
        L"Traffic", L"AddonCarModelName", L"ironroadster",
        trafficModelWide, 128, iniPath.c_str()
    );

    char trafficModelUtf8[128]{};
    WideCharToMultiByte(
        CP_UTF8, 0, trafficModelWide, -1,
        trafficModelUtf8, 128, nullptr, nullptr
    );

    addonCarModel = MISC::GET_HASH_KEY(trafficModelUtf8);
    fallbackVehicleModel = MISC::GET_HASH_KEY("BUGGY01");
    addonCarAvailable = STREAMING::IS_MODEL_VALID(addonCarModel);

    wchar_t modelNameWide[128]{};
    GetPrivateProfileStringW(L"Flash", L"CustomModelName", L"", modelNameWide, 128, iniPath.c_str());

    if (modelNameWide[0] != L'\0') {
        char modelNameUtf8[128]{};
        WideCharToMultiByte(CP_UTF8, 0, modelNameWide, -1, modelNameUtf8, 128, nullptr, nullptr);
        customModel = MISC::GET_HASH_KEY(modelNameUtf8);
    }

    log("FlashRDR2 v13.1 compile-fixed single-ASI traffic started; Story Mode only");

    auto lastTick = std::chrono::steady_clock::now();
    ULONGLONG nextTrafficScan = 0;

    for (;;) {
        WAIT(0);

        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTick).count();
        lastTick = now;
        dt = std::clamp(dt, 0.0f, 0.033f);

        const unsigned ev = events.exchange(0);
        Ped ped = PLAYER::PLAYER_PED_ID();
        Player player = PLAYER::PLAYER_ID();

        if (ev & Emergency) {
            resetEffects("F9 emergency");
            enabled = false;
            status = "OFF - F6 enable";
        }

        if (ev & ToggleFlash) {
            if (enabled) {
                resetEffects("F6 disable");
                enabled = false;
                status = "OFF - F6 enable";
            } else if (eligible(ped)) {
                enabled = true;
                tryApplyCustomModel();
                status = "FLASH ON | W super run | Shift max turbo | Space super jump";
                log("enabled");
            }
        }

        ped = PLAYER::PLAYER_PED_ID();
        player = PLAYER::PLAYER_ID();

        if (!enabled) {
            display(status);
            continue;
        }

        if (!eligible(ped)) {
            resetEffects("unsafe context");
            enabled = false;
            status = "OFF - unsafe context";
            display(status);
            continue;
        }

        PLAYER::SET_PLAYER_INVINCIBLE(player, true);
        PLAYER::RESTORE_PLAYER_STAMINA(player, 1.0f);
        PED::SET_PED_CAN_RAGDOLL(ped, false);
        PED::_SET_PED_SCALE(ped, giantScale);

        updateExplosiveBullets(ped);

        MISC::SET_SUPER_JUMP_THIS_FRAME(player);
        updateFlashRun(ped);

        pruneTraffic();
        const ULONGLONG nowMs = GetTickCount64();
        if (trafficEnabled && nowMs >= nextTrafficScan) {
            scanTraffic();
            nextTrafficScan = nowMs + 1500;
        }

        const float kmh = ENTITY::GET_ENTITY_SPEED(ped) * 3.6f;

        static char hud[256];
        sprintf_s(
            hud,
            "FLASH v13.1 | %s | %.0f km/h | SCALE %.2fx | TRAFFIC %s | GODMODE | INF STAMINA | F10 traffic",
            sprintHeld.load() ? "MAX TURBO" : "SUPER RUN",
            kmh,
            giantScale,
            trafficEnabled ? (addonCarAvailable ? "ADDON CAR" : "FALLBACK BUGGY") : "OFF"
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
