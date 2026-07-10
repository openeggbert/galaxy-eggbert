#include "GESound.hpp"

#include <Microsoft/Xna/Framework/Audio/SoundState.hpp>

#include <cstdio>
#include <filesystem>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Ported verbatim from mobile-eggbert Sound.hpp's tableVolumePitch --
        // the SAME real table already transcribed (with approval) into
        // GalaxyEggbertSimple3D's GESound.cpp, reused here rather than
        // re-transcribed. Index [ch*2] = volume multiplier, [ch*2+1] = pitch
        // (not applied here -- see GESound.hpp's class comment).
        constexpr float kVolumePitch[200] = {
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
    }

    void GESound::LoadContent()
    {
        using Microsoft::Xna::Framework::Audio::SoundEffect;

        for (int i = 0; i < kNumChannels; ++i)
        {
            char path[48];
            std::snprintf(path, sizeof(path), "Content/sounds/sound%03d.wav", i);
            if (std::filesystem::exists(path))
            {
                channels_[i].effect = std::make_unique<SoundEffect>(path);
            }
        }
    }

    int GESound::LoadedCount() const noexcept
    {
        int count = 0;
        for (const Channel& ch : channels_)
        {
            if (ch.effect)
            {
                ++count;
            }
        }
        return count;
    }

    void GESound::Play(GalaxyEggbert::SoundChannel channel, bool loop)
    {
        using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
        using Microsoft::Xna::Framework::Audio::SoundState;

        if (!enabled_)
        {
            return;
        }
        const int idx = static_cast<int>(GalaxyEggbert::ToRaw(channel));
        if (idx < 0 || idx >= kNumChannels)
        {
            return;
        }
        Channel& ch = channels_[static_cast<std::size_t>(idx)];
        if (!ch.effect)
        {
            return; // no WAV file for this channel -- silent no-op
        }

        // Channel 10 always restarts; every other channel doesn't interrupt
        // itself if already playing (matches Simple3D's GESound exactly).
        if (channel != GalaxyEggbert::SoundChannel::SoundChannel10 && !loop &&
            ch.instance && ch.instance->getStateProperty() == SoundState::Playing)
        {
            return;
        }

        if (!ch.instance)
        {
            ch.instance = std::make_unique<SoundEffectInstance>(ch.effect->CreateInstance());
        }
        else
        {
            ch.instance->Stop();
        }

        const float volume = kVolumePitch[static_cast<std::size_t>(idx) * 2];
        ch.instance->setVolumeProperty(volume);
        ch.instance->setIsLoopedProperty(loop);
        ch.instance->Play();
    }

    void GESound::StopAll()
    {
        for (Channel& ch : channels_)
        {
            if (ch.instance)
            {
                ch.instance->Stop();
            }
        }
    }
}
