#pragma once
#include "..\common\bsp\BSPParser.hpp"
#include "data.hpp"
#include "Entity.hpp"
#include "offsets.hpp"
#include "Driver.h"

class Aimbot
{
private:
    hazedumper::BSPParser* bspParser;
    float defaultSensitivity;
    Entity mYaw();
    Entity findClosestEnemyToFOV();
    Entity findClosestEnemyToFOV2();
    Vector3 angleDifferenceToEntity(Entity& localPlayer, Entity& entity);
    Vector3 getViewAngles();
    void normalizeAngles(Vector3& angles);
    void clampAngles(Vector3& angles);
    void setViewAngles(Vector3& viewAngles);
    float getSensitivity();
    bool enemyIsInCrossHair();
    const char* getMapDirectory();
    const char* getGameDirectory();
public:
    bool aimBot();
    Entity localPlayer;
    Vector3 aimAnglesTo(Vector3& entity);
    bool aimAssist();
    bool CrosshairEsp();
    void walkBot();
    void TriggerBot();
    bool EnemyIsInCrossHair();
    Aimbot(hazedumper::BSPParser* bspParser);
    Aimbot();
    ~Aimbot();
};
