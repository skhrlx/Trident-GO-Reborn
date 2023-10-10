#include "Engine.hpp"
#include "offsets.hpp"

// hazedumper namespace
using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

#include <iostream>

bool engine::worldToScreen(const Vector3& from, Vector3& to)
{
    WorldToScreenMatrix matrix = Driver::read<WorldToScreenMatrix>(PID, ClientAddr + dwViewMatrix);

    const auto w = matrix(3, 0) * from(0) + matrix(3, 1) * from(1) + matrix(3, 2) * from(2) + matrix(3, 3);
    if (w < 0.001f)
        return false;

    to(0) = ImGui::GetIO().DisplaySize.x / 2.0f;
    to(1) = ImGui::GetIO().DisplaySize.y / 2.0f;
    to(0) *= 1.0f + (matrix(0, 0) * from(0) + matrix(0, 1) * from(1) + matrix(0, 2) * from(2) + matrix(0, 3)) / w;
    to(1) *= 1.0f - (matrix(1, 0) * from(0) + matrix(1, 1) * from(1) + matrix(1, 2) * from(2) + matrix(1, 3)) / w;

    return true;
}

bool engine::IsInGame()
{
    uint32_t ClientState = Driver::read<uint32_t>(PID, EngineAddr + dwClientState);
    uint32_t Second = Driver::read<uint32_t>(PID, ClientState + dwClientState_State);
    return GetGameState(Second) == InGame;
}

GameState engine::GetGameState(uint8_t State)
{
	switch (State)
    {
		case 0: return Lobby;
        case 1: return Loading;
        case 2: return Connecting;
        case 5: return Connected;
        case 6: return InGame;
    }
    return UnknownG;
}

Vector3 engine::getViewAngles()
{
    uint32_t ClientState = Driver::read<uint32_t>(PID, EngineAddr + dwClientState);
    return Driver::read<Vector3>(PID, ClientState + dwClientState_ViewAngles);
}

void engine::setViewAngles(Vector3& viewAngles)
{
    std::cout << "set view angles" << std::endl;
}