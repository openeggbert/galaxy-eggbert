#pragma once

#include <Simple3D/Simple3D.h>
#include <string>

namespace GESimple3D {

// Minimal sound wrapper for the Simple3D port (S3D-1 pass).
//
// The old SoundManager had 93 indexed WAV channels with per-channel volume from
// a tableVolumePitch table. Simple3D exposes only Game::PlaySound(path, volume).
// In S3D-1 we map only the 5 most critical sound events to named paths.
// TODO(S3D-7): Expand with per-channel volume and loop control when Simple3D
//              exposes a richer Audio API.  See docs/SIMPLE3D_GAPS.md.
class GESound {
public:
    explicit GESound(Simple3D::Game* game) : game_(game) {}

    void PlayJump()    { Play("sounds/sound042.wav", 0.85f); }
    void PlayLand()    { Play("sounds/sound018.wav", 0.70f); }
    void PlayCollect() { Play("sounds/sound010.wav", 0.80f); }
    void PlayHit()     { Play("sounds/sound033.wav", 0.90f); }
    void PlayStomp()   { Play("sounds/sound047.wav", 0.80f); }
    void PlayWin()     { Play("sounds/sound057.wav", 0.90f); }

    void SetEnabled(bool e) { enabled_ = e; }
    bool IsEnabled()  const { return enabled_; }

    // Set master volume via Simple3D (0.0 – 1.0).
    void SetMasterVolume(float v) { if (game_) game_->SetMasterVolume(v); }

private:
    void Play(const std::string& path, float volume) {
        if (enabled_ && game_) game_->PlaySound(path, volume);
    }

    Simple3D::Game* game_   = nullptr;
    bool            enabled_ = true;
};

} // namespace GESimple3D
