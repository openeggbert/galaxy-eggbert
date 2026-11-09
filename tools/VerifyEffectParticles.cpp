// VerifyEffectParticles: the 3D effects' particles (EffectParticles.hpp) are stepped by the simulation's
// ticks, the same every run and whatever the frame rate, and only the effect objects become particles.

#include <GalaxyEggbert/Game/EffectParticles.hpp>

#include <cmath>
#include <cstdio>

namespace
{
    using GalaxyEggbert::Def::ObjectType;
    using GalaxyEggbert::Game::EffectParticleKind;
    using GalaxyEggbert::Game::EffectParticles;
    using GalaxyEggbert::Game::ParticleKind;
    using GalaxyEggbert::Game::SceneSheet;
    using Microsoft::Xna::Framework::Vector3;

    int failures = 0;

    void Check(bool ok, const char* what)
    {
        std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
        if (!ok)
        {
            ++failures;
        }
    }

    const CnaEngine::Graphics::UvRect kUv{0.1f, 0.2f, 0.3f, 0.4f};

    void Feed(EffectParticles& particles, int index, ParticleKind kind)
    {
        particles.Feed(index, kind, SceneSheet::Explo, kUv, Vector3(1.0f, 2.0f, 3.0f));
    }

    bool Same(const EffectParticles& a, const EffectParticles& b)
    {
        if (a.Count() != b.Count())
        {
            return false;
        }
        for (std::size_t i = 0; i < a.Count(); ++i)
        {
            const auto& p = a.Particles()[i];
            const auto& q = b.Particles()[i];
            if (p.age != q.age || p.life != q.life || std::fabs(p.position.X - q.position.X) > 1e-5f ||
                std::fabs(p.position.Y - q.position.Y) > 1e-5f || std::fabs(p.position.Z - q.position.Z) > 1e-5f ||
                std::fabs(p.size - q.size) > 1e-5f)
            {
                return false;
            }
        }
        return true;
    }
}

int main()
{
    ParticleKind kind{};
    Check(EffectParticleKind(ObjectType::ObjectType8, kind) && kind == ParticleKind::Fire, "an explosion (8) is fire");
    Check(EffectParticleKind(ObjectType::ObjectType98, kind) && kind == ParticleKind::Splash, "a splash (98) is water");
    Check(EffectParticleKind(ObjectType::ObjectType39, kind) && kind == ParticleKind::Sparkle, "a sparkle (39) sparks");
    Check(!EffectParticleKind(ObjectType::ObjectType36, kind), "the cloud puff (36) keeps its sprite only");
    Check(!EffectParticleKind(ObjectType::ObjectType53, kind), "the tentacle (53) is no particle");
    Check(!EffectParticleKind(ObjectType::ObjectType5, kind), "a treasure (5) is no particle");

    // The same ticks and feeds, the same particles.
    EffectParticles a;
    EffectParticles b;
    for (int tick = 100; tick < 130; ++tick)
    {
        a.Advance(tick);
        b.Advance(tick);
        Feed(a, 3, ParticleKind::Fire);
        Feed(b, 3, ParticleKind::Fire);
    }
    Check(a.Count() > 0 && Same(a, b), "the same ticks give the same particles");

    // A frame in the same tick (or paused) emits and moves nothing.
    const auto before = a.Particles();
    a.Advance(129);
    Feed(a, 3, ParticleKind::Fire);
    bool unchanged = a.Count() == before.size();
    for (std::size_t i = 0; unchanged && i < before.size(); ++i)
    {
        unchanged = a.Particles()[i].position.X == before[i].position.X && a.Particles()[i].age == before[i].age;
    }
    Check(unchanged, "a second frame in the same tick changes nothing");

    // Two ticks in one frame come out as two frames of one tick each.
    EffectParticles perTick;
    EffectParticles perTwo;
    perTick.Advance(10);
    perTwo.Advance(10);
    for (int tick = 11; tick <= 30; ++tick)
    {
        perTick.Advance(tick);
        Feed(perTick, 1, ParticleKind::Splash);
        if (tick % 2 == 0)
        {
            perTwo.Advance(tick);
            Feed(perTwo, 1, ParticleKind::Splash);
        }
    }
    Check(perTick.Count() > 0 && Same(perTick, perTwo), "the frame rate does not change the particles");

    // Left alone they burn out; a tick going back (a new mission) clears them.
    EffectParticles dying;
    dying.Advance(0);
    dying.Advance(1);
    Feed(dying, 0, ParticleKind::Fire);
    Check(dying.Count() > 0, "an explosion emits");
    dying.Advance(61);
    Check(dying.Count() == 0, "its particles are gone a second later");
    Feed(dying, 0, ParticleKind::Fire);
    dying.Advance(5);
    Check(dying.Count() == 0, "the clock going back clears them");

    // Never more than the cap, however many explosions.
    EffectParticles many;
    many.Advance(0);
    for (int tick = 1; tick < 40; ++tick)
    {
        many.Advance(tick);
        for (int index = 0; index < 200; ++index)
        {
            Feed(many, index, ParticleKind::Fire);
        }
    }
    Check(many.Count() <= EffectParticles::kMaxParticles + 2, "the particles stay under their cap");

    // Bursts (Blupi's moments): once per frame whose tick moved on, none while paused; the periodic
    // ones count the multiples the stepped ticks crossed, whatever the frame rate.
    EffectParticles moments;
    moments.Advance(10);
    moments.Burst(ParticleKind::Dust, 1, SceneSheet::Element, kUv, Vector3(0.0f, 0.0f, 0.0f), 10);
    Check(moments.Count() == 0, "no burst in the frame that starts the clock");
    moments.Advance(11);
    moments.Burst(ParticleKind::Dust, 1, SceneSheet::Element, kUv, Vector3(0.0f, 0.0f, 0.0f), 10);
    Check(moments.Count() == 10, "a burst of ten after a tick");
    moments.Advance(11);
    moments.Burst(ParticleKind::Dust, 1, SceneSheet::Element, kUv, Vector3(0.0f, 0.0f, 0.0f), 10);
    Check(moments.Count() == 10, "no burst while the tick stands still");
    EffectParticles everyTick;
    EffectParticles everyFour;
    everyTick.Advance(0);
    everyFour.Advance(0);
    int tickCount = 0;
    int fourCount = 0;
    for (int tick = 1; tick <= 36; ++tick)
    {
        everyTick.Advance(tick);
        tickCount += everyTick.Crossed(9);
        if (tick % 4 == 0)
        {
            everyFour.Advance(tick);
            fourCount += everyFour.Crossed(9);
        }
    }
    Check(tickCount == 4 && fourCount == 4, "a period of 9 crossed four times in 36 ticks at any frame rate");

    std::printf("%s\n", failures == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
