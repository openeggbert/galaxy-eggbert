#pragma once

namespace GalaxyEggbert
{
    enum class SoundChannel : SharpRuntime::ushortcs;

    class ISound
    {
    public:
        virtual ~ISound() = default;
        virtual void LoadContent() = 0;

        virtual bool Create() = 0;

        virtual void SetState(bool bState) = 0;

        virtual void SetCDAudio(bool bAudio) = 0;

        virtual bool GetEnable() = 0;

        virtual void SetAudioVolume(int volume) = 0;

        virtual int GetAudioVolume() = 0;

        virtual void SetMidiVolume(int volume) = 0;

        virtual int GetMidiVolume() = 0;

        virtual void StopAll() = 0;

        virtual bool PlayImage(SoundChannel channel, TinyPoint pos, int rank = -1, bool bLoop = false) = 0;

        virtual bool PosImage(SoundChannel channel, TinyPoint pos) = 0;

        virtual bool Stop(SoundChannel channel) = 0;
    };
}
