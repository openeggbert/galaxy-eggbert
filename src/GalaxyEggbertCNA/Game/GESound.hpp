#pragma once

#include <GalaxyEggbert/def/SoundChannel.hpp>

#include <Microsoft/Xna/Framework/Audio/SoundEffect.hpp>
#include <Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace GalaxyEggbert::CNA
{
    // Real mobile-eggbert sound playback for GalaxyEggbertCNA (2026-07-10).
    // Same 93 real WAV files (Content/sounds/sound000.wav..sound092.wav,
    // already copied build-time from mobile-eggbert alongside icons/
    // backgrounds) and the real per-channel volume table and conflict
    // policy documented in mobile-eggbert-reference/07-sounds.md and
    // sourced from Sound.hpp/Sound.cpp. Pitch is intentionally NOT applied
    // yet --
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

        // Conflict policy matches mobile-eggbert Sound.cpp: a channel
        // already playing is not restarted, except
        // channel 10. A channel with no loaded file (missing WAV) is a
        // silent no-op, not an error.
        void Play(GalaxyEggbert::SoundChannel channel, bool loop = false);
        // Stops just one channel's instance (a no-op if it isn't currently playing/doesn't
        // exist) -- needed for real looped sounds with a distinct stop trigger (e.g. the
        // crate-push loop, ch38) where StopAll() would incorrectly also kill every other
        // currently-playing sound.
        void Stop(GalaxyEggbert::SoundChannel channel);
        void StopAll();

        void SetEnabled(bool enabled) { enabled_ = enabled; if (!enabled) StopAll(); }
        [[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

        // Named shortcuts for movement/jump/landing events. The complete
        // real channel map is maintained in
        // mobile-eggbert-reference/07-sounds.md.
        void PlayJump() { Play(GalaxyEggbert::SoundChannel::SoundChannel1); }

        // Real mobile-eggbert (mobile-eggbert-reference/07-sounds.md,
        // channels 0-9): channel 3 covers BOTH footstep AND landing -- one
        // shared sound, not two separate events. Channel 4 is a distinct
        // real event (head-bump/ceiling-hit, triggered when Blupi's upward
        // jump arc hits an obstacle above) that has nothing to do with
        // landing. This class's original `PlayLand()` played channel 4 -- a
        // real, confirmed bug found by the independently verified channel
        // research in 07-sounds.md, not a deliberate choice.
        // Fixed here to channel 3, matching PlayStep() (plan.md
        // E3D-MIG-084's terrain-remap task surfaced this while researching
        // the real channels 78-91 the same shared sound remaps to).
        // Head-bump (channel 4 and its own 79/81/83/85/87/89/91 terrain
        // remaps) stays unwired -- there is no ceiling-hit detection in
        // GEBlupiController yet, a separate not-yet-implemented mechanic.
        //
        // groundIcon is the tile icon underfoot (GEBlupiController::
        // GetGroundBlockType()) -- remapped via FootstepChannelFor() to one
        // of the 7 real terrain-specific variants when it falls in one of
        // Decor::SoundEnviron()'s real icon ranges, channel 3 (this same
        // generic sound) otherwise.
        void PlayStep(std::uint16_t groundIcon) { Play(FootstepChannelFor(groundIcon)); }
        void PlayLand(std::uint16_t groundIcon) { Play(FootstepChannelFor(groundIcon)); }

        // Decor::SoundEnviron()'s real landing/footstep terrain remap
        // (mobile-eggbert-reference/07-sounds.md channels 78-91): 7 tile-
        // icon ranges, each with its own .wav: 78 (32-34,41-47,139-143), 80
        // (1-28,78-90,250-260,311-316,324-329), 82 (284-303,338), 84
        // (341-363), 86 (215-234), 88 (246-249), 90 (107-109). Falls back
        // to the generic channel 3 for every icon outside all 7 ranges.
        [[nodiscard]] static GalaxyEggbert::SoundChannel FootstepChannelFor(std::uint16_t icon) noexcept;

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
