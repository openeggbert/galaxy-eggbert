#pragma once

#include <GalaxyEggbert/Def/SoundChannel.hpp>

#include <Microsoft/Xna/Framework/Audio/SoundEffect.hpp>
#include <Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace GalaxyEggbert::Game
{
    // Real mobile-eggbert sound playback for GalaxyEggbertCNA (2026-07-10).
    // Same 93 real WAV files (Content/sounds/sound000.wav..sound092.wav,
    // already copied build-time from mobile-eggbert alongside icons/
    // backgrounds) and the real per-channel volume table and conflict
    // policy documented in mobile-eggbert-reference/07-sounds.md and
    // sourced from Sound.hpp/Sound.cpp. The table's pitch is applied as the
    // original applies it (Sound::Play: `sei.setPitchProperty(pitch)`, XNA's
    // octaves, BD-0031); it was left out until 2026-11-02, so every effect
    // played lower than the original's, Blupi's crate efforts a whole octave.
    class Sound
    {
    public:
        static constexpr int kNumChannels = 93;

        // Loads every Content/sounds/soundNNN.wav that actually exists
        // (0..92) -- a missing index is skipped, not an error (mirrors
        // GalaxyEggbertGame::LoadContent's background-image loader,
        // which degrades the same way for missing region files).
        void LoadContent();

        [[nodiscard]] int LoadedCount() const noexcept;

        // Conflict policy matches mobile-eggbert Sound.cpp: a channel
        // already playing is not restarted, except
        // channel 10. A channel with no loaded file (missing WAV) is a
        // silent no-op, not an error.
        void Play(GalaxyEggbert::Def::SoundChannel channel, bool loop = false);
        // As Play(), past the hidden-Blupi filter below: the original's sounds played by
        // m_sound->PlayImage directly, not through Decor::PlaySound (the voyages' arrival, 3).
        void PlayDirect(GalaxyEggbert::Def::SoundChannel channel, bool loop = false);

        // While Blupi is hidden (the lollipop's Hide power) the original's Decor::PlaySound
        // (Decor.cpp:1518-1572) plays none of his own sounds -- his steps, landings, jumps, the
        // water's, the crates' efforts, the taps and falls (BD-0075); the host sets it every frame
        // of play.
        void SetBlupiHidden(bool hidden) noexcept { blupiHidden_ = hidden; }
        // The channels Decor::PlaySound leaves out while m_blupiHide.
        [[nodiscard]] static bool MutedWhileHidden(GalaxyEggbert::Def::SoundChannel channel) noexcept;
        // Stops just one channel's instance (a no-op if it isn't currently playing/doesn't
        // exist) -- needed for sounds with a distinct stop trigger (e.g. the crate push's
        // ch38, cut when the push ends) where StopAll() would incorrectly also kill every other
        // currently-playing sound.
        void Stop(GalaxyEggbert::Def::SoundChannel channel);
        void StopAll();

        void SetEnabled(bool enabled) { enabled_ = enabled; if (!enabled) StopAll(); }
        [[nodiscard]] bool IsEnabled() const noexcept { return enabled_; }

        // The effects' volume from Setup (0 silent .. 1 as the original), multiplying every
        // channel's table volume from the next Play() on (a Galaxy addition, the owner's request
        // of 2026-11-02: the original has only Sounds on/off).
        void SetEffectsVolume(float volume) noexcept { effectsVolume_ = volume < 0.0f ? 0.0f : volume > 1.0f ? 1.0f : volume; }
        [[nodiscard]] float GetEffectsVolume() const noexcept { return effectsVolume_; }

        // Named shortcuts for movement/jump/landing events. The complete
        // real channel map is maintained in
        // mobile-eggbert-reference/07-sounds.md.
        void PlayJump() { Play(GalaxyEggbert::Def::SoundChannel::SoundChannel1); }

        // Real mobile-eggbert (mobile-eggbert-reference/07-sounds.md,
        // channels 0-9): channel 3 is the landing (no footsteps while walking: BD-0037) -- one
        // shared sound, not two separate events. Channel 4 is a distinct
        // real event (head-bump/ceiling-hit, triggered when Blupi's upward
        // jump arc hits an obstacle above) that has nothing to do with
        // landing. This class's original `PlayLand()` played channel 4 -- a
        // real, confirmed bug found by the independently verified channel
        // research in 07-sounds.md, not a deliberate choice.
        // Fixed here to channel 3, matching the landing (plan.md
        // E3D-MIG-084's terrain-remap task surfaced this while researching
        // the real channels 78-91 the same shared sound remaps to).
        // Channel 4 and its 79/81/83/85/87/89/91 remaps: a ceiling met rising
        // (CeilingChannelFor; BlupiController's CeilingTapped).
        //
        // groundIcon is the tile icon underfoot (BlupiController::
        // GetGroundBlockType()) -- remapped via FootstepChannelFor() to one
        // of the 7 real terrain-specific variants when it falls in one of
        // Decor::SoundEnviron()'s real icon ranges, channel 3 (this same
        // generic sound) otherwise.
        void PlayLand(std::uint16_t groundIcon) { Play(FootstepChannelFor(groundIcon)); }

        // Decor::SoundEnviron()'s real landing/footstep terrain remap
        // (mobile-eggbert-reference/07-sounds.md channels 78-91): 7 tile-
        // icon ranges, each with its own .wav: 78 (32-34,41-47,139-143), 80
        // (1-28,78-90,250-260,311-316,324-329), 82 (284-303,338), 84
        // (341-363), 86 (215-234), 88 (246-249), 90 (107-109). Falls back
        // to the generic channel 3 for every icon outside all 7 ranges.
        [[nodiscard]] static GalaxyEggbert::Def::SoundChannel FootstepChannelFor(std::uint16_t icon) noexcept;
        // The same remap of channel 4, the tap of a ceiling met rising (Decor.cpp:2961): each
        // material's second sound (79, 81, ... 91), channel 4 outside every range.
        [[nodiscard]] static GalaxyEggbert::Def::SoundChannel CeilingChannelFor(std::uint16_t icon) noexcept;

    private:
        struct Channel
        {
            std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffect> effect;
            std::unique_ptr<Microsoft::Xna::Framework::Audio::SoundEffectInstance> instance;
        };

        std::array<Channel, kNumChannels> channels_;
        bool enabled_ = true;
        float effectsVolume_ = 1.0f;
        bool blupiHidden_ = false;
        void PlayChannel(GalaxyEggbert::Def::SoundChannel channel, bool loop, bool decorSound);
    };
}
