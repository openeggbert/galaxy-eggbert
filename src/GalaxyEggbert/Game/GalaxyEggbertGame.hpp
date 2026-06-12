#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/def/GamePhase.hpp"
#include "Blupi.hpp"
#include "Camera.hpp"
#include "Decor.hpp"
#include "HUD.hpp"
#include "PhaseManager.hpp"
#include "GameData.hpp"
#include "SoundManager.hpp"
#include <memory>
#include <string>
#include <unordered_map>

// Main game coordinator: owns the scene, terrain, and subsystem objects.
// Delegates camera, HUD, and phase/overlay management to dedicated classes.
class GalaxyEggbertGame {
public:
    explicit GalaxyEggbertGame(Urho3D::Context* context);
    ~GalaxyEggbertGame();

    void Start();
    void Update(float dt);
    void Stop();

private:
    void CreateScene();
    void CreateTerrain();
    void BuildDemoWorld();
    void SpawnTerrainNodes();
    void CreateDemoObjects();

    void EnterPhase(GalaxyEggbert::GamePhase next);

    Urho3D::SharedPtr<Urho3D::Material> MakeFlatMaterial(
        const Urho3D::Color& color, float emissive = 0.12f);
    Urho3D::SharedPtr<Urho3D::Material> GetTileMaterial(uint16_t blockType);

    void UpdateInit(float dt);
    void UpdatePlay(float dt);
    void UpdatePause(float dt);
    void UpdateWin(float dt);
    void UpdateLost(float dt);
    void ResetLevel();

    // Scene
    Urho3D::SharedPtr<Urho3D::Scene> scene_;

    // World data + tile material cache
    std::unique_ptr<GalaxyEggbert::Worlds::World>  world_;
    std::unordered_map<uint16_t, Urho3D::SharedPtr<Urho3D::Material>> tileMatCache_;
    Urho3D::SharedPtr<Urho3D::Texture2D> objectSheet_;

    // Subsystems
    std::unique_ptr<Blupi>            blupi_;
    std::unique_ptr<Decor>            decor_;
    std::unique_ptr<CameraController> camera_;
    std::unique_ptr<HUD>              hud_;
    std::unique_ptr<PhaseManager>     phases_;
    std::unique_ptr<SoundManager>     sound_;

    // State
    bool        drawDebug_     = false;
    int         lives_         = 3;
    int         prevCollected_ = 0;
    int         keysCollected_ = 0;
    float       shieldTimer_   = 0.0f;
    int         currentWorld_  = 1;
    GameData    gameData_;
    std::string savePath_;

    static constexpr int kWCX = 50;
    static constexpr int kWCZ = 50;

    Urho3D::Context* context_;
};
