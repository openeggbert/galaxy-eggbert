#pragma once

#include <list>

#include "IGame1.hpp"
#include "GameData.hpp"
#include "ISound.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"

namespace GalaxyEggbert
{
    /**
     * @class Sound
     * @brief The Sound class is responsible for managing audio playback and controlling sound attributes.
     *
     * This class provides functionality to either load, play, pause, stop, and manage audio files.
     * It also allows configuration and control over sound properties such as volume and looping.
     */
    class Sound : public ISound
    {
        class Play
        {
            Microsoft::Xna::Framework::Audio::SoundEffectInstance sei;
            const SoundChannel channel;

        public:
            [[nodiscard]] SoundChannel getChannelProperty() const;

            [[nodiscard]] bool getIsFreeProperty() const;

            Play(Microsoft::Xna::Framework::Audio::SoundEffect& se, SoundChannel channel, double volume, double balance,
                 double pitch, bool isLooped);

            void Stop();
        };

        static constexpr short tableVolumePitchLength = 200;

        /**
         * @var tableVolumePitch
         * @brief A lookup table containing preset volume and pitch adjustment values for audio playback.
         *
         * This static and constant array is used to determine the volume scalar and pitch values
         * for sound effects during playback. It provides efficient access to predefined adjustments
         * to ensure consistent audio behavior across channels and effects.
         *
         * Each pair of elements in the array corresponds to a specific audio configuration,
         * where the first value represents a volume multiplier and the second value represents
         * the pitch adjustment factor.
         */
        static constexpr double tableVolumePitch[tableVolumePitchLength] =
        {
            1.0, 0.0, 0.5, 1.0, 0.5, 1.0, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.1, 1.0, 0.3, 1.0, 0.2, 1.0, 0.3, 1.0, 0.5,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.1, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 0.7, 0.2, 1.0, 0.1, 1.0, 0.1,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.4, 1.0, 0.0, 1.0, 0.0,
            1.0, 0.0, 1.0, 0.0, 1.0, 0.2, 1.0, 0.2, 0.7, 0.4,
            1.0, 0.2, 1.0, 0.4, 1.0, 0.2, 0.5, 1.0, 0.5, 1.0,
            1.0, 0.4, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 0.6, 0.4, 0.8, 0.1,
            0.6, 0.5, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.0,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.0,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.0, 1.0, 0.0, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 0.6, 0.4,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2,
            1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2, 1.0, 0.2
        };

    public:
        virtual ~Sound() = default;

        static constexpr int MAXVOLUME = 20;

    private:
        IGame1* game1;

        const GameData& gameData;

        std::vector<Microsoft::Xna::Framework::Audio::SoundEffect> soundEffects;

        std::list<Play> plays;

        double volume;

    public:
        Sound(IGame1* game1, GameData& gameData);
        Sound(const Sound&) = delete;
        Sound& operator=(const Sound&) = delete;

        void LoadContent() override;

        bool Create() override;

        void SetState(bool bState) override;

        void SetCDAudio(bool bAudio) override;

        bool GetEnable() override;

        void SetAudioVolume(int volume) override;

        int GetAudioVolume() override;

        void SetMidiVolume(int volume) override;

        int GetMidiVolume() override;

        void StopAll() override;

        bool PlayImage(SoundChannel channel, TinyPoint pos, int rank = -1, bool bLoop = false) override;

        bool PosImage(SoundChannel channel, TinyPoint pos) override;

        bool Stop(SoundChannel channel) override;

    private:
        double GetVolume(TinyPoint pos);
        double GetBalance(TinyPoint pos);
    };
}
