#pragma once

#include <GalaxyEggbert/def/SoundChannel.hpp>

#include <Microsoft/Xna/Framework/Audio/SoundEffect.hpp>
#include <Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp>

#include <array>
#include <memory>

namespace GalaxyEggbert::CNA
{
    // Real mobile-eggbert sound playback for GalaxyEggbertCNA (2026-07-10).
    // Same 93 real WAV files (Content/sounds/sound000.wav..sound092.wav,
    // already copied build-time from mobile-eggbert alongside icons/
    // backgrounds -- see CLAUDE.md's asset-reuse table, "Sounds: Direct
    // asset reuse -- same WAV files, same indices") and the same real
    // per-channel volume table and conflict policy as
    // GalaxyEggbertSimple3D's already-shipped GESound
    // (src/GalaxyEggbertSimple3D/Game/GESound.cpp, whose own comment says
    // its volume/pitch table was "ported verbatim from mobile-eggbert
    // Sound.hpp tableVolumePitch") -- reused here as the same real data,
    // not a fresh transcription. Pitch is intentionally NOT applied yet
    // (matches Simple3D's own "(ignored in Simple3D)" simplification) --
    // CNA's SoundEffectInstance does support a Pitch property, but the
    // table's exact pitch encoding/units were never independently verified
    // against real mobile-eggbert source, so applying it here without that
    // verification would risk audibly wrong pitch shifts rather than
    // silence.
    class GESound
    {
    public:
        static constexpr int kNumChannels = 93;

        // Loads every Content/sounds/soundNNN.wav that actually exists
        // (0..92) -- a missing index is skipped, not an error (mirrors
        // GalaxyEggbertCnaGame::LoadContent's background-image loader,
        // which degrades the same way for missing region files).
        void LoadContent();

        [[nodiscard]] int LoadedCount() const noexcept;

        // Conflict policy matches Simple3D's GESound exactly (itself
        // matching real mobile-eggbert Sound.cpp behavior per that file's
        // own comment): a channel already playing is not restarted, except
        // channel 10. A channel with no loaded file (missing WAV) is a
        // silent no-op, not an error.
        void Play(GalaxyEggbert::SoundChannel channel, bool loop = false);
        void StopAll();

        void SetEnabled(bool enabled) { enabled_ = enabled; if (!enabled) StopAll(); }
        [[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

        // Named shortcuts for the events GalaxyEggbertCnaGame can actually
        // trigger today (movement/jump/landing -- there is no interactive-
        // object system yet, so pickup/key/hazard sounds aren't wired up
        // here; see Simple3D's GESound for the full real channel list once
        // that system exists). Same real channel assignments as Simple3D's
        // GESound.
        void PlayJump() { Play(GalaxyEggbert::SoundChannel::SoundChannel1); }
        void PlayStep() { Play(GalaxyEggbert::SoundChannel::SoundChannel3); }
        void PlayLand() { Play(GalaxyEggbert::SoundChannel::SoundChannel4); }

    private:
        struct Channel
        {
            std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> effect;
            std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> instance;
        };

        std::array<Channel, kNumChannels> channels_;
        bool enabled_ = true;
    };
}
