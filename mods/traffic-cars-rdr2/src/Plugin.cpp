#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <main.h>
#include <natives.h>
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
HMODULE moduleHandle{};
std::atomic<bool> toggleRequested{false};

bool enabled = true;
Hash carModel = 0;
int trafficPercent = 55;
float shellOffsetZ = -0.65f;
float shellOffsetY = 0.10f;

struct TrafficCar {
    Ped rider{};
    Ped mount{};
    Object shell{};
};

std::vector<TrafficCar> cars;
std::ofstream logFile;

void log(const std::string& s) {
    if (logFile) logFile << GetTickCount64() << " " << s << std::endl;
}

bool online() {
    return NETWORK::NETWORK_IS_SESSION_ACTIVE()
        || NETWORK::NETWORK_IS_IN_SESSION()
        || NETWORK::NETWORK_IS_GAME_IN_PROGRESS();
}

bool validEntity(Entity e) {
    return e && ENTITY::DOES_ENTITY_EXIST(e);
}

bool alreadyConverted(Ped mount) {
    for (const auto& c : cars) {
        if (c.mount == mount) return true;
    }
    return false;
}

void cleanupDeadCars() {
    for (auto it = cars.begin(); it != cars.end();) {
        if (!validEntity(it->mount)) {
            if (validEntity(it->shell)) {
                Object obj = it->shell;
                OBJECT::DELETE_OBJECT(&obj);
            }
            it = cars.erase(it);
            continue;
        }
        ++it;
    }
}

void restoreAll() {
    for (auto& c : cars) {
        if (validEntity(c.mount)) {
            ENTITY::SET_ENTITY_VISIBLE(c.mount, true);
        }
        if (validEntity(c.shell)) {
            Object obj = c.shell;
            OBJECT::DELETE_OBJECT(&obj);
        }
    }
    cars.clear();
    log("restored traffic mounts");
}

bool ensureModel() {
    if (!carModel || !STREAMING::IS_MODEL_VALID(carModel)) return false;
    if (STREAMING::HAS_MODEL_LOADED(carModel)) return true;

    STREAMING::REQUEST_MODEL(carModel, false);
    const ULONGLONG deadline = GetTickCount64() + 2000;
    while (!STREAMING::HAS_MODEL_LOADED(carModel) && GetTickCount64() < deadline) {
        WAIT(0);
    }
    return STREAMING::HAS_MODEL_LOADED(carModel);
}

bool convertMountToCarShell(Ped rider, Ped mount) {
    if (!validEntity(rider) || !validEntity(mount) || alreadyConverted(mount)) return false;
    if (!ensureModel()) return false;

    Vector3 pos = ENTITY::GET_ENTITY_COORDS(mount, NULL, true);

    Object shell = OBJECT::CREATE_OBJECT(
        carModel,
        pos,
        true, true, true, false, false
    );

    if (!validEntity(shell)) return false;

    ENTITY::SET_ENTITY_COLLISION(shell, false, false);

    ENTITY::ATTACH_ENTITY_TO_ENTITY(
        shell,
        mount,
        0,
        Vector3(0.0f, shellOffsetY, shellOffsetZ),
        Vector3(0.0f, 0.0f, 0.0f),
        NULL,
        true,
        true,
        false,
        0,
        true,
        NULL,
        NULL
    );

    // The horse remains the hidden navigation/AI driver.
    // This gives us road-following NPC "cars" without breaking mount AI.
    ENTITY::SET_ENTITY_VISIBLE(mount, false);

    cars.push_back({ rider, mount, shell });
    return true;
}

void scanTraffic() {
    if (!enabled || online()) return;

    Ped player = PLAYER::PLAYER_PED_ID();
    Ped worldPeds[512]{};
    const int count = worldGetAllPeds(worldPeds, 512);

    for (int i = 0; i < count; ++i) {
        Ped rider = worldPeds[i];
        if (!validEntity(rider) || rider == player || ENTITY::IS_ENTITY_DEAD(rider)) continue;

        Ped mount = PED::GET_MOUNT(rider);
        if (!validEntity(mount) || alreadyConverted(mount)) continue;

        // Stable pseudo-random sampling from entity handles.
        const unsigned sample = (static_cast<unsigned>(rider) * 1103515245u + 12345u) % 100u;
        if (sample >= static_cast<unsigned>(trafficPercent)) continue;

        convertMountToCarShell(rider, mount);
    }
}

void displayStatus() {
    const char* state = enabled ? "TRAFFIC CARS ON" : "TRAFFIC CARS OFF";
    HUD::SET_TEXT_SCALE(0.30f, 0.30f);
    HUD::_SET_TEXT_COLOR(90, 220, 255, 255);
    HUD::SET_TEXT_CENTRE(false);
    HUD::_DISPLAY_TEXT(
        MISC::_CREATE_VAR_STRING(10, "LITERAL_STRING", state),
        0.025f, 0.115f
    );
}

void keyboard(DWORD key, WORD, BYTE, BOOL, BOOL, BOOL wasDown, BOOL up) {
    if (up || wasDown) return;
    if (key == VK_F10) toggleRequested.store(true);
}

void scriptMain() {
    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(moduleHandle, modulePath, MAX_PATH);

    const auto dir = std::filesystem::path(modulePath).parent_path();
    const auto iniPath = dir / L"TrafficCarsRDR2.ini";

    logFile.open(dir / L"TrafficCarsRDR2.log", std::ios::app);

    trafficPercent = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(
            L"TrafficCars", L"TrafficPercent", 55, iniPath.c_str())),
        0, 100
    );

    const int offsetZcm = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(
            L"TrafficCars", L"ShellOffsetZcm", -65, iniPath.c_str())),
        -300, 300
    );
    const int offsetYcm = std::clamp(
        static_cast<int>(GetPrivateProfileIntW(
            L"TrafficCars", L"ShellOffsetYcm", 10, iniPath.c_str())),
        -300, 300
    );

    shellOffsetZ = static_cast<float>(offsetZcm) / 100.0f;
    shellOffsetY = static_cast<float>(offsetYcm) / 100.0f;

    wchar_t modelWide[128]{};
    GetPrivateProfileStringW(
        L"TrafficCars",
        L"CarModelName",
        L"ironroadster",
        modelWide,
        128,
        iniPath.c_str()
    );

    char modelUtf8[128]{};
    WideCharToMultiByte(
        CP_UTF8, 0, modelWide, -1, modelUtf8, 128, nullptr, nullptr
    );
    carModel = MISC::GET_HASH_KEY(modelUtf8);

    log("TrafficCarsRDR2 started");

    ULONGLONG nextScan = 0;

    for (;;) {
        WAIT(0);

        if (toggleRequested.exchange(false)) {
            enabled = !enabled;
            if (!enabled) restoreAll();
            log(enabled ? "enabled" : "disabled");
        }

        if (online()) {
            if (!cars.empty()) restoreAll();
            displayStatus();
            continue;
        }

        cleanupDeadCars();

        const ULONGLONG now = GetTickCount64();
        if (enabled && now >= nextScan) {
            scanTraffic();
            nextScan = now + 1500;
        }

        displayStatus();
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
