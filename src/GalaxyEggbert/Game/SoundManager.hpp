#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/def/SoundChannel.hpp"
#include <array>

// Wraps Urho3D audio for one-shot and looped sound-effect playback.
// Channel indices map directly to sound asset file indices (sound000.wav … sound092.wav).
// Volume per channel is taken from the tableVolumePitch table ported from mobile-eggbert.
// Same-channel conflict policy: a playing channel is not restarted (except SoundChannel10).
class SoundManager {
public:
    SoundManager(Urho3D::Context* context, Urho3D::Scene* scene);
    ~SoundManager() = default;

    void Play(GalaxyEggbert::SoundChannel channel, bool loop = false);
    void Stop(GalaxyEggbert::SoundChannel channel);
    void StopAll();

    static constexpr int kNumChannels = 93;

private:
    Urho3D::Context* context_;
    Urho3D::Node*    audioNode_ = nullptr;
    std::array<Urho3D::SharedPtr<Urho3D::Sound>, kNumChannels>  sounds_{};
    std::array<Urho3D::SoundSource*, kNumChannels>              sources_{};
};
