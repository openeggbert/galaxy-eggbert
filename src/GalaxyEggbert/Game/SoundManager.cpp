#include "SoundManager.hpp"
#include <cstdio>

using namespace Urho3D;
using namespace GalaxyEggbert;

// Ported verbatim from mobile-eggbert Sound.hpp tableVolumePitch.
// Indexed [ch*2]=volume multiplier, [ch*2+1]=pitch shift (ignored — no freq data).
static const float kTableVolumePitch[200] = {
    1.0f, 0.0f,  0.5f, 1.0f,  0.5f, 1.0f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.1f,  1.0f, 0.3f,  1.0f, 0.2f,  1.0f, 0.3f,  1.0f, 0.5f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.1f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.0f,  1.0f, 0.0f,  1.0f, 0.0f,  1.0f, 0.0f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  0.7f, 0.2f,  1.0f, 0.1f,  1.0f, 0.1f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.4f,  1.0f, 0.0f,  1.0f, 0.0f,
    1.0f, 0.0f,  1.0f, 0.0f,  1.0f, 0.2f,  1.0f, 0.2f,  0.7f, 0.4f,
    1.0f, 0.2f,  1.0f, 0.4f,  1.0f, 0.2f,  0.5f, 1.0f,  0.5f, 1.0f,
    1.0f, 0.4f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  0.6f, 0.4f,  0.8f, 0.1f,
    0.6f, 0.5f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.0f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.0f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.0f,  1.0f, 0.0f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  0.6f, 0.4f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,
    1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,  1.0f, 0.2f,
};

SoundManager::SoundManager(Context* context, Scene* scene)
    : context_(context)
{
    auto* cache = context_->GetSubsystem<ResourceCache>();
    audioNode_ = scene->CreateChild("Audio");

    char path[48];
    for (int i = 0; i < kNumChannels; ++i) {
        std::snprintf(path, sizeof(path), "sounds/sound%03d.wav", i);
        sounds_[i] = cache->GetResource<Sound>(path);

        // One child node per channel — Urho3D allows only one SoundSource per node.
        auto* chNode = audioNode_->CreateChild(String("AudioCh") + String(i));
        sources_[i] = chNode->CreateComponent<SoundSource>();
        sources_[i]->SetSoundType(SOUND_EFFECT);
        if (i * 2 < 200)
            sources_[i]->SetGain(kTableVolumePitch[i * 2]);
    }
}

void SoundManager::Play(SoundChannel channel, bool loop) {
    if (!enabled_) return;
    int idx = static_cast<int>(ToRaw(channel));
    if (idx < 0 || idx >= kNumChannels) return;
    if (!sounds_[idx] || !sources_[idx]) return;

    if (channel != SoundChannel::SoundChannel10 && sources_[idx]->IsPlaying()) return;

    sounds_[idx]->SetLooped(loop);
    if (idx * 2 < 200)
        sources_[idx]->SetGain(kTableVolumePitch[idx * 2]);
    sources_[idx]->Play(sounds_[idx].Get());
}

void SoundManager::Stop(SoundChannel channel) {
    int idx = static_cast<int>(ToRaw(channel));
    if (idx < 0 || idx >= kNumChannels || !sources_[idx]) return;
    sources_[idx]->Stop();
}

void SoundManager::StopAll() {
    for (int i = 0; i < kNumChannels; ++i) {
        if (sources_[i] && sources_[i]->IsPlaying())
            sources_[i]->Stop();
    }
}
