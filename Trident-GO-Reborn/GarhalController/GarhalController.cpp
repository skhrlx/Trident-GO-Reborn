#pragma comment(lib,"ntdll.lib")
#pragma warning(disable : 26451) // Bug in VS according to Stackoverflow.

#include "offsets.hpp"
#include "data.hpp"
#include <iostream>
#include <TlHelp32.h>
#include "Aimbot.hpp"
#include "..\common\bsp\BSPParser.hpp"
#include "engine.hpp"
#include "Entity.hpp"
#include <Windows.h>
#include "csgo_menu.hpp"
#include "csgo_settings.hpp"
#include "imgui_extensions.h"
#include "utils.hpp"
#include "Arduino.hpp"

// hazedumper namespace
using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

// Declarations
inline Aimbot aim = NULL;

bool CheckDriverStatus() {
    int icheck = 82;//29
    NTSTATUS status = 0;

    uintptr_t BaseAddr = Driver::GetBaseAddress(GetCurrentProcessId());
    if (BaseAddr == 0) {
        return false;
    }

    int checked = Driver::read<int>(GetCurrentProcessId(), (uintptr_t)&icheck, &status);
    if (checked != icheck) {
        return false;
    }
    return true;
}

int main()
{
    if (!utils::IsProcessElevated(GetCurrentProcess()))
    {
        std::cout << "Process is not elevated!" << std::endl;
        MessageBoxA(0, "Process is not elevated!", "Trident", MB_OK | MB_ICONWARNING);
        return 0x1;
    }

    if (!Driver::initialize() || !CheckDriverStatus()) {
        UNICODE_STRING VariableName = RTL_CONSTANT_STRING(VARIABLE_NAME);
        NtSetSystemEnvironmentValueEx(
            &VariableName,
            &DummyGuid,
            0,
            0,
            ATTRIBUTES);//delete var

        //exit(1);
        //return 1;
    }

    Arduino::Connect("\\\\.\\\COM6");

    std::string random = utils::GenerateStr(20);
    SetConsoleTitleA(random.c_str());

    bool PrintOnce = false;

    while (PID == 0 || ClientAddr == 0 || EngineAddr == 0)
    {
        if (!PrintOnce)
        {
            std::cout << "Addresses are 0x0. Waiting for CSGO... " << std::endl;
            PrintOnce = true;
        }

        Sleep(1000);
    }

    std::cout << "Addresses look good. Starting..." << std::endl;

    // Initialize our renderer
    CSGORender csgoRender;
    if (!csgoRender.createWindow())
    {
        std::cout << "Failed to create window!" << std::endl;
        MessageBoxA(0, "Failed to create window!", "Garhal", MB_OK | MB_ICONWARNING);
        return 0x1;
    }

    // Prep parser
    hazedumper::BSPParser bspParser;

    std::cout << "==== Memory Addresses ====" << std::endl;
    std::cout << "PID: " << PID << std::endl;
    std::cout << "ClientAddr: " << std::hex << ClientAddr << std::endl;
    std::cout << "EngineAddr: " << std::hex << EngineAddr << std::endl;

    std::cout << "==== Config Values ====" << std::endl;
    std::cout << "AimbotState: " << static_cast<unsigned>(csgo_settings::AimbotState) << std::endl;
    std::cout << "AimbotKey: " << static_cast<unsigned>(csgo_settings::AimbotKey) << std::endl;
    std::cout << "AimbotTarget: " << static_cast<unsigned>(csgo_settings::AimbotTarget) << std::endl;
    std::cout << "AimbotBullets: " << static_cast<unsigned>(csgo_settings::AimbotBullets) << std::endl;
    std::cout << "Bhop: " << std::boolalpha << csgo_settings::Bhop << std::endl;
    std::cout << "Wallhack: " << std::boolalpha << csgo_settings::Wallhack << std::endl;
    std::cout << "TriggerBot: " << std::boolalpha << csgo_settings::TriggerBot << std::endl;
    std::cout << "TriggerBotKey: " << static_cast<unsigned>(csgo_settings::TriggerBotKey) << std::endl;
    std::cout << "TriggerBotDelay: " << std::boolalpha << csgo_settings::TriggerBotDelay << std::endl;

    aim = Aimbot(&bspParser);

    // We are creating a renderData list where you can dump all the informations you wish to render
    // Note that we are currently only using this to render bones, It is up to you what you make out of it.
    std::vector<RenderData> renderDatas;

    while (true)
    {
        // Clear render datas
        renderDatas.clear();
        
        if (!engine::IsInGame())
        {
            Sleep(500);
            continue;
        }

        // Ready to read, reserve a potentional amount in the memory.
        renderDatas.reserve(64);

        uint32_t LocalPlayer = Driver::read<uint32_t>(PID, ClientAddr + dwLocalPlayer);
        Entity LocalPlayerEnt = Entity(LocalPlayer);

        if (aim.localPlayer.GetEntityAddress() != LocalPlayerEnt.GetEntityAddress())
        {
            aim.localPlayer = LocalPlayerEnt;
        }

        uint8_t OurTeam = LocalPlayerEnt.getTeam();

        Vector3 myPosition = LocalPlayerEnt.getAbsolutePosition();

        for (short int i = 0; i < 64; i++)
        {
            uint32_t EntityAddr = Driver::read<uint32_t>(PID, ClientAddr + dwEntityList + i * 0x10);

            if (EntityAddr == NULL)
            {
                continue;
            }

            if (csgo_settings::Wallhack)
            {
                Entity ent = utils::CreateEntity(EntityAddr);
                if (ent.isValidPlayer() && !ent.IsDormant())
                {
                    Vector3 screenPos;
                    Vector3 position = ent.getAbsolutePosition();
                    if (engine::worldToScreen(position, screenPos))
                    {
                        ent.BuildBonePairs();

                        const float distance = (position - myPosition).norm();
                        RenderData renderData = ent.getRenderData(OurTeam, screenPos, distance);

                        Vector3 headScreenPos;
                        if (engine::worldToScreen(ent.getHeadPosition(), headScreenPos))
                        {
                            renderData.headPos = ImVec2(headScreenPos.at(0), headScreenPos.at(1));
                        }
                        
                        for (size_t y = 0; y < ent.CurrentBonePairs; ++y)
                        {
                            const auto& pair = ent.BonePairs[y];

                            if (pair.first == pair.second || pair.first < 0 || pair.second < 0 || pair.first > 128 || pair.second > 128)
                            {
                                continue;
                            }

                            Vector3 boneScreenPos;
                            Vector3 boneScreenPos2;
                            if (engine::worldToScreen(ent.BonePositions[pair.first], boneScreenPos)
                                && engine::worldToScreen(ent.BonePositions[pair.second], boneScreenPos2))
                            {
                                renderData.bones.emplace_back(ImVec2(boneScreenPos.at(0), boneScreenPos.at(1)),
                                    ImVec2(boneScreenPos2.at(0), boneScreenPos2.at(1)));
                            }
                        }
                        
                        renderDatas.push_back(renderData);
                    }
                }
            }

            // TODO: Implement your own 2D radar into the rendering as using this will flag you.
        }

        // Render all
        csgoRender.PollSystem();
        csgoRender.render(renderDatas);

        // This is flagged, use It only by understanding that you might get banned.
        if (csgo_settings::Bhop)
        {
            if (GetAsyncKeyState(0x20))
            {
                if (LocalPlayerEnt.isValidPlayer())
                {
                    if (!LocalPlayerEnt.isInAir())
                    {
                        Arduino::SendCommand(CMD_JUMP);
                    }
                }
            }
        }

        uint16_t WeaponID = LocalPlayerEnt.GetCurrentWeaponID();

        if (csgo_settings::TriggerBot && csgo_settings::TriggerBotKey == 0 || ImGui::IsCustomKeyPressed(csgo_settings::TriggerBotKey, true))
        {
            if (!IsWeaponNonAim(WeaponID))
            {
                aim.TriggerBot();
            }
        }

        if (GetAsyncKeyState(0x01) && IsWeaponRifle(WeaponID) || GetAsyncKeyState(0x01) && IsWeaponSMG(WeaponID)) {
            Arduino::SendCommand(CMD_AIM, {0, 1, 0});
        }

        if (csgo_settings::AimbotState == 1)
        {
            if (GetAsyncKeyState(0x4C))
            {
                if (IsWeaponPistol(WeaponID))
                {
                    csgo_settings::AimbotFov = 1;
                    csgo_settings::AimbotSmooth = 35;
                    aim.aimBot();
                }
                if (IsWeaponSniper(WeaponID))
                {
                    csgo_settings::AimbotFov = 1;
                    csgo_settings::AimbotSmooth = 35;
                    aim.aimBot();
                    aim.TriggerBot();
                }
                if (IsWeaponSMG(WeaponID))
                {
                    csgo_settings::AimbotFov = 1;
                    csgo_settings::AimbotSmooth = 35;
                    aim.aimBot();
                }
            }

            if (GetAsyncKeyState(0x01) && IsWeaponRifle(WeaponID))
            {
                csgo_settings::AimbotFov = 1;
                csgo_settings::AimbotSmooth = 25;
                aim.aimBot();
            }
        }

        if (csgo_settings::SoundEsp)
        {
            if (GetAsyncKeyState(0x43))
            {
                aim.CrosshairEsp();
            }
        }

        if (GetAsyncKeyState(0x56) & 0x8000) {
            Sleep(130);
            csgo_settings::Wallhack = !csgo_settings::Wallhack;
        }
        Sleep(1);

        if (!engine::IsInGame() && GetKeyState(0x24))
        {
            csgo_settings::AutoAccept = !csgo_settings::AutoAccept;
            Arduino::SendCommand(CMD_CLEFT);
            Sleep(2000);
        }

    }
}

#ifdef _WINDLL

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)main, nullptr, NULL, nullptr);
        return TRUE;
    }

    return TRUE;
}

#else

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    main();

    return 0;
}

#endif