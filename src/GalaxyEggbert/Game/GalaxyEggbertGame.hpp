#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/def/GamePhase.hpp"
#include "GalaxyEggbert/def/ObjectType.hpp"
#include "Blupi.hpp"
#include "Camera.hpp"
#include "Decor.hpp"
#include "HUD.hpp"
#include "PhaseManager.hpp"
#include "GameData.hpp"
#include "SoundManager.hpp"
#include "Explosion.hpp"
#include "ScorePopup.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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
    struct MobileObjSpec {
        GalaxyEggbert::ObjectType type;
        Urho3D::Vector3           posStart;
        Urho3D::Vector3           posEnd;
        float                     speed;
    };

    void CreateScene();
    void LoadWorld(int worldNum);
    void BuildDemoWorld();
    bool LoadMobileEggbertTerrain(const char* path);
    void SpawnTerrainNodes();
    void CreateDemoObjects();
    void UpdateSkyDome(int region);

    void EnterPhase(GalaxyEggbert::GamePhase next);

    Urho3D::SharedPtr<Urho3D::Material> MakeFlatMaterial(
        const Urho3D::Color& color, float emissive = 0.12f);
    Urho3D::SharedPtr<Urho3D::Material> GetTileMaterial(uint16_t blockType);

    void UpdateInit(float dt);
    void UpdatePlay(float dt);
    void UpdatePause(float dt);
    void UpdateWin(float dt);
    void UpdateLost(float dt);
    void UpdateSettings(float dt);
    void ResetLevel();
    void AdvanceToNextWorld();
    void SelectGamer(int slot);

    // Scene
    Urho3D::SharedPtr<Urho3D::Scene> scene_;
    Urho3D::SharedPtr<Urho3D::Node>  terrainRoot_;

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
    std::vector<std::unique_ptr<Explosion>>  explosions_;
    std::vector<std::unique_ptr<ScorePopup>> popups_;

    std::vector<MobileObjSpec> mobileObjects_;
    Urho3D::Vector3            blupiSpawn_{0.0f, Blupi::kHalfH + 0.5f, 0.0f};
    int                        skyRegion_ = 0;

    // ── persistent game state (survives level transitions) ──────────────────
    GalaxyEggbert::GamePhase settingsReturnPhase_ = GalaxyEggbert::GamePhase::Init;
    bool        drawDebug_    = false;
    int         lives_        = 3;
    int         currentWorld_ = 1;
    int         score_        = 0;    // accumulated across all levels; reset on full restart
    float       totalTime_    = 0.0f; // runs continuously; drives hazard-tile animation
    GameData    gameData_;
    std::string savePath_;

    // ── per-level state (reset in AdvanceToNextWorld / ResetLevel) ──────────
    float       levelTime_                = 0.0f; // seconds elapsed this level
    int         prevCollected_            = 0;
    int         keysRed_ = 0, keysGreen_ = 0, keysBlue_ = 0;
    float       shieldTimer_              = 0.0f;
    float       respawnInvincibleTimer_   = 0.0f;
    float       deathFreezeTimer_         = 0.0f;
    float       controlsHintTimer_        = 8.0f;
    bool        bonusLifeAwarded_         = false;

    static constexpr int kWCX     = 50;
    static constexpr int kWCZ     = 50;
    static constexpr int kMaxWorld = 5;

    Urho3D::Context* context_;
};
