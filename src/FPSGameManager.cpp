#include "FPSGameManager.h"
#include "UIManager.h"
#include "ECS/Coordinator.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/RenderComponent.h"
#include "ECS/Components/PlayerComponent.h"
#include "ECS/Components/WeaponComponent.h"
#include "ECS/Components/ProjectileComponent.h"
#include "ECS/Components/EnemyComponent.h"
#include "ECS/Components/GameStateComponent.h"
#include "ECS/Systems/PlayerSystem.h"
#include "ECS/Systems/WeaponSystem.h"
#include "ECS/Systems/ProjectileSystem.h"
#include "ECS/Systems/EnemyAISystem.h"
#include "ECS/Systems/GameStateSystem.h"
#include "InputManager.h"
#include "Camera.h"
#include "PhysicsSystem.h"
#include "Mesh.h"
#include "Material.h"
#include "ShapeGenerator.h"
#include <iostream>
#include <random>

extern Coordinator gCoordinator;

FPSGameManager::FPSGameManager()
    : m_inputManager(nullptr), m_camera(nullptr), m_uiManager(nullptr)
    , m_physicsSystem(nullptr), m_initialized(false), m_gameStarted(false)
{}

FPSGameManager::~FPSGameManager() { Shutdown(); }

bool FPSGameManager::Initialize(InputManager* inputManager, Camera* camera, UIManager* uiManager, PhysicsSystem* physicsSystem) {
    if (m_initialized) return true;
    m_inputManager = inputManager; m_camera = camera; m_uiManager = uiManager; m_physicsSystem = physicsSystem;
    if (!m_inputManager || !m_camera || !m_uiManager || !m_physicsSystem) {
        std::cerr << "FPSGameManager: Missing required dependencies" << std::endl;
        return false;
    }
    try {
        gCoordinator.RegisterComponent<PlayerComponent>();
        gCoordinator.RegisterComponent<WeaponComponent>();
        gCoordinator.RegisterComponent<ProjectileComponent>();
        gCoordinator.RegisterComponent<EnemyComponent>();
        gCoordinator.RegisterComponent<GameStateComponent>();
    } catch (const std::exception& e) {
        std::cerr << "Error registering FPS components: " << e.what() << std::endl;
        return false;
    }
    RegisterFPSSystems();
    SetupSystemDependencies();
    m_initialized = true;
    AddConsoleMessage("FPS Game Manager initialized successfully");
    return true;
}

void FPSGameManager::Shutdown() {
    if (!m_initialized) return;
    CleanupGameEntities();
    m_playerSystem.reset(); m_weaponSystem.reset(); m_projectileSystem.reset();
    m_enemySystem.reset(); m_gameStateSystem.reset();
    m_initialized = false; m_gameStarted = false;
}

void FPSGameManager::RegisterFPSSystems() {
    try {
        m_playerSystem = gCoordinator.RegisterSystem<PlayerSystem>();
        m_weaponSystem = gCoordinator.RegisterSystem<WeaponSystem>();
        m_projectileSystem = gCoordinator.RegisterSystem<ProjectileSystem>();
        m_enemySystem = gCoordinator.RegisterSystem<EnemyAISystem>();
        m_gameStateSystem = gCoordinator.RegisterSystem<GameStateSystem>();

        Signature sig;
        sig.set(gCoordinator.GetComponentType<TransformComponent>());
        sig.set(gCoordinator.GetComponentType<PlayerComponent>());
        gCoordinator.SetSystemSignature<PlayerSystem>(sig);

        sig.reset();
        sig.set(gCoordinator.GetComponentType<TransformComponent>());
        sig.set(gCoordinator.GetComponentType<WeaponComponent>());
        gCoordinator.SetSystemSignature<WeaponSystem>(sig);

        sig.reset();
        sig.set(gCoordinator.GetComponentType<TransformComponent>());
        sig.set(gCoordinator.GetComponentType<ProjectileComponent>());
        gCoordinator.SetSystemSignature<ProjectileSystem>(sig);

        sig.reset();
        sig.set(gCoordinator.GetComponentType<TransformComponent>());
        sig.set(gCoordinator.GetComponentType<EnemyComponent>());
        gCoordinator.SetSystemSignature<EnemyAISystem>(sig);

        sig.reset();
        sig.set(gCoordinator.GetComponentType<GameStateComponent>());
        gCoordinator.SetSystemSignature<GameStateSystem>(sig);
    } catch (const std::exception& e) {
        std::cerr << "Error registering FPS systems: " << e.what() << std::endl;
    }
}

void FPSGameManager::SetupSystemDependencies() {
    try {
        m_playerSystem->Init(m_camera, m_inputManager);
        m_weaponSystem->Init(m_playerSystem.get(), m_camera);
        m_projectileSystem->Init(m_enemySystem.get(), m_playerSystem.get());
        m_enemySystem->Init(m_playerSystem.get(), m_weaponSystem.get());
        m_gameStateSystem->Init(m_playerSystem.get(), m_enemySystem.get(), m_uiManager);
    } catch (const std::exception& e) {
        std::cerr << "Error setting up system dependencies: " << e.what() << std::endl;
    }
}

// --- Visible Geometry Helpers ---

Entity FPSGameManager::CreateVisibleCube(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color) {
    Entity entity = gCoordinator.CreateEntity();
    TransformComponent tc; tc.position = pos; tc.scale = scale;
    gCoordinator.AddComponent(entity, tc);

    std::vector<Vertex> verts; std::vector<unsigned int> idx;
    generateCubeMesh(verts, idx);
    Mesh* mesh = new Mesh(verts, idx, std::vector<Texture>{});
    mesh->pbrMaterial = std::make_shared<PBRMaterial>();
    mesh->pbrMaterial->albedo = color;
    mesh->pbrMaterial->roughness = 0.8f;

    RenderComponent rc; rc.renderable = mesh; rc.visible = true;
    gCoordinator.AddComponent(entity, rc);
    TrackEntity(entity);
    return entity;
}

Entity FPSGameManager::CreateVisibleSphere(const glm::vec3& pos, float radius, const glm::vec3& color) {
    Entity entity = gCoordinator.CreateEntity();
    TransformComponent tc; tc.position = pos; tc.scale = glm::vec3(radius);
    gCoordinator.AddComponent(entity, tc);

    std::vector<Vertex> verts; std::vector<unsigned int> idx;
    generateSphereMesh(verts, idx, 1.0f, 16, 12);
    Mesh* mesh = new Mesh(verts, idx, std::vector<Texture>{});
    mesh->pbrMaterial = std::make_shared<PBRMaterial>();
    mesh->pbrMaterial->albedo = color;
    mesh->pbrMaterial->emissive = color * 0.5f;

    RenderComponent rc; rc.renderable = mesh; rc.visible = true;
    gCoordinator.AddComponent(entity, rc);
    TrackEntity(entity);
    return entity;
}

Entity FPSGameManager::CreateVisiblePlane(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color) {
    Entity entity = gCoordinator.CreateEntity();
    TransformComponent tc; tc.position = pos; tc.scale = scale;
    gCoordinator.AddComponent(entity, tc);

    std::vector<Vertex> verts; std::vector<unsigned int> idx;
    generatePlaneMesh(verts, idx);
    Mesh* mesh = new Mesh(verts, idx, std::vector<Texture>{});
    mesh->pbrMaterial = std::make_shared<PBRMaterial>();
    mesh->pbrMaterial->albedo = color;
    mesh->pbrMaterial->roughness = 0.9f;

    RenderComponent rc; rc.renderable = mesh; rc.visible = true;
    gCoordinator.AddComponent(entity, rc);
    TrackEntity(entity);
    return entity;
}

void FPSGameManager::TrackEntity(Entity e) { m_gameEntities.push_back(e); }

void FPSGameManager::CleanupGameEntities() {
    for (auto e : m_gameEntities) { try { gCoordinator.DestroyEntity(e); } catch (...) {} }
    m_gameEntities.clear(); m_pickupEntities.clear(); m_levelData.rooms.clear();
}

// --- DOOM Level Generation ---

void FPSGameManager::GenerateDoomLevel() {
    AddConsoleMessage("Generating DOOM level with 6 rooms...");
    m_levelData.rooms.clear();
    m_levelData.playerSpawnPosition = glm::vec3(0.0f, 0.0f, 3.0f);

    // Room 0: Spawn Hall
    { Room r; r.center = glm::vec3(0,0,0); r.width=12; r.height=3.5f; r.depth=10;
      r.state = RoomState::ACTIVE; r.doors.push_back({glm::vec3(0,0,-5), 1});
      m_levelData.rooms.push_back(r); }
    // Room 1: Armory
    { Room r; r.center = glm::vec3(0,0,-20); r.width=14; r.height=3.5f; r.depth=12;
      r.doors.push_back({glm::vec3(0,0,-14), -1}); r.doors.push_back({glm::vec3(0,0,-26), 2});
      m_levelData.rooms.push_back(r); }
    // Room 2: Crossroads
    { Room r; r.center = glm::vec3(0,0,-38); r.width=16; r.height=4.0f; r.depth=16;
      r.doors.push_back({glm::vec3(0,0,-30), -1}); r.doors.push_back({glm::vec3(-8,0,-38), 3});
      r.doors.push_back({glm::vec3(8,0,-38), 4});
      m_levelData.rooms.push_back(r); }
    // Room 3: Arena
    { Room r; r.center = glm::vec3(-20,0,-55); r.width=14; r.height=4.0f; r.depth=14;
      r.doors.push_back({glm::vec3(-13,0,-55), -1}); r.doors.push_back({glm::vec3(-20,0,-62), 5});
      m_levelData.rooms.push_back(r); }
    // Room 4: Sniper Nest
    { Room r; r.center = glm::vec3(20,0,-55); r.width=12; r.height=5.0f; r.depth=12;
      r.doors.push_back({glm::vec3(13,0,-55), -1}); r.doors.push_back({glm::vec3(20,0,-62), 5});
      m_levelData.rooms.push_back(r); }
    // Room 5: Boss Arena
    { Room r; r.center = glm::vec3(0,0,-80); r.width=20; r.height=5.0f; r.depth=20;
      r.doors.push_back({glm::vec3(-10,0,-70), -1}); r.doors.push_back({glm::vec3(10,0,-70), -1});
      m_levelData.rooms.push_back(r); }

    for (int i = 0; i < (int)m_levelData.rooms.size(); i++) BuildRoom(m_levelData.rooms[i], i);
    BuildCorridor(m_levelData.rooms[0], m_levelData.rooms[1]);
    BuildCorridor(m_levelData.rooms[1], m_levelData.rooms[2]);
    BuildCorridor(m_levelData.rooms[2], m_levelData.rooms[3]);
    BuildCorridor(m_levelData.rooms[2], m_levelData.rooms[4]);
    BuildCorridor(m_levelData.rooms[3], m_levelData.rooms[5]);
    BuildCorridor(m_levelData.rooms[4], m_levelData.rooms[5]);
    SpawnRoomEnemies(0);
    AddConsoleMessage("DOOM level generated: 6 rooms, corridors, enemies in Room 0");
}

void FPSGameManager::BuildRoom(Room& room, int roomIndex) {
    float hw = room.width*0.5f, hd = room.depth*0.5f;
    float cx = room.center.x, cz = room.center.z, h = room.height;
    glm::vec3 floorC(0.15f,0.15f,0.18f), ceilC(0.1f,0.1f,0.12f), wallC(0.25f,0.22f,0.2f);

    CreateVisibleCube(glm::vec3(cx,-0.1f,cz), glm::vec3(room.width,0.2f,room.depth), floorC);
    CreateVisibleCube(glm::vec3(cx,h,cz), glm::vec3(room.width,0.2f,room.depth), ceilC);

    auto buildWall = [&](float fixedCoord, float start, float end, bool isXWall, bool checkX) {
        bool hasDoor = false; float doorPos = 0;
        for (auto& [dpos, _] : room.doors) {
            float dCheck = checkX ? dpos.x : dpos.z;
            float dFixed = checkX ? dpos.z : dpos.x;
            (void)dCheck;
            if (std::abs((checkX ? dpos.z : dpos.x) - fixedCoord) < 1.0f) {
                hasDoor = true; doorPos = checkX ? dpos.x : dpos.z; break;
            }
        }
        if (isXWall) {
            if (hasDoor) BuildWallWithGap(glm::vec3(start,0,fixedCoord), glm::vec3(end,0,fixedCoord), h*0.5f, h, 0.3f, doorPos, 2.5f, wallC, true);
            else BuildSolidWall(glm::vec3(start,0,fixedCoord), glm::vec3(end,0,fixedCoord), h*0.5f, h, 0.3f, wallC, true);
        } else {
            if (hasDoor) BuildWallWithGap(glm::vec3(fixedCoord,0,start), glm::vec3(fixedCoord,0,end), h*0.5f, h, 0.3f, doorPos, 2.5f, wallC, false);
            else BuildSolidWall(glm::vec3(fixedCoord,0,start), glm::vec3(fixedCoord,0,end), h*0.5f, h, 0.3f, wallC, false);
        }
    };

    buildWall(cz+hd, cx-hw, cx+hw, true, true);   // North
    buildWall(cz-hd, cx-hw, cx+hw, true, true);   // South
    buildWall(cx-hw, cz-hd, cz+hd, false, false); // West
    buildWall(cx+hw, cz-hd, cz+hd, false, false); // East

    if (roomIndex == 2 || roomIndex == 3 || roomIndex == 5) {
        float pOff = std::min(hw,hd)*0.4f;
        for (auto& pp : {glm::vec3(cx-pOff,h*0.5f,cz-pOff), glm::vec3(cx+pOff,h*0.5f,cz-pOff),
                         glm::vec3(cx-pOff,h*0.5f,cz+pOff), glm::vec3(cx+pOff,h*0.5f,cz+pOff)})
            CreateVisibleCube(pp, glm::vec3(0.6f,h,0.6f), glm::vec3(0.3f,0.28f,0.25f));
    }

    std::mt19937 rng(roomIndex*42+7);
    std::uniform_real_distribution<float> dX(cx-hw*0.6f, cx+hw*0.6f), dZ(cz-hd*0.6f, cz+hd*0.6f);
    for (int i = 0; i < 2+(roomIndex%3); i++)
        CreateVisibleCube(glm::vec3(dX(rng),0.4f,dZ(rng)), glm::vec3(1.0f,0.8f,1.0f), glm::vec3(0.35f,0.25f,0.15f));

    if (roomIndex == 4) {
        CreateVisibleCube(glm::vec3(cx-3,0.75f,cz-3), glm::vec3(3,1.5f,3), glm::vec3(0.2f,0.18f,0.16f));
        CreateVisibleCube(glm::vec3(cx+3,0.75f,cz+3), glm::vec3(3,1.5f,3), glm::vec3(0.2f,0.18f,0.16f));
    }
}

void FPSGameManager::BuildWallWithGap(const glm::vec3& wallStart, const glm::vec3& wallEnd,
    float wallY, float wallHeight, float thickness, float gapCenter, float gapWidth, const glm::vec3& color, bool alongX) {
    float halfGap = gapWidth*0.5f;
    if (alongX) {
        float z = wallStart.z;
        if (gapCenter-halfGap > wallStart.x+0.1f)
            CreateVisibleCube(glm::vec3((wallStart.x+gapCenter-halfGap)*0.5f, wallY, z), glm::vec3(gapCenter-halfGap-wallStart.x, wallHeight, thickness), color);
        if (wallEnd.x > gapCenter+halfGap+0.1f)
            CreateVisibleCube(glm::vec3((gapCenter+halfGap+wallEnd.x)*0.5f, wallY, z), glm::vec3(wallEnd.x-gapCenter-halfGap, wallHeight, thickness), color);
    } else {
        float x = wallStart.x;
        if (gapCenter-halfGap > wallStart.z+0.1f)
            CreateVisibleCube(glm::vec3(x, wallY, (wallStart.z+gapCenter-halfGap)*0.5f), glm::vec3(thickness, wallHeight, gapCenter-halfGap-wallStart.z), color);
        if (wallEnd.z > gapCenter+halfGap+0.1f)
            CreateVisibleCube(glm::vec3(x, wallY, (gapCenter+halfGap+wallEnd.z)*0.5f), glm::vec3(thickness, wallHeight, wallEnd.z-gapCenter-halfGap), color);
    }
}

void FPSGameManager::BuildSolidWall(const glm::vec3& start, const glm::vec3& end,
    float wallY, float wallHeight, float thickness, const glm::vec3& color, bool alongX) {
    if (alongX) CreateVisibleCube(glm::vec3((start.x+end.x)*0.5f, wallY, start.z), glm::vec3(std::abs(end.x-start.x), wallHeight, thickness), color);
    else CreateVisibleCube(glm::vec3(start.x, wallY, (start.z+end.z)*0.5f), glm::vec3(thickness, wallHeight, std::abs(end.z-start.z)), color);
}

void FPSGameManager::BuildCorridor(const Room& from, const Room& to, float width, float height) {
    glm::vec3 corC(0.18f,0.16f,0.15f), flC(0.12f,0.12f,0.14f);
    glm::vec3 dF = from.center, dT = to.center;
    for (auto& [dp,_] : from.doors) if (glm::length(dp-to.center) < glm::length(dF-to.center)) dF = dp;
    for (auto& [dp,_] : to.doors)   if (glm::length(dp-from.center) < glm::length(dT-from.center)) dT = dp;
    float halfW = width*0.5f, halfH = height*0.5f;

    if (std::abs(dT.z-dF.z) >= std::abs(dT.x-dF.x)) {
        float z1=std::min(dF.z,dT.z), z2=std::max(dF.z,dT.z), len=z2-z1;
        float mZ=(z1+z2)*0.5f, mX=(dF.x+dT.x)*0.5f;
        CreateVisibleCube(glm::vec3(mX,-0.1f,mZ), glm::vec3(width,0.2f,len), flC);
        CreateVisibleCube(glm::vec3(mX,height,mZ), glm::vec3(width,0.2f,len), glm::vec3(0.08f,0.08f,0.1f));
        CreateVisibleCube(glm::vec3(mX-halfW,halfH,mZ), glm::vec3(0.3f,height,len), corC);
        CreateVisibleCube(glm::vec3(mX+halfW,halfH,mZ), glm::vec3(0.3f,height,len), corC);
    } else {
        float x1=std::min(dF.x,dT.x), x2=std::max(dF.x,dT.x), len=x2-x1;
        float mX=(x1+x2)*0.5f, mZ=(dF.z+dT.z)*0.5f;
        CreateVisibleCube(glm::vec3(mX,-0.1f,mZ), glm::vec3(len,0.2f,width), flC);
        CreateVisibleCube(glm::vec3(mX,height,mZ), glm::vec3(len,0.2f,width), glm::vec3(0.08f,0.08f,0.1f));
        CreateVisibleCube(glm::vec3(mX,halfH,mZ-halfW), glm::vec3(len,height,0.3f), corC);
        CreateVisibleCube(glm::vec3(mX,halfH,mZ+halfW), glm::vec3(len,height,0.3f), corC);
    }
}

// --- Unified Enemy Creation ---

Entity FPSGameManager::CreateUnifiedEnemy(EnemyType type, const glm::vec3& position, int roomIndex) {
    Entity enemy = gCoordinator.CreateEntity();
    glm::vec3 scale, color; float hp, dmg; int scoreVal;
    switch (type) {
        case EnemyType::GRUNT:   scale={0.8f,1.8f,0.6f}; color={0.8f,0.2f,0.15f}; hp=50; dmg=15; scoreVal=10; break;
        case EnemyType::SOLDIER: scale={1.0f,1.8f,0.8f}; color={0.2f,0.3f,0.8f};  hp=75; dmg=25; scoreVal=20; break;
        case EnemyType::HEAVY:   scale={1.4f,2.0f,1.2f}; color={0.15f,0.7f,0.2f}; hp=150; dmg=40; scoreVal=50; break;
        case EnemyType::SNIPER:  scale={0.7f,2.2f,0.5f}; color={0.8f,0.7f,0.1f};  hp=40; dmg=60; scoreVal=30; break;
    }

    TransformComponent tc; tc.position = position; tc.scale = scale;
    gCoordinator.AddComponent(enemy, tc);

    EnemyComponent ec; ec.type = type; ec.health = hp; ec.maxHealth = hp;
    ec.damage = dmg; ec.scoreValue = scoreVal; ec.state = EnemyState::PATROL; ec.isAlive = true;
    Room& room = m_levelData.rooms[roomIndex];
    float pr = std::min(room.width, room.depth) * 0.3f;
    ec.patrolPoints = {
        glm::vec3(position.x-pr*0.5f, position.y, position.z-pr*0.5f),
        glm::vec3(position.x+pr*0.5f, position.y, position.z-pr*0.5f),
        glm::vec3(position.x+pr*0.5f, position.y, position.z+pr*0.5f),
        glm::vec3(position.x-pr*0.5f, position.y, position.z+pr*0.5f)
    };
    gCoordinator.AddComponent(enemy, ec);

    std::vector<Vertex> verts; std::vector<unsigned int> idx;
    generateCubeMesh(verts, idx);
    Mesh* mesh = new Mesh(verts, idx, std::vector<Texture>{});
    mesh->pbrMaterial = std::make_shared<PBRMaterial>();
    mesh->pbrMaterial->albedo = color; mesh->pbrMaterial->roughness = 0.6f;
    RenderComponent rc; rc.renderable = mesh; rc.visible = true;
    gCoordinator.AddComponent(enemy, rc);

    if (m_weaponSystem) {
        WeaponType wt = WeaponType::PISTOL;
        switch (type) {
            case EnemyType::GRUNT: wt=WeaponType::PISTOL; break; case EnemyType::SOLDIER: wt=WeaponType::RIFLE; break;
            case EnemyType::HEAVY: wt=WeaponType::SHOTGUN; break; case EnemyType::SNIPER: wt=WeaponType::SNIPER; break;
        }
        Entity weapon = m_weaponSystem->CreateWeapon(wt, position);
        m_weaponSystem->EquipWeapon(weapon, enemy); TrackEntity(weapon);
    }
    TrackEntity(enemy);
    return enemy;
}

void FPSGameManager::SpawnRoomEnemies(int roomIndex) {
    Room& room = m_levelData.rooms[roomIndex];
    float cx = room.center.x, cz = room.center.z;
    auto spawn = [&](EnemyType t, float ox, float oz) {
        room.spawnedEnemyIds.push_back(CreateUnifiedEnemy(t, glm::vec3(cx+ox, 1.0f, cz+oz), roomIndex));
    };
    switch (roomIndex) {
        case 0: spawn(EnemyType::GRUNT,-2,-2); spawn(EnemyType::GRUNT,2,-2); break;
        case 1: spawn(EnemyType::GRUNT,-3,0); spawn(EnemyType::GRUNT,3,0); spawn(EnemyType::GRUNT,0,-3); spawn(EnemyType::SOLDIER,0,3); break;
        case 2: spawn(EnemyType::SOLDIER,-3,-3); spawn(EnemyType::SOLDIER,3,3); spawn(EnemyType::HEAVY,0,0); break;
        case 3: spawn(EnemyType::HEAVY,-3,0); spawn(EnemyType::HEAVY,3,0); spawn(EnemyType::SOLDIER,0,-3); spawn(EnemyType::SOLDIER,0,3); break;
        case 4: spawn(EnemyType::SNIPER,-3,-3); spawn(EnemyType::SNIPER,3,3); spawn(EnemyType::GRUNT,0,0); break;
        case 5: spawn(EnemyType::HEAVY,0,0); spawn(EnemyType::SOLDIER,-5,-5); spawn(EnemyType::SOLDIER,5,5); spawn(EnemyType::SNIPER,-5,5); spawn(EnemyType::SNIPER,5,-5); break;
    }
    AddConsoleMessage("Room " + std::to_string(roomIndex) + ": " + std::to_string(room.spawnedEnemyIds.size()) + " enemies spawned");
}

// --- Pickups ---

Entity FPSGameManager::CreatePickup(PickupType type, const glm::vec3& position) {
    glm::vec3 color;
    switch (type) {
        case PickupType::HEALTH: color={0,1,0.3f}; break; case PickupType::AMMO: color={1,0.8f,0}; break;
        case PickupType::WEAPON_SHOTGUN: color={1,0.5f,0}; break; case PickupType::WEAPON_RIFLE: color={0.3f,0.5f,1}; break;
        case PickupType::WEAPON_SNIPER: color={0.7f,0.3f,1}; break;
    }
    Entity entity = CreateVisibleSphere(glm::vec3(position.x, 0.5f, position.z), 0.4f, color);
    m_pickupEntities.push_back({entity, type});
    return entity;
}

void FPSGameManager::CheckPickupCollisions() {
    if (!m_camera) return;
    glm::vec3 playerPos = m_camera->Position; playerPos.y = 0;
    for (auto it = m_pickupEntities.begin(); it != m_pickupEntities.end(); ) {
        try {
            auto& tc = gCoordinator.GetComponent<TransformComponent>(it->first);
            auto& rc = gCoordinator.GetComponent<RenderComponent>(it->first);
            if (!rc.visible) { ++it; continue; }
            glm::vec3 pp = tc.position; pp.y = 0;
            if (glm::length(playerPos - pp) < 1.5f) {
                switch (it->second) {
                    case PickupType::HEALTH:
                        if (m_playerSystem && m_playerSystem->HasPlayer()) {
                            auto& pc = gCoordinator.GetComponent<PlayerComponent>(m_playerSystem->GetPlayer());
                            pc.health = std::min(pc.health + 25.0f, pc.maxHealth);
                        } break;
                    case PickupType::AMMO:
                        if (m_playerSystem && m_playerSystem->HasPlayer()) {
                            auto& pc = gCoordinator.GetComponent<PlayerComponent>(m_playerSystem->GetPlayer());
                            if (!pc.weapons.empty()) { auto& wc = gCoordinator.GetComponent<WeaponComponent>(static_cast<Entity>(pc.weapons[pc.currentWeapon])); wc.reserveAmmo += 15; }
                        } break;
                    case PickupType::WEAPON_SHOTGUN: GivePlayerWeapon(2); break;
                    case PickupType::WEAPON_RIFLE: GivePlayerWeapon(1); break;
                    case PickupType::WEAPON_SNIPER: GivePlayerWeapon(3); break;
                }
                rc.visible = false; it = m_pickupEntities.erase(it);
            } else ++it;
        } catch (...) { it = m_pickupEntities.erase(it); }
    }
}

// --- Room Progression ---

void FPSGameManager::CheckRoomTransitions() {
    if (!m_camera) return;
    glm::vec3 playerPos = m_camera->Position;
    for (int i = 0; i < (int)m_levelData.rooms.size(); i++) {
        Room& room = m_levelData.rooms[i];
        bool inRoom = std::abs(playerPos.x - room.center.x) < room.width*0.5f+1 &&
                      std::abs(playerPos.z - room.center.z) < room.depth*0.5f+1;
        if (room.state == RoomState::LOCKED && inRoom) {
            room.state = RoomState::ACTIVE; SpawnRoomEnemies(i);
            AddConsoleMessage("Room " + std::to_string(i) + " ACTIVATED!");
        }
        if (room.state == RoomState::ACTIVE) {
            int alive = 0;
            for (Entity eid : room.spawnedEnemyIds) { try { if (gCoordinator.GetComponent<EnemyComponent>(eid).isAlive) alive++; } catch (...) {} }
            if (alive == 0 && !room.spawnedEnemyIds.empty()) {
                room.state = RoomState::CLEARED;
                AddConsoleMessage("Room " + std::to_string(i) + " CLEARED!");
                switch (i) {
                    case 1: CreatePickup(PickupType::WEAPON_SHOTGUN, room.center); break;
                    case 2: CreatePickup(PickupType::WEAPON_RIFLE, room.center); break;
                    case 4: CreatePickup(PickupType::WEAPON_SNIPER, room.center); break;
                    case 5: CreatePickup(PickupType::HEALTH, room.center);
                        if (m_gameStateSystem) { m_gameStateSystem->EndGame(true); AddConsoleMessage("=== VICTORY! ==="); } break;
                    default: CreatePickup(PickupType::AMMO, room.center); break;
                }
            }
        }
    }
}

// --- Game Flow ---

void FPSGameManager::StartNewGame() {
    if (!m_initialized) return;
    AddConsoleMessage("=== STARTING DOOM-STYLE FPS GAME ===");
    try {
        m_gameStarted = true;
        if (m_camera) {
            m_savedCameraPos = m_camera->Position; m_savedCameraYaw = m_camera->Yaw;
            m_savedCameraPitch = m_camera->Pitch; m_savedOrbitMode = m_camera->OrbitMode;
            m_camera->SetOrbitMode(false);
            m_camera->Position = m_levelData.playerSpawnPosition + glm::vec3(0,1.7f,0);
            m_camera->Yaw = -90.0f; m_camera->Pitch = 0.0f; m_camera->updateCameraVectors();
        }
        if (m_inputManager) { m_inputManager->EnableSceneEditorMode(false); m_inputManager->SetInputContext(InputContext::GAME_PLAY); }
        GenerateDoomLevel();
        if (m_playerSystem) { Entity p = m_playerSystem->CreatePlayer(m_levelData.playerSpawnPosition); TrackEntity(p); }
        if (m_playerSystem && m_playerSystem->HasPlayer() && m_weaponSystem) GivePlayerWeapon(0);
        AddConsoleMessage("=== DOOM GAME STARTED — WASD + mouse + left-click ===");
    } catch (const std::exception& e) { AddConsoleMessage("CRASH PREVENTED: " + std::string(e.what())); m_gameStarted = false; }
      catch (...) { AddConsoleMessage("UNKNOWN CRASH PREVENTED"); m_gameStarted = false; }
}

void FPSGameManager::StopGame() {
    if (!m_gameStarted) return;
    CleanupGameEntities();
    if (m_camera) { m_camera->Position = m_savedCameraPos; m_camera->Yaw = m_savedCameraYaw;
        m_camera->Pitch = m_savedCameraPitch; m_camera->updateCameraVectors(); m_camera->SetOrbitMode(m_savedOrbitMode); }
    if (m_inputManager) { m_inputManager->EnableSceneEditorMode(true); m_inputManager->SetInputContext(InputContext::SCENE_EDITOR); }
    m_gameStarted = false;
    AddConsoleMessage("=== FPS GAME STOPPED, EDITOR RESTORED ===");
}

void FPSGameManager::PauseGame() { if (m_gameStateSystem && IsGameActive()) { m_gameStateSystem->PauseGame(); AddConsoleMessage("Game paused"); } }
void FPSGameManager::ResumeGame() { if (m_gameStateSystem && IsGamePaused()) { m_gameStateSystem->ResumeGame(); AddConsoleMessage("Game resumed"); } }
void FPSGameManager::RestartGame() { StopGame(); StartNewGame(); }
void FPSGameManager::QuitGame() { StopGame(); }

// --- Game Loop ---

void FPSGameManager::Update(float deltaTime) {
    if (!m_initialized) return;
    try {
        if (m_playerSystem) m_playerSystem->Update(deltaTime);
        if (m_weaponSystem) m_weaponSystem->Update(deltaTime);
        if (m_projectileSystem) m_projectileSystem->Update(deltaTime);
        if (m_enemySystem) m_enemySystem->Update(deltaTime);
        if (m_gameStateSystem) m_gameStateSystem->Update(deltaTime);
    } catch (const std::exception& e) { AddConsoleMessage("Error updating: " + std::string(e.what())); }

    if (m_gameStarted && m_inputManager) {
        try {
            if (m_inputManager->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) HandleShooting();
            static bool escWas = false; bool escIs = m_inputManager->IsKeyPressed(GLFW_KEY_ESCAPE);
            if (escIs && !escWas) TogglePause(); escWas = escIs;
        } catch (...) {}
        CheckRoomTransitions(); CheckPickupCollisions();
    }
}

void FPSGameManager::HandleShooting() {
    if (!m_camera || !m_weaponSystem || !m_playerSystem || !m_playerSystem->HasPlayer()) return;
    try {
        auto& pc = gCoordinator.GetComponent<PlayerComponent>(m_playerSystem->GetPlayer());
        if (!pc.weapons.empty()) {
            Entity wEnt = static_cast<Entity>(pc.weapons[pc.currentWeapon]);
            if (m_weaponSystem->CanShoot(wEnt)) m_weaponSystem->Shoot(wEnt, m_camera->Position, m_camera->Front);
        }
    } catch (...) {}
}

// --- Queries ---

bool FPSGameManager::IsGameActive() const { return m_gameStarted && m_initialized; }
bool FPSGameManager::IsGamePaused() const { return m_gameStateSystem ? m_gameStateSystem->IsGamePaused() : false; }
void FPSGameManager::TogglePause() { if (IsGamePaused()) ResumeGame(); else if (IsGameActive()) PauseGame(); }

HUDData FPSGameManager::GetHUDData() const {
    HUDData data;
    if (m_playerSystem && m_playerSystem->HasPlayer()) {
        try {
            auto& pc = gCoordinator.GetComponent<PlayerComponent>(m_playerSystem->GetPlayer());
            data.health = pc.health; data.maxHealth = pc.maxHealth; data.score = pc.score; data.kills = pc.kills;
            if (!pc.weapons.empty()) {
                auto& wc = gCoordinator.GetComponent<WeaponComponent>(static_cast<Entity>(pc.weapons[pc.currentWeapon]));
                data.ammo = wc.currentAmmo; data.maxAmmo = wc.maxAmmo; data.reserveAmmo = wc.reserveAmmo;
                data.weaponName = wc.name; data.isReloading = wc.isReloading;
                data.reloadProgress = wc.isReloading ? (wc.currentReloadTime / wc.reloadTime) : 0.0f;
            }
        } catch (...) {}
    }
    if (m_enemySystem) data.aliveEnemies = m_enemySystem->GetAliveEnemyCount();
    for (int i = 0; i < (int)m_levelData.rooms.size(); i++) if (m_levelData.rooms[i].state == RoomState::ACTIVE) data.currentRoom = i;
    return data;
}

// --- Debug ---

void FPSGameManager::SpawnTestEnemy(const glm::vec3& position) { if (m_enemySystem) m_enemySystem->CreateGrunt(position); }

void FPSGameManager::GivePlayerWeapon(int weaponType) {
    if (!m_playerSystem || !m_weaponSystem || !m_playerSystem->HasPlayer()) return;
    Entity player = m_playerSystem->GetPlayer();
    auto& pt = gCoordinator.GetComponent<TransformComponent>(player);
    Entity weapon;
    switch (weaponType) {
        case 0: weapon = m_weaponSystem->CreatePistol(pt.position); break;
        case 1: weapon = m_weaponSystem->CreateRifle(pt.position); break;
        case 2: weapon = m_weaponSystem->CreateWeapon(WeaponType::SHOTGUN, pt.position); break;
        case 3: weapon = m_weaponSystem->CreateWeapon(WeaponType::SNIPER, pt.position); break;
        default: return;
    }
    m_weaponSystem->EquipWeapon(weapon, player); TrackEntity(weapon);
}

void FPSGameManager::PrintGameStats() {
    if (!m_initialized) return;
    if (m_enemySystem) AddConsoleMessage("Alive enemies: " + std::to_string(m_enemySystem->GetAliveEnemyCount()));
    if (m_playerSystem && m_playerSystem->HasPlayer()) {
        try { auto& pc = gCoordinator.GetComponent<PlayerComponent>(m_playerSystem->GetPlayer());
            AddConsoleMessage("HP:" + std::to_string(pc.health) + " Score:" + std::to_string(pc.score)); } catch (...) {}
    }
}

void FPSGameManager::AddConsoleMessage(const std::string& message) { std::cout << "[FPS] " << message << std::endl; }
