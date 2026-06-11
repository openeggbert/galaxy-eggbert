#pragma once

#include "../GEEngine.hpp"

class GalaxyEggbertGame {
public:
    explicit GalaxyEggbertGame(Urho3D::Context* context);
    ~GalaxyEggbertGame();

    void Start();
    void Update(float dt);
    void Stop();

private:
    void CreateScene();
    void CreateCamera();
    void CreateTerrain();
    void CreateHUD();
    Urho3D::SharedPtr<Urho3D::Material> MakeFlatMaterial(const Urho3D::Color& color,
                                                          float emissive = 0.12f);
    void UpdateCamera(float dt);
    void UpdateOrbit(float dt);

    Urho3D::SharedPtr<Urho3D::Scene> scene_;
    Urho3D::WeakPtr<Urho3D::Node>   cameraNode_;
    Urho3D::WeakPtr<Urho3D::Node>   orbitNode_;

    float yaw_       = 0.0f;
    float pitch_     = 20.0f;
    float elapsed_   = 0.0f;
    bool  drawDebug_ = false;

    Urho3D::Context* context_;
};
