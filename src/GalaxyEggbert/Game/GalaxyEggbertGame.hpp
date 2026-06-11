#pragma once

#include "../GEEngine.hpp"

// GalaxyEggbertGame — engine-agnostic game class.
// All code here uses only the shared Urho3D:: scene-graph API that both
// U3D and Nova3D (once complete) expose.  Engine-specific bootstrap lives
// in GalaxyEggbertApp; this class never touches entry-point details.
class GalaxyEggbertGame {
public:
    explicit GalaxyEggbertGame(Urho3D::Context* context);
    ~GalaxyEggbertGame();

    void Start();
    void Update(float dt);
    void Stop();

private:
#ifdef GE_ENGINE_U3D
    void CreateScene();
    void CreateCamera();
    void CreateTerrain();
    void CreateHUD();
    Urho3D::SharedPtr<Urho3D::Material> MakeFlatMaterial(const Urho3D::Color& color,
                                                          float emissive = 0.12f);
    void UpdateCamera(float dt);
    void UpdateOrbit(float dt);

    Urho3D::SharedPtr<Urho3D::Scene>  scene_;
    Urho3D::WeakPtr<Urho3D::Node>     cameraNode_;
    Urho3D::WeakPtr<Urho3D::Node>     orbitNode_;

    float yaw_      = 0.0f;
    float pitch_    = 20.0f;
    float elapsed_  = 0.0f;
    bool  drawDebug_ = false;
#endif

    Urho3D::Context* context_;
};
