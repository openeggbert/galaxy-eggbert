#include "GalaxyEggbertSimpleGame.hpp"
#include "Support/Simple3DMissingFeatures.hpp"
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <string>

using namespace Simple3D;
using namespace GalaxyEggbert;

// ─── lifecycle ───────────────────────────────────────────────────────────────

void GalaxyEggbertSimpleGame::Start() {
    SetWindowTitle("Galaxy Eggbert");
    SetWindowSize(1280, 720);
    SetClearColor(Color(0.14f, 0.12f, 0.30f));
    SetAppName("GalaxyEggbert");

    // Resource prefix path: U3D binary dir should be set from CMake via
    // the GALAXY_EGGBERT_U3D_BIN_PATH define (similar to blupi_proto).
#if defined(GALAXY_EGGBERT_U3D_BIN_PATH)
    SetResourcePrefixPaths(GALAXY_EGGBERT_U3D_BIN_PATH);
#endif

    sound_ = std::make_unique<GESimple3D::GESound>(this);

    LoadAllSlots();
    LoadSettings();
    sound_->SetEnabled(soundOn_);

    SetupInput();

    // Lighting
    auto* sun = CreateEntity("Sun");
    sun->AddDirectionalLight(Color(1.0f, 0.94f, 0.80f), 2.0f);
    sun->SetRotation(Quaternion(50.0f, 30.0f, 0.0f));

    auto* fill = CreateEntity("Fill");
    fill->AddDirectionalLight(Color(0.35f, 0.45f, 0.85f), 0.55f);
    fill->SetRotation(Quaternion(-20.0f, -120.0f, 0.0f));
    fill->SetCastShadows(false);

    hud_.Create(*this);

    EnterPhase(GamePhase::Init);
}

void GalaxyEggbertSimpleGame::SetupInput() {
    BindAction("Jump")
        .Key(Key::LCtrl)
        .Key(Key::Space)
        .GamepadButton(GamepadButton::A);

    BindAction("Pause")
        .Key(Key::Escape)
        .GamepadButton(GamepadButton::Start);

    BindAction("AnyKey")
        .Key(Key::Return)
        .Key(Key::Space)
        .Key(Key::LCtrl)
        .GamepadButton(GamepadButton::A)
        .GamepadButton(GamepadButton::B);

    // TODO(#24B): Move to Task #24B Input Actions when defined.
    // Tank controls: Left/Right = turn, Up/Down = move forward/back.
    BindAxis2D("Move")
        .Keys(Key::Left, Key::Right, Key::Down, Key::Up)
        .GamepadStick(GamepadAxis::LeftX, GamepadAxis::LeftY)
        .InvertY(true);
}

void GalaxyEggbertSimpleGame::Update(float dt) {
    dt *= gameSpeed_;

    switch (phase_) {
        case GamePhase::Init:      UpdateInit(dt);     break;
        case GamePhase::Play:      UpdatePlay(dt);     break;
        case GamePhase::Pause:     UpdatePause(dt);    break;
        case GamePhase::MainSetup:
        case GamePhase::PlaySetup: UpdateSettings(dt); break;
        case GamePhase::Win:       UpdateWin(dt);      break;
        case GamePhase::Lost:      UpdateLost(dt);     break;
        default: break;
    }
    hud_.Update(dt);
}

void GalaxyEggbertSimpleGame::Stop() {
    sound_.reset();
}

// ─── phase management ────────────────────────────────────────────────────────

void GalaxyEggbertSimpleGame::EnterPhase(GamePhase next) {
    phase_ = next;
    phaseTimer_ = 0.0f;

    switch (next) {
        case GamePhase::Init:
            hud_.ShowInit(BuildInitText());
            break;
        case GamePhase::Play:
            controlsHintTimer_ = 8.0f;
            hud_.SetVisible(true);
            break;
        case GamePhase::Pause:
            hud_.ShowPause(score_, worldRuntime_.GetLevelTime());
            break;
        case GamePhase::MainSetup:
            hud_.ShowSettings(soundOn_, false);
            break;
        case GamePhase::PlaySetup:
            hud_.ShowSettings(soundOn_, true);
            break;
        case GamePhase::Win:
            hud_.ShowWin(currentWorld_,
                         GESimple3D::GEWorldRuntime::WorldName(currentWorld_),
                         decor_.GetCollected(), decor_.GetTotalTreasures(),
                         lives_, score_,
                         worldRuntime_.GetLevelTime());
            break;
        case GamePhase::Lost:
            hud_.ShowLost(currentWorld_,
                          GESimple3D::GEWorldRuntime::WorldName(currentWorld_));
            break;
        default: break;
    }
}

// ─── world loading ───────────────────────────────────────────────────────────

void GalaxyEggbertSimpleGame::LoadWorld(int worldNum) {
    terrain_.Clear(*this);
    decor_.Clear(*this);

    worldRuntime_.SetWorldNum(worldNum);
    worldRuntime_.ResetLevel();

    // Locate the world file adjacent to the executable (desktop convention).
    // TODO: Use proper file-system resolution for web/Android.
    char fname[64];
    std::snprintf(fname, sizeof(fname), "worlds/world%03d.txt", worldNum);

    bool loaded = false;
    if (std::filesystem::exists(fname))
        loaded = worldRuntime_.LoadFromMobileEggbertFile(fname);
    if (!loaded)
        worldRuntime_.BuildDemoWorld();

    // TODO(S3D-sky): Set sky/fog per world region — Simple3D lacks zone/fog API.
    //   Simple3D::Game has no SetFogColor yet; SetClearColor is a rough proxy.
    static const Color kSkyColors[] = {
        Color(0.55f, 0.72f, 0.42f), // 1 Grassland
        Color(0.12f, 0.20f, 0.10f), // 2 Forest
        Color(0.62f, 0.72f, 0.88f), // 3 Ice Caves
        Color(0.22f, 0.08f, 0.04f), // 4 Lava Fields
        Color(0.02f, 0.03f, 0.10f), // 5 Space Station
    };
    int idx = std::max(0, std::min(worldNum - 1, 4));
    SetClearColor(kSkyColors[idx]);

    terrain_.Build(*this, worldRuntime_);

    blupi_.SetSpawnPoint(worldRuntime_.GetBlupiSpawn());
    blupi_.Respawn();

    decor_.Build(*this, worldRuntime_, blupi_.GetEntity());

    camera_.Create(*this, blupi_.GetEntity());

    exitOpen_         = false;
    bonusLifeAwarded_ = false;
    shieldTimer_      = 0.0f;
    prevCollected_    = 0;
    prevTotalKeys_    = 0;
    wasShieldActive_  = false;
}

// ─── slot persistence ─────────────────────────────────────────────────────────

void GalaxyEggbertSimpleGame::LoadAllSlots() {
    for (int i = 0; i < 3; ++i) {
        Simple3D::SaveData sd;
        if (sd.Load(i)) {
            slots_[i].lives = sd.GetInt("lives", 3);
            slots_[i].world = sd.GetInt("world", 1);
            slots_[i].best  = sd.GetInt("best",  0);
        }
    }
}

void GalaxyEggbertSimpleGame::SaveSlot(int idx) {
    Simple3D::SaveData sd;
    sd.Set("lives", slots_[idx].lives);
    sd.Set("world", slots_[idx].world);
    sd.Set("best",  slots_[idx].best);
    sd.Save(idx);
}

void GalaxyEggbertSimpleGame::LoadSettings() {
    Simple3D::SaveData sd;
    if (sd.Load(3)) soundOn_ = sd.GetBool("sound", true);
}

void GalaxyEggbertSimpleGame::SaveSettings() {
    Simple3D::SaveData sd;
    sd.Set("sound", soundOn_);
    sd.Save(3);
}

std::string GalaxyEggbertSimpleGame::BuildInitText() const {
    char buf[512];
    int n = 0;
    n += std::snprintf(buf+n, sizeof(buf)-n, "Galaxy Eggbert\n\nSelect a gamer:\n\n");
    for (int i = 0; i < 3; ++i) {
        n += std::snprintf(buf+n, sizeof(buf)-n,
            "  [%d]  Lives: %d   World: %d   Best: %d\n",
            i+1, slots_[i].lives, slots_[i].world, slots_[i].best);
    }
    std::snprintf(buf+n, sizeof(buf)-n,
        "\n1/2/3: choose gamer   S: Settings   Esc: quit");
    return buf;
}

// ─── gamer select ────────────────────────────────────────────────────────────

void GalaxyEggbertSimpleGame::SelectGamer(int slot) {
    gamerSlot_    = slot;
    currentWorld_ = slots_[slot - 1].world;
    lives_        = slots_[slot - 1].lives;
    score_        = 0;
    blupi_.Create(*this);
    LoadWorld(currentWorld_);
    EnterPhase(GamePhase::Play);
}

// ─── level transitions ───────────────────────────────────────────────────────

void GalaxyEggbertSimpleGame::ResetLevel() {
    lives_--;
    int idx = gamerSlot_ - 1;
    if (lives_ <= 0) {
        lives_ = 0;
        if (score_ > slots_[idx].best) slots_[idx].best = score_;
        slots_[idx].lives = 3;
        slots_[idx].world = 1;
        SaveSlot(idx);
        EnterPhase(GamePhase::Lost);
        return;
    }
    slots_[idx].lives = lives_;
    if (score_ > slots_[idx].best) slots_[idx].best = score_;
    SaveSlot(idx);
    LoadWorld(currentWorld_);
    EnterPhase(GamePhase::Play);
}

void GalaxyEggbertSimpleGame::AdvanceToNextWorld() {
    currentWorld_++;
    if (currentWorld_ > kMaxWorld) currentWorld_ = 1;
    int idx = gamerSlot_ - 1;
    slots_[idx].world = currentWorld_;
    slots_[idx].lives = lives_;
    if (score_ > slots_[idx].best) slots_[idx].best = score_;
    SaveSlot(idx);
    LoadWorld(currentWorld_);
    EnterPhase(GamePhase::Play);
}

// ─── phase updates ───────────────────────────────────────────────────────────

void GalaxyEggbertSimpleGame::UpdateInit(float dt) {
    (void)dt;
    hud_.ShowInit(BuildInitText());

    for (int slot = 1; slot <= 3; ++slot) {
        Key k = (slot == 1) ? Key::Num1 : (slot == 2) ? Key::Num2 : Key::Num3;
        if (IsKeyPressed(k) ||
            (slot == 1 && IsGamepadButtonPressed(GamepadButton::A))) {
            SelectGamer(slot);
            return;
        }
    }
    if (IsKeyPressed(Key::S)) {
        settingsReturnPhase_ = GamePhase::Init;
        EnterPhase(GamePhase::MainSetup);
        return;
    }
    if (IsKeyPressed(Key::Escape) || IsGamepadButtonPressed(GamepadButton::B))
        Quit();
}

void GalaxyEggbertSimpleGame::UpdatePlay(float dt) {
    phaseTimer_       += dt;
    controlsHintTimer_ -= dt;
    if (shieldTimer_ > 0.0f) shieldTimer_ -= dt;

    worldRuntime_.Update(dt);

    blupi_.SetShieldTimer(shieldTimer_);
    blupi_.SetInputFrozen(false);
    blupi_.Update(*this, dt);

    Vector3 blupiPos = blupi_.GetPosition();
    float   blupiVelY = blupi_.GetVelY();

    decor_.Update(dt, blupiPos, blupiVelY);

    // Collect treasures
    if (decor_.GetCollected() > 0 && !exitOpen_) {
        if (decor_.GetCollected() >= decor_.GetTotalTreasures() &&
            decor_.GetTotalTreasures() > 0) {
            exitOpen_ = true;
            // TODO: Show "EXIT OPEN!" popup — needs Simple3D::Label with timed fade
        }
        if (!bonusLifeAwarded_ && exitOpen_) {
            bonusLifeAwarded_ = true;
            lives_ = std::min(lives_ + 1, 9);
            score_ += 100;
        }
    }

    // Collect treasure score
    if (decor_.GetCollected() > prevCollected_) {
        score_ += 10 * (decor_.GetCollected() - prevCollected_);
        prevCollected_ = decor_.GetCollected();
        sound_->PlayCollect();
    }

    if (decor_.WasEggCollected() || decor_.WasDrinkCollected()) {
        lives_ = std::min(lives_ + 1, 9);
        score_ += 50;
        sound_->PlayLife();
    }

    // Key pickup sound (ch11)
    int totalKeys = decor_.GetKeys49() + decor_.GetKeys50() + decor_.GetKeys51();
    if (totalKeys > prevTotalKeys_) sound_->PlayKey();
    prevTotalKeys_ = totalKeys;

    // Shield-off sound (ch44) when shield expires
    bool shieldNow = blupi_.IsShieldActive();
    if (wasShieldActive_ && !shieldNow) sound_->PlayShieldOff();
    wasShieldActive_ = shieldNow;

    if (decor_.WasShieldCollected()) {
        shieldTimer_ = 5.0f;
        sound_->PlayCollect();
    }

    if (decor_.WasStompKill()) {
        score_ += 25;
        sound_->PlayStomp();
        blupi_.Respawn(); // bounce placeholder — TODO: real Bounce()
    }

    if (decor_.WasBlupiHit() && !blupi_.IsShieldActive()) {
        hud_.ShowHitFlash();
        sound_->PlayHit();
        ResetLevel();
        return;
    }

    if (decor_.WasExitReached() && exitOpen_) {
        sound_->PlayWin();
        EnterPhase(GamePhase::Win);
        return;
    }

    if (blupi_.WasLandedThisFrame())  sound_->PlayLand();
    if (blupi_.WasJumpedThisFrame())  sound_->PlayJump();

    // Fall death
    if (blupi_.GetPosition().y_ < -10.0f) {
        hud_.ShowHitFlash();
        sound_->PlayHit();
        ResetLevel();
        return;
    }

    // Game speed: G key cycles Slow (0.6) → Normal (1.0) → Fast (1.5)
    if (IsKeyPressed(Key::G)) {
        if (gameSpeed_ < 0.8f)       gameSpeed_ = 1.0f;
        else if (gameSpeed_ < 1.2f)  gameSpeed_ = 1.5f;
        else                         gameSpeed_ = 0.6f;
    }

    // Pause
    if (IsActionPressed("Pause")) {
        EnterPhase(GamePhase::Pause);
        return;
    }

    hud_.ShowPlay(
        currentWorld_,
        GESimple3D::GEWorldRuntime::WorldName(currentWorld_),
        lives_, decor_.GetCollected(), decor_.GetTotalTreasures(),
        decor_.GetKeys49(), decor_.GetKeys50(), decor_.GetKeys51(),
        shieldTimer_, worldRuntime_.GetLevelTime(), score_,
        gameSpeed_, controlsHintTimer_ > 0.0f);

    decor_.ClearEvents();
}

void GalaxyEggbertSimpleGame::UpdatePause(float dt) {
    (void)dt;
    if (IsKeyPressed(Key::S)) {
        settingsReturnPhase_ = GamePhase::Pause;
        EnterPhase(GamePhase::PlaySetup);
        return;
    }
    if (IsActionPressed("Pause") || IsActionPressed("AnyKey")) {
        EnterPhase(GamePhase::Play);
    }
}

void GalaxyEggbertSimpleGame::UpdateSettings(float dt) {
    (void)dt;
    if (IsKeyPressed(Key::S)) {
        soundOn_ = !soundOn_;
        if (sound_) sound_->SetEnabled(soundOn_);
        SaveSettings();
        hud_.ShowSettings(soundOn_, settingsReturnPhase_ == GamePhase::Pause);
    }
    if (IsKeyPressed(Key::Escape))
        EnterPhase(settingsReturnPhase_);
}

void GalaxyEggbertSimpleGame::UpdateWin(float dt) {
    phaseTimer_ += dt;
    if (phaseTimer_ > 1.0f && IsActionPressed("AnyKey")) {
        AdvanceToNextWorld();
    }
}

void GalaxyEggbertSimpleGame::UpdateLost(float dt) {
    phaseTimer_ += dt;
    if (phaseTimer_ > 1.0f && IsActionPressed("AnyKey")) {
        currentWorld_ = 1;
        lives_        = 3;
        score_        = 0;
        EnterPhase(GamePhase::Init);
    }
}
