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

        // Real bug fix (2026-07-11, reported live via a debugger backtrace:
        // crashes after a while with `System::InvalidOperationException`
        // from `SoundEffectInstance::setIsLoopedProperty`, thrown from
        // inside `GESound::Play`/`PlayStep`). setIsLoopedProperty() is only
        // valid BEFORE an instance's first Play() call -- its own
        // `hasStarted_` guard, set the first time Play() runs, is never
        // reset, not even by Stop() (see SoundEffectInstance.hpp's own
        // doc comment on setIsLoopedProperty: "@throws ... if the instance
        // has already been played"). This code used to call it
        // unconditionally on every Play(), including on a reused,
        // already-started instance (the `else` branch below) -- the very
        // first *second* play of any channel (e.g. the second footstep
        // while walking) threw, uncaught, and crashed the whole game. Only
        // set it once, right after actually constructing a fresh instance.
        if (!ch.instance)
        {
            ch.instance = std::make_unique<SoundEffectInstance>(ch.effect->CreateInstance());
            ch.instance->setIsLoopedProperty(loop);
        }
        else
        {
            ch.instance->Stop();
        }

        const float volume = kVolumePitch[static_cast<std::size_t>(idx) * 2];
        ch.instance->setVolumeProperty(volume);
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

    GalaxyEggbert::SoundChannel GESound::FootstepChannelFor(std::uint16_t icon) noexcept
    {
        using GalaxyEggbert::SoundChannel;
        if (icon == 32 || icon == 33 || icon == 34 ||
            (icon >= 41 && icon <= 47) || (icon >= 139 && icon <= 143))
        {
            return SoundChannel::SoundChannel78;
        }
        if ((icon >= 1 && icon <= 28) || (icon >= 78 && icon <= 90) ||
            (icon >= 250 && icon <= 260) || (icon >= 311 && icon <= 316) ||
            (icon >= 324 && icon <= 329))
        {
            return SoundChannel::SoundChannel80;
        }
        if ((icon >= 284 && icon <= 303) || icon == 338)
        {
            return SoundChannel::SoundChannel82;
        }
        if (icon >= 341 && icon <= 363)
        {
            return SoundChannel::SoundChannel84;
        }
        if (icon >= 215 && icon <= 234)
        {
            return SoundChannel::SoundChannel86;
        }
        if (icon >= 246 && icon <= 249)
        {
            return SoundChannel::SoundChannel88;
        }
        if (icon >= 107 && icon <= 109)
        {
            return SoundChannel::SoundChannel90;
        }
        return SoundChannel::SoundChannel3;
    }
}
