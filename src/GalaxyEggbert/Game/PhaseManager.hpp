#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/def/GamePhase.hpp"

class PhaseManager {
public:
    explicit PhaseManager(Urho3D::Context* context);
    ~PhaseManager() = default;

    void Enter(GalaxyEggbert::GamePhase phase);
    GalaxyEggbert::GamePhase Current() const { return phase_; }

private:
    void ShowOverlay(const char* texPath);
    void HideOverlay();

    Urho3D::Context*                    context_;
    GalaxyEggbert::GamePhase            phase_ = GalaxyEggbert::GamePhase::None;
    Urho3D::WeakPtr<Urho3D::UIElement>  overlayEl_;
};
