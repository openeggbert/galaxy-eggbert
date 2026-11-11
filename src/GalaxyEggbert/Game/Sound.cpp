#include "Sound.hpp"

#include <Microsoft/Xna/Framework/Audio/SoundState.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace GalaxyEggbert::Game
{
    namespace
    {
        // Ported verbatim, with approval, from mobile-eggbert Sound.hpp's
        // tableVolumePitch. Index [ch*2] = volume multiplier,
        // [ch*2+1] = pitch (XNA octaves; BD-0031).
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

    void Sound::LoadContent()
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

    int Sound::LoadedCount() const noexcept
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

    bool Sound::MutedWhileHidden(GalaxyEggbert::Def::SoundChannel channel) noexcept
    {
        // Decor.cpp:1521-1566, the list as it stands (8-19, 26, 28-31, 33, 41-45, 50-63, 66-77 and
        // 92 still play: the world's sounds, not his).
        const int c = static_cast<int>(GalaxyEggbert::Def::ToRaw(channel));
        return (c >= 1 && c <= 7) || (c >= 20 && c <= 25) || c == 27 || c == 32 || (c >= 34 && c <= 40) ||
               (c >= 46 && c <= 49) || c == 64 || c == 65 || (c >= 78 && c <= 91);
    }

    void Sound::Play(GalaxyEggbert::Def::SoundChannel channel, bool loop)
    {
        Note(channel, loop, listenerX_, listenerY_, listenerZ_);
        PlayChannel(channel, loop, true);
    }

    void Sound::PlayAt(GalaxyEggbert::Def::SoundChannel channel, float x, float y, float z, bool loop)
    {
        Note(channel, loop, x, y, z);
        if (!Audible(x - listenerX_, y - listenerY_, z - listenerZ_))
        {
            return;
        }
        PlayChannel(channel, loop, true);
    }

    void Sound::PlayRemote(GalaxyEggbert::Def::SoundChannel channel, float x, float y, float z)
    {
        auto* const outbox = outbox_;
        outbox_ = nullptr;
        if (Audible(x - listenerX_, y - listenerY_, z - listenerZ_)) PlayChannel(channel, false, false);
        outbox_ = outbox;
    }

    void Sound::Note(GalaxyEggbert::Def::SoundChannel channel, bool loop, float x, float y, float z)
    {
        if (outbox_ == nullptr || loop) return;
        // Not what the hidden Blupi's filter keeps from playing (it is not heard here either).
        if (blupiHidden_ && MutedWhileHidden(channel)) return;
        const int idx = static_cast<int>(GalaxyEggbert::Def::ToRaw(channel));
        if (idx < 0 || idx >= kNumChannels) return;
        const Channel& ch = channels_[static_cast<std::size_t>(idx)];
        if (!ch.effect) return;
        const auto now = std::chrono::steady_clock::now();
        auto& until = notedUntil_[static_cast<std::size_t>(idx)];
        if (channel != GalaxyEggbert::Def::SoundChannel::SoundChannel10 && now < until) return;
        // As long as it plays here: its length, shortened by the table's pitch (octaves).
        const float pitch = std::max(0.0f, kVolumePitch[static_cast<std::size_t>(idx) * 2 + 1]);
        const double seconds = ch.effect->getDurationProperty().getTotalSecondsProperty() / std::pow(2.0, pitch);
        until = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seconds));
        outbox_->push_back({channel, false, x, y, z});
    }

    void Sound::PlayDirect(GalaxyEggbert::Def::SoundChannel channel, bool loop)
    {
        PlayChannel(channel, loop, false);
    }

    void Sound::PlayChannel(GalaxyEggbert::Def::SoundChannel channel, bool loop, bool decorSound)
    {
        if (replayMuted_) return;
        if (decorSound && blupiHidden_ && MutedWhileHidden(channel))
        {
            return;
        }
        using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
        using Microsoft::Xna::Framework::Audio::SoundState;

        if (!enabled_)
        {
            return;
        }
        const int idx = static_cast<int>(GalaxyEggbert::Def::ToRaw(channel));
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
        // itself if already playing, matching mobile-eggbert Sound.cpp.
        if (channel != GalaxyEggbert::Def::SoundChannel::SoundChannel10 && !loop &&
            ch.instance && ch.instance->getStateProperty() == SoundState::Playing)
        {
            return;
        }

        // Real bug fix (2026-07-11, reported live via a debugger backtrace:
        // crashes after a while with `System::InvalidOperationException`
        // from `SoundEffectInstance::setIsLoopedProperty`, thrown from
        // inside `Sound::Play`/`PlayStep`). setIsLoopedProperty() is only
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

        // The original's Sound::Play (mobile-eggbert Sound.cpp:96-110): the table's volume
        // multiplies, its pitch replaces the caller's (XNA's Pitch, octaves from -1 to 1, clamped
        // below at 0): most channels play 0.1-0.5 octave up, Blupi's push and pull efforts (38,
        // 39) and the life-loss sounds (1, 2) a whole octave up (BD-0031).
        const float volume = kVolumePitch[static_cast<std::size_t>(idx) * 2] * effectsVolume_;
        const float pitch = std::max(0.0f, kVolumePitch[static_cast<std::size_t>(idx) * 2 + 1]);
        ch.instance->setVolumeProperty(volume);
        ch.instance->setPitchProperty(pitch);
        ch.instance->Play();
    }

    void Sound::Stop(GalaxyEggbert::Def::SoundChannel channel)
    {
        const int idx = static_cast<int>(GalaxyEggbert::Def::ToRaw(channel));
        if (idx < 0 || idx >= kNumChannels)
        {
            return;
        }
        // Noted only for a sound noted as still playing (a loop's stop is his own, as its start).
        auto& until = notedUntil_[static_cast<std::size_t>(idx)];
        if (outbox_ != nullptr && std::chrono::steady_clock::now() < until)
        {
            until = {};
            outbox_->push_back({channel, true, listenerX_, listenerY_, listenerZ_});
        }
        Channel& ch = channels_[static_cast<std::size_t>(idx)];
        if (ch.instance)
        {
            ch.instance->Stop();
        }
    }

    void Sound::StopAll()
    {
        for (Channel& ch : channels_)
        {
            if (ch.instance)
            {
                ch.instance->Stop();
            }
        }
    }

    GalaxyEggbert::Def::SoundChannel Sound::CeilingChannelFor(std::uint16_t icon) noexcept
    {
        // SoundEnviron pairs each material's channel 3 sound with the next as its channel 4.
        const auto step = FootstepChannelFor(icon);
        if (step == GalaxyEggbert::Def::SoundChannel::SoundChannel3)
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel4;
        }
        return static_cast<GalaxyEggbert::Def::SoundChannel>(static_cast<int>(step) + 1);
    }

    GalaxyEggbert::Def::SoundChannel Sound::FootstepChannelFor(std::uint16_t icon) noexcept
    {
        using GalaxyEggbert::Def::SoundChannel;
        if (icon == 32 || icon == 33 || icon == 34 ||
            (icon >= 41 && icon <= 47) || (icon >= 139 && icon <= 143))
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel78;
        }
        if ((icon >= 1 && icon <= 28) || (icon >= 78 && icon <= 90) ||
            (icon >= 250 && icon <= 260) || (icon >= 311 && icon <= 316) ||
            (icon >= 324 && icon <= 329))
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel80;
        }
        if ((icon >= 284 && icon <= 303) || icon == 338)
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel82;
        }
        if (icon >= 341 && icon <= 363)
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel84;
        }
        if (icon >= 215 && icon <= 234)
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel86;
        }
        if (icon >= 246 && icon <= 249)
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel88;
        }
        if (icon >= 107 && icon <= 109)
        {
            return GalaxyEggbert::Def::SoundChannel::SoundChannel90;
        }
        return GalaxyEggbert::Def::SoundChannel::SoundChannel3;
    }
}
