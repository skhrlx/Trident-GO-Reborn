#pragma warning (disable : 26451)

#include "Aimbot.hpp"
#include <chrono>
#include <cstddef>
#include <iostream>

#include "csgo_settings.hpp"
#include "data.hpp"
#include "offsets.hpp"
#include "Engine.hpp"
#include "Arduino.hpp"

using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

Vector3 Aimbot::aimAnglesTo(Vector3& target)
{
    //Vector3 localPosition = localPlayer.getAbsolutePosition();
    Vector3 localPosition = localPlayer.getHeadPosition();

    Vector3 punchAngles = localPlayer.getAimPunch();

    Vector3 dPosition = localPosition - target;

    float hypotenuse = sqrt(dPosition(0) * dPosition(0) + dPosition(1) * dPosition(1));

    Vector3 a((float)(atan2f(dPosition(2), hypotenuse) * 57.295779513082f), (float)(atanf(dPosition(1) / dPosition(0)) * 57.295779513082f), 0);

    if (dPosition(0) >= 0.f)
        a(1) += 180.0f;

    Vector3 aimAngles;
    aimAngles(0) = a(0);     // up and down
    aimAngles(1) = a(1);      // left and right

    aimAngles(0) -= punchAngles(0) * 2;
    aimAngles(1) -= punchAngles(1) * 2;

    normalizeAngles(aimAngles);
    clampAngles(aimAngles);

    aimAngles(2) = 0.f;
    return aimAngles;
}

Vector3 Aimbot::angleDifferenceToEntity(Entity& localPlayer, Entity& entity)
{
    Vector3 viewAngles = getViewAngles();
    Vector3 pos;

    if (csgo_settings::AimbotTarget == 3)
    {
        if (entity.getHealth() <= 50)
        {
            pos = entity.getHeadPosition();
        }
        else
        {
            pos = entity.getHeadPosition();//target.getBonePosition(boneId);
        }
    }
    else if (csgo_settings::AimbotTarget == 2)
    {
        pos = entity.getHeadPosition();
    }
    else
    {
        pos = entity.getHeadPosition();
    }

    Vector3 aimAngles = aimAnglesTo(pos);

    Vector3 dAngle(-1, -1, 0);

    if (aimAngles(0) != aimAngles(0) || aimAngles(1) != aimAngles(1))
        return dAngle;


    dAngle(0) = abs(aimAngles(0) - viewAngles(0));
    dAngle(1) = abs(aimAngles(1) - viewAngles(1));

    return dAngle;
}


bool Aimbot::enemyIsInCrossHair()
{
    if (!localPlayer.isValidPlayer())
    {
        return false;
    }

    uint32_t crossHairId = localPlayer.getCrosshairId();
    if (crossHairId <= 0 || crossHairId > 65)
    {
        return false;
    }

    crossHairId -= 1;

    uint32_t EntityAddr = Driver::read<uint32_t>(PID, ClientAddr + hazedumper::signatures::dwEntityList + 0x10 * crossHairId);

    Entity target = Entity(EntityAddr);

    if (!target.isValidPlayer())
    {
        return false;
    }

    bool isEnemy = target.getTeam() != localPlayer.getTeam();

    return isEnemy;
}

Entity Aimbot::findClosestEnemyToFOV()
{
    uint32_t localPlayerTeam = localPlayer.getTeam();
    Entity closestPlayer;

    if (!localPlayer.isValidPlayer())
        return closestPlayer;

    Vector3 localPosition = localPlayer.getAbsolutePosition();

    float closest = FLT_MAX;
    for (int i = 0; i < 64; i++)
    {
        uint32_t EntityAddr = Driver::read<uint32_t>(PID, ClientAddr + hazedumper::signatures::dwEntityList + i * 0x10);

        if (EntityAddr == NULL)
        {
            continue;
        }

        Entity entity = Entity(EntityAddr);

        if (!entity.isValidPlayer()) 
        {
            continue;
        }

        if (entity.getTeam() == localPlayerTeam) 
        {
            continue;
        }

        Vector3 entityPosition;

        if (csgo_settings::AimbotTarget == 3)
        {
            if (entity.getHealth() <= 50)
            {
                entityPosition = entity.GetBonePosition(CHEST_BONE_ID);
            }
            else
            {
                entityPosition = entity.getHeadPosition();
            }
        }
        else if (csgo_settings::AimbotTarget == 2)
        {
            entityPosition = entity.GetBonePosition(CHEST_BONE_ID);
        }
        else
        {
            entityPosition = entity.getHeadPosition();
        }

    	
        if (!bspParser->is_visible(localPosition, entityPosition)) 
        {
            continue;
        }

        Vector3 dAngle = angleDifferenceToEntity(localPlayer, entity);
        float screenDifferenceToEntity = sqrt(dAngle(0) * dAngle(0) + dAngle(1) * dAngle(1));
        if (screenDifferenceToEntity >= closest) 
        {
            continue;
        }

        if (screenDifferenceToEntity >= csgo_settings::AimbotFov)
        {
            //std::cout << fov << " " << screenDifferenceToEntity << std::endl;
            continue;
        }

        closest = screenDifferenceToEntity;
        closestPlayer = entity;
    }

    return closestPlayer;
}

Entity Aimbot::mYaw()
{
    uint32_t Yaw = Driver::read<uint32_t>(PID, ClientAddr + hazedumper::signatures::m_yawClassPtr);
    return Yaw;
}

Entity Aimbot::findClosestEnemyToFOV2()
{
    uint32_t localPlayerTeam = localPlayer.getTeam();
    Entity closestPlayer;

    if (!localPlayer.isValidPlayer())
        return closestPlayer;

    Vector3 localPosition = localPlayer.getAbsolutePosition();

    float closest = FLT_MAX;
    for (int i = 0; i < 64; i++)
    {
        uint32_t EntityAddr = Driver::read<uint32_t>(PID, ClientAddr + hazedumper::signatures::dwEntityList + i * 0x10);

        if (EntityAddr == NULL)
        {
            continue;
        }

        Entity entity = Entity(EntityAddr);

        if (!entity.isValidPlayer())
        {
            continue;
        }

        if (entity.getTeam() == localPlayerTeam)
        {
            continue;
        }

        Vector3 entityPosition;

        entityPosition = entity.GetBonePosition(CHEST_BONE_ID);

        if (!bspParser->is_visible(localPosition, entityPosition))
        {
            continue;
        }

        Vector3 dAngle = angleDifferenceToEntity(localPlayer, entity);
        float screenDifferenceToEntity = sqrt(dAngle(0) * dAngle(0) + dAngle(1) * dAngle(1));
        if (screenDifferenceToEntity >= closest)
        {
            continue;
        }

        if (screenDifferenceToEntity >= csgo_settings::SoundEspFov)
        {
            continue;
        }

        closest = screenDifferenceToEntity;
        closestPlayer = entity;
    }

    return closestPlayer;
}

Vector3 Aimbot::getViewAngles()
{
    uint32_t clientState = Driver::read<uint32_t>(PID, EngineAddr + hazedumper::signatures::dwClientState);
	return Driver::read<Vector3>(PID, clientState + hazedumper::signatures::dwClientState_ViewAngles);
}


float Aimbot::getSensitivity()
{
    uint32_t sensitivityPtr = ClientAddr + hazedumper::signatures::dwSensitivityPtr;
    uint32_t sensitivity = Driver::read<uint32_t>(PID, ClientAddr + hazedumper::signatures::dwSensitivity);

    sensitivity ^= sensitivityPtr;

    float sens = *reinterpret_cast<float*>(&sensitivity);

    return sens;
}

const char* Aimbot::getMapDirectory()
{
    uint32_t clientState = Driver::read<uint32_t>(PID, EngineAddr + + hazedumper::signatures::dwClientState);
    static std::array<char, 0x120> mapDirectory = Driver::read<std::array<char, 0x120>>(PID, clientState + hazedumper::signatures::dwClientState_MapDirectory);
    return mapDirectory.data();
}

const char* Aimbot::getGameDirectory()
{
    static std::array<char, 0x120> gameDirectory = Driver::read<std::array<char, 0x120>>(PID, EngineAddr + hazedumper::signatures::dwGameDir);
    return gameDirectory.data();
}

void Aimbot::setViewAngles(Vector3& viewAngles)
{
    uint32_t clientState = Driver::read<uint32_t>(PID, EngineAddr + dwClientState);
    Driver::write(PID, clientState + dwClientState_ViewAngles, viewAngles);
}

bool Aimbot::EnemyIsInCrossHair()
{
    if (!localPlayer.isValidPlayer()) 
    {
        return false;
    }

    uint32_t localPlayerTeam = localPlayer.getTeam();
    uint16_t CrosshairID = localPlayer.getCrosshairId();
    if (CrosshairID <= 0 || CrosshairID > 65)
    {
        return false;
    }

    CrosshairID -= 1;
    uint32_t crosshairentity = Driver::read<uint32_t>(PID, ClientAddr + hazedumper::signatures::dwEntityList + 0x10 * CrosshairID);
	if (crosshairentity == NULL)
	{
        return false;
	}
	
    Entity target(crosshairentity);

    if (!target.isValidPlayer()) 
    {
        return false;
    }

    bool isEnemy = target.getTeam() != localPlayerTeam;

    return isEnemy;
}

void Aimbot::TriggerBot()
{
    if (EnemyIsInCrossHair()) 
    {
        localPlayer.shoot();
    }
}

bool Aimbot::aimAssist()
{
    const char* gameDirectory = getGameDirectory();
    const char* mapDirectory = getMapDirectory();
    bspParser->parse_map(gameDirectory, mapDirectory);

    if (!localPlayer.isValidPlayer())
    {
        return false;
    }

    int16_t WeaponID = localPlayer.GetCurrentWeaponID();

    if (WeaponID == 0)
    {
        return false;
    }

    // Enable AimAssist after Nth bullet except pistols & snipers.
    if (localPlayer.getShotsFired() < csgo_settings::AimbotBullets && !IsWeaponPistol(WeaponID) && !IsWeaponSniper(WeaponID))
    {
        return false;
    }

    static Entity target = findClosestEnemyToFOV();
    static auto killTime = std::chrono::high_resolution_clock::now();
    static Vector3* lastPosition = NULL;

    Entity newTarget = findClosestEnemyToFOV();
    if (target.isValidPlayer() && !target.isInAir() && newTarget.isValidPlayer() && target.GetEntityAddress() == newTarget.GetEntityAddress())
    {
        killTime = std::chrono::high_resolution_clock::now();
        lastPosition = new Vector3();


        if (csgo_settings::AimbotTarget == 3)
        {
            if (target.getHealth() <= 50)
            {
                *lastPosition = target.getHeadPosition();
            }
            else
            {
                *lastPosition = target.getHeadPosition();
            }
        }
        else if (csgo_settings::AimbotTarget == 2)
        {
            *lastPosition = target.getHeadPosition();
        }
        else
        {
            *lastPosition = target.getHeadPosition();
        }
    }

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeSinceKill = now - killTime;

    // continue shooting at target even after they die for 0.2 seconds
    if (timeSinceKill.count() > 0.002f && lastPosition)
    {
        delete lastPosition;
        lastPosition = NULL;
    }

    if (lastPosition)
    {
        if (!bspParser->is_visible(localPlayer.getAbsolutePosition(), *lastPosition))
            return false;

        Vector3 targetAngle = aimAnglesTo(*lastPosition);
        if (targetAngle(0) != targetAngle(0) || targetAngle(1) != targetAngle(1) || targetAngle(2) != targetAngle(2))
        {
            return false;
        }

        Vector3 viewAngles = getViewAngles();


        float dYawTowardsRight = viewAngles(1) - targetAngle(1);
        while (dYawTowardsRight < 0)
        {
            dYawTowardsRight += 360.f;
        }
        while (dYawTowardsRight > 360.f)
        {
            dYawTowardsRight -= 360.f;
        }

        float dYawTowardsLeft = dYawTowardsRight - 360.f;

        Vector3 dAngles = targetAngle - viewAngles;
        if (abs(dYawTowardsRight) < abs(dYawTowardsLeft))
        {
            dAngles(1) = -dYawTowardsRight;
        }
        else
        {
            dAngles(1) = -dYawTowardsLeft;
        }

        Vector3 smoothTargetAngles = viewAngles + dAngles / csgo_settings::AimbotSmooth;

        normalizeAngles(smoothTargetAngles);
        clampAngles(smoothTargetAngles);

        smoothTargetAngles(2) = 0.f;
        setViewAngles(smoothTargetAngles);

        return true;
    }

    // find new target
    target = newTarget;
    killTime = std::chrono::high_resolution_clock::now();
    lastPosition = NULL;

    return false;
}


bool Aimbot::aimBot()
{
    const char* gameDirectory = getGameDirectory();
    const char* mapDirectory = getMapDirectory();
    bspParser->parse_map(gameDirectory, mapDirectory);

    if (!localPlayer.isValidPlayer())
    {
        return false;
    }

    int16_t WeaponID = localPlayer.GetCurrentWeaponID();

    Entity target = findClosestEnemyToFOV();

    static auto killTime = std::chrono::high_resolution_clock::now();
    static Vector3* lastPosition = NULL;

    Entity newTarget = findClosestEnemyToFOV();
    if (target.isValidPlayer() && !target.isInAir() && newTarget.isValidPlayer() && target.GetEntityAddress() == newTarget.GetEntityAddress())
    {
        killTime = std::chrono::high_resolution_clock::now();
        lastPosition = new Vector3();

        Vector3 pos;
        Vector3 screen_coords;
        pos = engine::worldToScreen(target.getHeadPosition(), screen_coords);
        *lastPosition = screen_coords;
    }

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeSinceKill = now - killTime;

    // continue shooting at target even after they die for 0.2 seconds
    if (timeSinceKill.count() > 0.2f && lastPosition)
    {
        delete lastPosition;
        lastPosition = NULL;
    }

    if (lastPosition)
    {
        if (!bspParser->is_visible(localPlayer.getAbsolutePosition(), *lastPosition))
            return false;

        Vector3 pos;
        Vector3 screen_coords;
        pos = engine::worldToScreen(target.getHeadPosition(), screen_coords);
        float diff_x = screen_coords.at(0) - ImGui::GetIO().DisplaySize.x / 2;
        float diff_y = screen_coords.at(1) - ImGui::GetIO().DisplaySize.y / 2 ;
        if (IsWeaponRifle(WeaponID)) {
            float coords_x = diff_x * 0.5f;
            float coords_y = diff_y * 0.5f;
            Arduino::MouseMove(coords_x, coords_y, 0);
        }
        if (IsWeaponPistol(WeaponID)) {
            float coords_x = diff_x * 0.5f;
            float coords_y = diff_y * 0.5f;
            Arduino::MouseMove(coords_x, coords_y, 0);
        }
        if (IsWeaponSniper(WeaponID)) {
            float coords_x = diff_x * 0.5f;
            float coords_y = diff_y * 0.5f;
            Arduino::MouseMove(coords_x, coords_y, 0);
        }
        if (IsWeaponSMG(WeaponID) or IsWeaponShotgun(WeaponID)) {
            float coords_x = diff_x * 0.1f;
            float coords_y = diff_y * 0.5f;

            Arduino::MouseMove(coords_x, coords_y, 0);
        }
    }
    // find new target
    target = newTarget;
    killTime = std::chrono::high_resolution_clock::now();
    lastPosition = NULL;

    return false;
}


bool Aimbot::CrosshairEsp()
{
    const char* gameDirectory = getGameDirectory();
    const char* mapDirectory = getMapDirectory();
    bspParser->parse_map(gameDirectory, mapDirectory);

    if (!localPlayer.isValidPlayer())
    {
        return false;
    }

    int16_t WeaponID = localPlayer.GetCurrentWeaponID();

    if (WeaponID == 0)
    {
        return false;
    }

    static Entity target = findClosestEnemyToFOV2();
    static auto killTime = std::chrono::high_resolution_clock::now();
    static Vector3* lastPosition = NULL;

    Entity newTarget = findClosestEnemyToFOV2();
    if (target.isValidPlayer() && !target.isInAir() && newTarget.isValidPlayer() && target.GetEntityAddress() == newTarget.GetEntityAddress())
    {
        killTime = std::chrono::high_resolution_clock::now();
        lastPosition = new Vector3();

        *lastPosition = target.getHeadPosition();
    }

    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeSinceKill = now - killTime;

    if (timeSinceKill.count() > 0.0001f && lastPosition)
    {
        delete lastPosition;
        lastPosition = NULL;
    }

    if (lastPosition)
    {
        if (!bspParser->is_visible(localPlayer.getAbsolutePosition(), *lastPosition))
            return false;

        Vector3 targetAngle = aimAnglesTo(*lastPosition);
        if (targetAngle(0) != targetAngle(0) || targetAngle(1) != targetAngle(1) || targetAngle(2) != targetAngle(2))
        {
            return false;
        }

        Vector3 viewAngles = getViewAngles();
        Beep(1000, 100);

        return true;
    }

    // find new target
    target = newTarget;
    killTime = std::chrono::high_resolution_clock::now();
    lastPosition = NULL;

    return false;
}

void Aimbot::walkBot()
{
    if (!localPlayer.isValidPlayer())
    {
        return;
    }

    Vector3 viewAngles = getViewAngles();
    Vector3 velocity = localPlayer.getVelocity();
    float speed = sqrt(velocity(0) * velocity(0) + velocity(1) * velocity(1));

    if (speed < 150.f)
    {
        viewAngles(0) = 0.f;
        viewAngles(1) += 1.f;
        viewAngles(2) = 0.f;

        normalizeAngles(viewAngles);
        clampAngles(viewAngles);

        setViewAngles(viewAngles);
    }
}

void Aimbot::normalizeAngles(Vector3& angles)
{
    while (angles(0) > 89.f)
        angles(0) -= 180.f;

    while (angles(0) < -89.f)
        angles(0) += 180.f;

    while (angles(1) > 180.f)
        angles(1) -= 360.f;

    while (angles(1) < -180.f)
        angles(1) += 360.f;
}

void Aimbot::clampAngles(Vector3& angles)
{
    if (angles(0) > 89.0)
        angles(0) = 89.0;

    if (angles(0) < -89.0)
        angles(0) = -89.0;

    if (angles(1) > 180.0)
        angles(1) = 180.0;

    if (angles(1) < -180.0)
        angles(1) = -180.0;
}

Aimbot::Aimbot(hazedumper::BSPParser* bspParser)
{
    this->bspParser = bspParser;
    this->defaultSensitivity = getSensitivity();
}


Aimbot::Aimbot()
{
    this->defaultSensitivity = getSensitivity();
}

Aimbot::~Aimbot()
{

}
