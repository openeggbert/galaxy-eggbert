#pragma once

#include "../GEEngine.hpp"
#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/def/GamePhase.hpp"
#include "Blupi.hpp"

#include <memory>
#include <unordered_map>

// Main game coordinator: owns the scene, terrain, camera, Blupi, and phase state.
// Phase transitions: Init (title) → Play (3D world) ↔ Pause (overlay).
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
    void CreateCamera();
    void CreateHUD();

    void EnterPhase(GalaxyEggbert::GamePhase next);
    void ShowOverlay(const char* texPath);
    void HideOverlay();

    Urho3D::SharedPtr<Urho3D::Material> MakeFlatMaterial(const Urho3D::Color& color, float emissive = 0.12f);
    Urho3D::SharedPtr<Urho3D::Material> GetTileMaterial(uint16_t blockType);

    void UpdateInit(float dt);
    void UpdatePlay(float dt);
    void UpdatePause(float dt);
    void UpdateCamera(float dt);

    // Scene
    Urho3D::SharedPtr<Urho3D::Scene>    scene_;
    Urho3D::WeakPtr<Urho3D::Node>       cameraNode_;

    // World + tile cache
    std::unique_ptr<GalaxyEggbert::Worlds::World> world_;
    std::unordered_map<uint16_t, Urho3D::SharedPtr<Urho3D::Material>> tileMatCache_;
    Urho3D::SharedPtr<Urho3D::Texture2D> objectSheet_;

    // Characters
    std::unique_ptr<Blupi> blupi_;

    // UI
    Urho3D::WeakPtr<Urho3D::UIElement> overlayEl_;
    Urho3D::WeakPtr<Urho3D::Text>      hudText_;

    // State
    GalaxyEggbert::GamePhase phase_ = GalaxyEggbert::GamePhase::Init;

    float camYaw_    = 180.0f;
    float camPitch_  =  25.0f;
    float camDist_   =  12.0f;
    bool  drawDebug_ = false;

    static constexpr int kWCX = 50;
    static constexpr int kWCZ = 50;

    Urho3D::Context* context_;
};
