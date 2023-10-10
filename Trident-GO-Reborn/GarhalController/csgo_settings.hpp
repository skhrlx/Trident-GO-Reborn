#pragma once
#include <windows.h>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include "fifo_map.hpp"

namespace csgo_settings
{
    void ReadConfig(const char* filename);
    void WriteConfig(const char* filename);

    extern int AimbotState;
    extern int AimbotKey;
    extern int AimbotKeyRifle;
    extern int AimbotTarget;
    extern int AimbotBullets;
    extern int AimbotFov;
    extern float AimbotSmooth;
    extern bool Bhop;
    extern bool SoundEsp;
    extern bool AutoAccept;
    extern bool AntiAFK;
    extern int SoundEspFov;
    extern bool Wallhack;
    extern bool TriggerBot;
    extern int TriggerBotKey;
    extern bool TriggerBotDelay;
    extern std::vector<int> TriggerBotAllowed;
    extern int TriggerBotDelayMin;
    extern int TriggerBotDelayMax;
    extern bool useVsync;
    extern bool showMenu;
    extern nlohmann::fifo_map<std::string, int> weaponIds;
    extern bool selectedWeaponIds[53];
    extern const std::vector<std::string> aimbotOptions;
    extern const std::vector<std::string> aimbotTargets;
    extern std::unordered_map<int, std::string> weaponIdHelper;
}
