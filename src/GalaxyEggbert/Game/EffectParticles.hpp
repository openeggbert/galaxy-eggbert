#pragma once

// The 3D effects' particles (the Setup's "3D effects"): the explosions, splashes and sparks thrown into
// the world as particles instead of one flat billboard each. A particle is a small copy of its effect's
// own sprite frame -- the original art, its colours and soft edges -- flying, turning and fading in 3D.
// Presentation only: the effect objects live and act exactly as before; only how they look changes.
//
// Stepped by the simulation's ticks (Advance), so a replay or a screenshot at a tick is the same every
// run, and nothing moves while the game is paused.

#include <GalaxyEggbert/Def/ObjectType.hpp>
#include <GalaxyEggbert/Game/SceneFrame.hpp>

#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game
{
    enum class ParticleKind : std::uint8_t
    {
        Fire,    // explosions: a ball of flame pieces bursting out, rising, growing, fading
        Splash,  // water thrown up and falling back
        Sparkle, // sparks and stars drifting off (the sprite is kept, the sparks trail it)
        // Blupi's and the world's own moments (presentation only, Burst()):
        Dust,    // a puff of dust where he lands or runs
        Bubble,  // a bubble rising from him under water
        Ember,   // a spark rising from the lava
        Flash,   // stars bursting out as he teleports
    };

    // Whether an effect object becomes particles, and which kind; false keeps its billboard alone.
    [[nodiscard]] bool EffectParticleKind(GalaxyEggbert::Def::ObjectType type, ParticleKind& kind) noexcept;
    // Whether the effect's own billboard gives way to its particles (fire, splashes) or stays (sparkles).
    [[nodiscard]] constexpr bool ParticlesReplaceSprite(ParticleKind kind) noexcept
    {
        return kind != ParticleKind::Sparkle;
    }

    struct EffectParticle
    {
        SceneSheet sheet = SceneSheet::Explo;
        CnaEngine::Graphics::UvRect uv;
        Microsoft::Xna::Framework::Vector3 position;
        Microsoft::Xna::Framework::Vector3 velocity;
        float size = 0.4f;    // blocks, at birth
        float growth = 0.0f;  // blocks a tick
        float rotation = 0.0f;
        float spin = 0.0f;    // radians a tick
        float gravity = 0.0f; // blocks a tick a tick, down (negative rises)
        float drag = 1.0f;    // the velocity kept each tick
        int age = 0;
        int life = 30;        // ticks
        Microsoft::Xna::Framework::Color tint = Microsoft::Xna::Framework::Color::White;
    };

    class EffectParticles
    {
    public:
        // At the simulation's tick `tick`: steps every particle through the ticks since the last call (none
        // in the same tick or while paused) and drops the dead. A tick going back (a new mission, a
        // restart) clears them.
        void Advance(int tick);
        // An effect object shown this frame: `objectIndex` its slot, its sprite's sheet, rectangle and
        // centre. Emits its pieces for each tick Advance() stepped.
        void Feed(int objectIndex, ParticleKind kind, SceneSheet sheet, const CnaEngine::Graphics::UvRect& uv,
                  const Microsoft::Xna::Framework::Vector3& center);
        // A burst of `count` particles of `kind` from `center`, cut from `uv` on `sheet` and tinted, once
        // per call in a frame whose tick moved on (none while paused); `seed` keeps bursts apart.
        void Burst(ParticleKind kind, int seed, SceneSheet sheet, const CnaEngine::Graphics::UvRect& uv,
                   const Microsoft::Xna::Framework::Vector3& center, int count,
                   const Microsoft::Xna::Framework::Color& tint = Microsoft::Xna::Framework::Color::White);
        // How many multiples of `period` (plus `phase`) the ticks stepped this frame crossed: a periodic
        // emission that does not depend on the frame rate.
        [[nodiscard]] int Crossed(int period, int phase = 0) const noexcept;
        [[nodiscard]] int Stepped() const noexcept { return m_stepped; }
        [[nodiscard]] int Tick() const noexcept { return m_tick; }
        // The particles as camera-facing quads, in batches by sheet.
        void Append(std::vector<SceneBillboardBatch>& batches, const Microsoft::Xna::Framework::Vector3& right,
                    const Microsoft::Xna::Framework::Vector3& up) const;
        void Clear();

        [[nodiscard]] std::size_t Count() const noexcept { return m_particles.size(); }
        [[nodiscard]] const std::vector<EffectParticle>& Particles() const noexcept { return m_particles; }

        static constexpr std::size_t kMaxParticles = 1500;

    private:
        void Emit(int objectIndex, int tick, ParticleKind kind, SceneSheet sheet, const CnaEngine::Graphics::UvRect& uv,
                  const Microsoft::Xna::Framework::Vector3& center, int count = -1,
                  const Microsoft::Xna::Framework::Color& tint = Microsoft::Xna::Framework::Color::White);

        int m_tick = -1;
        int m_stepped = 0;
        std::vector<EffectParticle> m_particles;
    };
}
