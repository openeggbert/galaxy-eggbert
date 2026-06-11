#pragma once

#include "GEEngine.hpp"
#include <memory>

class GalaxyEggbertGame;

class GalaxyEggbertApp : public Urho3D::Application {
    URHO3D_OBJECT(GalaxyEggbertApp, Urho3D::Application);

public:
    explicit GalaxyEggbertApp(Urho3D::Context* context);

    void Setup() override;
    void Start() override;
    void Stop()  override;

    void GameUpdate(float dt);
    void HandleUpdate(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);

private:
    std::unique_ptr<GalaxyEggbertGame> game_;
};
