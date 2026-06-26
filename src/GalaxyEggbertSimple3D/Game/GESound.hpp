#pragma once

#include <Simple3D/Simple3D.h>
#include <GalaxyEggbert/def/SoundChannel.hpp>

namespace GESimple3D {

// Sound system for the Simple3D port.
// Channel indices map directly to asset file indices (ch1 → sounds/sound001.wav).
// Per-channel volume from tableVolumePitch (ported from mobile-eggbert).
// Conflict policy: channel is not restarted if already playing, except channel 10.
class GESound {
public:
    static constexpr int kNumChannels = 93;

    explicit GESound(Simple3D::Game* game) : game_(game) {}

    void Play(GalaxyEggbert::SoundChannel channel, bool loop = false);
    void Stop(GalaxyEggbert::SoundChannel channel);
    void StopAll();

    void SetEnabled(bool e) { enabled_ = e; if (!e) StopAll(); }
    bool IsEnabled() const { return enabled_; }
    void SetMasterVolume(float v) { if (game_) game_->SetMasterVolume(v); }

    // Named shortcuts matching Urho3D SoundManager channel assignments
    void PlayJump()     { Play(GalaxyEggbert::SoundChannel::SoundChannel1);  }
    void PlayStep()     { Play(GalaxyEggbert::SoundChannel::SoundChannel3);  }
    void PlayLand()     { Play(GalaxyEggbert::SoundChannel::SoundChannel4);  }
    void PlayStomp()    { Play(GalaxyEggbert::SoundChannel::SoundChannel5);  }
    void PlayHit()      { Play(GalaxyEggbert::SoundChannel::SoundChannel8);  }
    void PlayCollect()  { Play(GalaxyEggbert::SoundChannel::SoundChannel10); }
    void PlayKey()      { Play(GalaxyEggbert::SoundChannel::SoundChannel11); }
    void PlayLife()     { Play(GalaxyEggbert::SoundChannel::SoundChannel42); }
    void PlayShieldOff(){ Play(GalaxyEggbert::SoundChannel::SoundChannel44); }
    void PlayWin()      { Play(GalaxyEggbert::SoundChannel::SoundChannel57); }

private:
    Simple3D::Game* game_    = nullptr;
    bool            enabled_ = true;
};

} // namespace GESimple3D
