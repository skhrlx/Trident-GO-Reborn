#pragma warning (disable : 26451)

#include "Entity.hpp"
#include <iostream>
#include <time.h>
#include "..\common\bsp\BSPStructure.hpp"
#include "csgo_settings.hpp"
#include "offsets.hpp"
#include "sdk.hpp"
#include "Arduino.hpp"

// hazedumper namespace
using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

bool Entity::IsDormant()
{
    bool isDormant = Driver::read<bool>(PID, EntityAddress + m_bDormant);
    return isDormant;
}

bool Entity::IsDefusing()
{
    bool Defusing = Driver::read<bool>(PID, EntityAddress + m_bIsDefusing);
    return Defusing;
}

uint8_t Entity::getTeam()
{
    uint8_t OurTeam = Driver::read<uint8_t>(PID, EntityAddress + m_iTeamNum);
    return OurTeam;
}

bool Entity::isInAir()
{
    uint32_t flags = Driver::read<uint32_t>(PID, EntityAddress + m_fFlags);
    return flags == 256 || flags == 258 || flags == 260 || flags == 262;
}

bool Entity::IsCrouching()
{
    uint32_t flags = Driver::read<uint32_t>(PID, EntityAddress + m_fFlags);
    return flags & FL_DUCKING;
}

uint8_t Entity::getHealth()
{
    uint8_t health = Driver::read<uint8_t>(PID, EntityAddress + m_iHealth);
    return health;
}

Vector3 Entity::getAimPunch()
{
    Vector3 aimPunch = Driver::read<Vector3>(PID, EntityAddress + m_aimPunchAngle);
    return aimPunch;
}

bool Entity::isValidPlayer()
{
    int health = getHealth();
    bool isDormant = IsDormant();

    return health > 0 && health <= 100 && !isDormant;
}

Vector3 Entity::getAbsolutePosition()
{
    Vector3 position = getFeetPosition();
    //position(2) += Driver::read<float>(PID, EntityAddress + 0x10c, sizeof(float));
    return position;
}

Vector3 Entity::getFeetPosition()
{
    Vector3 position = Driver::read<Vector3>(PID, EntityAddress + m_vecOrigin);
    return position;
}

Vector3 Entity::getVelocity()
{
    Vector3 vel = Driver::read<Vector3>(PID, EntityAddress + m_vecVelocity);
    return vel;
}

/*Vector3 Entity::getBonePosition(uint32_t boneId)
{
	int boneBase = Driver::read<int>(PID, EntityAddress + m_dwBoneMatrix, sizeof(int));

	Vector3 bonePosition;
	bonePosition(0) = Driver::read<float>(PID, boneBase + 0x30 * boneId + 0x0C, sizeof(float));
	bonePosition(1) = Driver::read<float>(PID, boneBase + 0x30 * boneId + 0x1C, sizeof(float));
	bonePosition(2) = Driver::read<float>(PID, boneBase + 0x30 * boneId + 0x2C, sizeof(float));

	return bonePosition;
}*/

Vector3 Entity::GetBonePosition(uint32_t targetBone)
{
    uint32_t boneMatrixAddress = Driver::read<uint32_t>(PID, EntityAddress + m_dwBoneMatrix);
    BoneMatrix boneMatrix = Driver::read<BoneMatrix>(PID, boneMatrixAddress + sizeof(BoneMatrix) * targetBone);
    return Vector3(boneMatrix.x, boneMatrix.y, boneMatrix.z);
}

void Entity::BuildBonePairs()
{
    // Update StudioHdr
    uint32_t studioHdr = Driver::read<uint32_t>(PID, EntityAddress + m_pStudioHdr);
    StudioHdrAddress = Driver::read<uint32_t>(PID, studioHdr);
    StudioHdrSt = Driver::read<StudioHdr>(PID, StudioHdrAddress);
    
    // Update and collect hitboxes
    StudioHitBoxSet = Driver::read<StudioHitboxSet>(PID, StudioHdrAddress + StudioHdrSt.hitboxsetindex);

    for (size_t i = 0; i < StudioHitBoxSet.numhitboxes; ++i)
    {
        StudioHitBoxes[i] = Driver::read<StudioBBox>(PID, StudioHdrAddress + StudioHdrSt.hitboxsetindex + StudioHitBoxSet.hitboxindex + i * sizeof(StudioBBox));
    }

    // Collect all StudioBones
    for (size_t i = 0; i < StudioHdrSt.numbones; ++i)
    {
        StudioBones[i] = Driver::read<StudioBone>(PID, StudioHdrAddress + StudioHdrSt.boneindex + i * sizeof(StudioBone));
    }

    // Collect all bone positions
    uint32_t boneMatrixAddress = Driver::read<uint32_t>(PID, EntityAddress + m_dwBoneMatrix);
    const int size = *(&BonePositions + 1) - BonePositions;
    
    for (size_t i = 0; i < size; ++i)
    {
        BoneMatrix boneMatrix = Driver::read<BoneMatrix>(PID, boneMatrixAddress + sizeof(BoneMatrix) * i);
        BonePositions[i] = Vector3(boneMatrix.x, boneMatrix.y, boneMatrix.z);
    }

    // Build bone pairs
    int pairs = 0;
    for (size_t i = 0; i < StudioHitBoxSet.numhitboxes; ++i)
    {
        const StudioBBox& hitbox = StudioHitBoxes[i];
        const StudioBone& bone = StudioBones[hitbox.bone];
        // Sanity check
        if (bone.parent >= 0 && bone.parent < StudioHdrSt.numbones)
        {
            BonePairs[pairs] = std::make_pair(hitbox.bone, bone.parent);
            pairs++;
        }
        CurrentBonePairs = pairs;
    }
}

RenderData Entity::getRenderData(uint8_t OurTeam, Vector3 screenPos, float inGameDistance)
{
    uint8_t ReadTeam = this->getTeam();
    // Green
    ImVec4 color = ImVec4(0, 1, 0, 1);
    if (OurTeam != ReadTeam)
    {
        // Red
        color = ImVec4(1, 0, 0, 1);
    }
    
    return RenderData
    {
        .x = screenPos.at(0),
        .y = screenPos.at(1),
        .inGameDistance = inGameDistance,
        .color = color
    };
}

Vector3 Entity::getHeadPosition()
{
    Vector3 Origin = Driver::read<Vector3>(PID, EntityAddress + m_vecOrigin);
    Vector3 ViewOffset = Driver::read<Vector3>(PID, EntityAddress + m_vecViewOffset);
    Vector3 LocalEyeOrigin = Origin + ViewOffset;
    if (this->IsCrouching())
    {
        LocalEyeOrigin(0) = LocalEyeOrigin(0) - 1.5f; // 0 for up and down
    }

    return LocalEyeOrigin;
}

uint16_t Entity::getCrosshairId()
{
    return Driver::read<uint16_t>(PID, EntityAddress + m_iCrosshairId);
}

uint8_t Entity::getForceAttack()
{
    return Driver::read<uint8_t>(PID, EntityAddress + dwForceAttack);
}

void Entity::shoot()
{
    if (csgo_settings::TriggerBot && csgo_settings::TriggerBotDelay)
    {
        srand(time(NULL));
        uint16_t rnd = rand() % (csgo_settings::TriggerBotDelayMax - csgo_settings::TriggerBotDelayMin + 1) +
            csgo_settings::TriggerBotDelayMin;
        Sleep(rnd);
    }

    Arduino::SendCommand(CMD_CLEFT);
}

uint16_t Entity::getShotsFired()
{
    return Driver::read<uint16_t>(PID, EntityAddress + m_iShotsFired);
}

uint32_t Entity::GetEntityAddress()
{
    return EntityAddress;
}

uint32_t Entity::GetGlowIndex()
{
    uint32_t GlowIndex = Driver::read<uint32_t>(PID, EntityAddress + m_iGlowIndex);
    return GlowIndex;
}

DWORD Entity::GetWeaponHandle()
{
    return Driver::read<DWORD>(PID, EntityAddress + m_hActiveWeapon);
}

uint16_t Entity::GetWeaponIndex()
{
    return GetWeaponHandle() & 0xFFF;
}

DWORD Entity::GetCurrentWeapon()
{
    return Driver::read<DWORD>(PID, ClientAddr + dwEntityList + (GetWeaponIndex() - 1) * 0x10);
}

uint16_t Entity::GetCurrentWeaponID()
{
    return Driver::read<uint16_t>(PID, GetCurrentWeapon() + m_iItemDefinitionIndex);
}

Entity::Entity(uint32_t EntityAddress)
{
    this->EntityAddress = EntityAddress;
}

Entity::Entity()
{
}

Entity::~Entity()
{
}
