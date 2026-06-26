#pragma once

#include <Simple3D/Simple3D.h>
#include "Game/GEWorldRuntime.hpp"
#include "Game/GETerrainRenderer.hpp"
#include "Game/GEBlupiController.hpp"
#include "Game/GEDecorSystem.hpp"
#include "Game/GEHud.hpp"
#include "Game/GESound.hpp"
#include "Game/GECameraRig.hpp"
#include <GalaxyEggbert/def/GamePhase.hpp>
#include <memory>
#include <string>

// Main game class for the Simple3D port of galaxy-eggbert.
// Corresponds to GalaxyEggbertApp + GalaxyEggbertGame in the old Urho3D tree.
class GalaxyEggbertSimpleGame final : public Simple3D::Game {
public:
    void Start()           override;
    void Update(float dt)  override;
    void Stop()            override;

private:
    void SetupInput();
    void LoadWorld(int worldNum);
    void EnterPhase(GalaxyEggbert::GamePhase next);
    void ResetLevel();
    void AdvanceToNextWorld();
    void SelectGamer(int slot);

    void UpdateInit(float dt);
    void UpdatePlay(float dt);
    void UpdatePause(float dt);
    void UpdateSettings(float dt);
    void UpdateWin(float dt);
    void UpdateLost(float dt);

    // Slot persistence helpers
    void LoadAllSlots();
    void SaveSlot(int idx);
    void LoadSettings();
    void SaveSettings();
    std::string BuildInitText() const;

    // Subsystems
    GESimple3D::GEWorldRuntime   worldRuntime_;
    GESimple3D::GETerrainRenderer terrain_;
    GESimple3D::GEBlupiController blupi_;
    GESimple3D::GEDecorSystem     decor_;
    GESimple3D::GEHud             hud_;
    GESimple3D::GECameraRig       camera_;
    std::unique_ptr<GESimple3D::GESound> sound_;

    // ── per-slot save data ────────────────────────────────────────────────────
    struct SlotData { int lives = 3; int world = 1; int best = 0; };
    SlotData slots_[3];
    bool soundOn_ = true;

    // ── persistent game state ─────────────────────────────────────────────────
    GalaxyEggbert::GamePhase phase_              = GalaxyEggbert::GamePhase::Init;
    GalaxyEggbert::GamePhase settingsReturnPhase_ = GalaxyEggbert::GamePhase::Init;
    int   currentWorld_  = 1;
    int   lives_         = 3;
    int   score_         = 0;
    float gameSpeed_     = 1.0f;
    int   gamerSlot_     = 1;

    // ── per-level state ───────────────────────────────────────────────────────
    float shieldTimer_        = 0.0f;
    float respawnInvincTimer_ = 0.0f;   // 2s invincibility + sprite flash after respawn
    float controlsHintTimer_  = 8.0f;
    bool  exitOpen_           = false;
    bool  bonusLifeAwarded_   = false;

    // ── phase transition state ─────────────────────────────────────────────────
    float phaseTimer_      = 0.0f;
    int   prevCollected_   = 0;
    int   prevTotalKeys_   = 0;
    bool  wasShieldActive_ = false;

    static constexpr int kMaxWorld = 5;
};
