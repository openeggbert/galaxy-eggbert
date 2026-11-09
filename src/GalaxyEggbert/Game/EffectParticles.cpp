#include <GalaxyEggbert/Game/EffectParticles.hpp>

#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::Game
{
    namespace
    {
        using Microsoft::Xna::Framework::Color;
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        using GalaxyEggbert::Def::ObjectType;

        // A fixed hash of (object, tick, piece, channel) to [0, 1): the same particles every run.
        float Random(int objectIndex, int tick, int piece, int channel) noexcept
        {
            std::uint64_t z = static_cast<std::uint64_t>(static_cast<std::uint32_t>(objectIndex)) * 0x9E3779B97F4A7C15ull ^
                              static_cast<std::uint64_t>(static_cast<std::uint32_t>(tick)) * 0xC2B2AE3D27D4EB4Full ^
                              static_cast<std::uint64_t>(piece * 131 + channel) * 0x165667B19E3779F9ull;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            z ^= z >> 31;
            return static_cast<float>(z >> 40) / static_cast<float>(1ull << 24);
        }

        float Between(float a, float b, float t) noexcept { return a + (b - a) * t; }

        // A direction on the unit sphere from two random numbers.
        Vector3 Direction(float u, float v) noexcept
        {
            const float z = u * 2.0f - 1.0f;
            const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
            const float a = v * 6.28318530718f;
            return Vector3(r * std::cos(a), z, r * std::sin(a));
        }

        Color Fade(const Color& tint, float alpha) noexcept
        {
            return Color(static_cast<int>(tint.getRProperty()), static_cast<int>(tint.getGProperty()),
                         static_cast<int>(tint.getBProperty()),
                         static_cast<int>(std::lround(std::clamp(alpha, 0.0f, 1.0f) * 255.0f)));
        }
    }

    bool EffectParticleKind(ObjectType type, ParticleKind& kind) noexcept
    {
        switch (type)
        {
            // The explosions (explo.png): the blast, the bombs, the dynamite.
            case ObjectType::ObjectType8:
            case ObjectType::ObjectType9:
            case ObjectType::ObjectType10:
            case ObjectType::ObjectType11:
                kind = ParticleKind::Fire;
                return true;
            // The splashes: object-m.png's and explo.png's.
            case ObjectType::ObjectType14:
            case ObjectType::ObjectType15:
            case ObjectType::ObjectType35:
            case ObjectType::ObjectType98:
            case ObjectType::ObjectType99:
            case ObjectType::ObjectType100:
                kind = ParticleKind::Splash;
                return true;
            // The sparkle trails, the bursts and the flashes.
            case ObjectType::ObjectType39:
            case ObjectType::ObjectType41:
            case ObjectType::ObjectType42:
            case ObjectType::ObjectType57:
            case ObjectType::ObjectType58:
            case ObjectType::ObjectType90:
            case ObjectType::ObjectType91:
            case ObjectType::ObjectType92:
            case ObjectType::ObjectType93:
                kind = ParticleKind::Sparkle;
                return true;
            default:
                return false;
        }
    }

    void EffectParticles::Clear()
    {
        m_particles.clear();
        m_tick = -1;
        m_stepped = 0;
    }

    void EffectParticles::Advance(int tick)
    {
        if (m_tick < 0 || tick < m_tick)
        {
            // The first frame, or the clock went back (a new mission, a restart): start afresh.
            m_particles.clear();
            m_tick = tick;
            m_stepped = 0;
            return;
        }
        // At most a second's worth at once (a long stall would otherwise burst everything out).
        m_stepped = std::min(tick - m_tick, 60);
        m_tick = tick;
        for (int step = 0; step < m_stepped; ++step)
        {
            for (auto& p : m_particles)
            {
                p.velocity = p.velocity * p.drag;
                p.velocity.Y -= p.gravity;
                p.position += p.velocity;
                p.size = std::max(0.02f, p.size + p.growth);
                p.rotation += p.spin;
                ++p.age;
            }
            m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
                                             [](const EffectParticle& p) { return p.age >= p.life; }),
                              m_particles.end());
        }
    }

    void EffectParticles::Feed(int objectIndex, ParticleKind kind, SceneSheet sheet, const CnaEngine::Graphics::UvRect& uv,
                               const Vector3& center)
    {
        // One emission per tick stepped, the earlier ones moved on by the ticks since.
        for (int back = m_stepped - 1; back >= 0; --back)
        {
            const std::size_t before = m_particles.size();
            Emit(objectIndex, m_tick - back, kind, sheet, uv, center);
            for (std::size_t i = before; i < m_particles.size(); ++i)
            {
                auto& p = m_particles[i];
                for (int step = 0; step < back; ++step)
                {
                    p.velocity = p.velocity * p.drag;
                    p.velocity.Y -= p.gravity;
                    p.position += p.velocity;
                    p.size = std::max(0.02f, p.size + p.growth);
                    p.rotation += p.spin;
                    ++p.age;
                }
            }
        }
    }

    void EffectParticles::Burst(ParticleKind kind, int seed, SceneSheet sheet, const CnaEngine::Graphics::UvRect& uv,
                                const Vector3& center, int count, const Color& tint)
    {
        if (m_stepped > 0 && count > 0)
        {
            Emit(100000 + seed, m_tick, kind, sheet, uv, center, count, tint);
        }
    }

    int EffectParticles::Crossed(int period, int phase) const noexcept
    {
        if (period <= 0 || m_stepped <= 0)
        {
            return 0;
        }
        const auto floorDiv = [period](int value) { return value >= 0 ? value / period : -((-value + period - 1) / period); };
        return floorDiv(m_tick - phase) - floorDiv(m_tick - m_stepped - phase);
    }

    void EffectParticles::Emit(int objectIndex, int tick, ParticleKind kind, SceneSheet sheet,
                               const CnaEngine::Graphics::UvRect& uv, const Vector3& center, int burst, const Color& tint)
    {
        if (m_particles.size() >= kMaxParticles)
        {
            return;
        }
        const auto random = [&](int piece, int channel) { return Random(objectIndex, tick, piece, channel); };
        int count = 0;
        switch (kind)
        {
            // A blast's pieces every tick it lasts, one more on even ticks; a splash's drops two a tick;
            // a sparkle's spark every other tick.
            case ParticleKind::Fire: count = 1 + (tick % 2 == 0 ? 1 : 0); break;
            case ParticleKind::Splash: count = 2; break;
            case ParticleKind::Sparkle: count = tick % 2 == 0 ? 1 : 0; break;
            default: count = 0; break;
        }
        if (burst >= 0)
        {
            count = burst;
        }
        for (int piece = 0; piece < count; ++piece)
        {
            EffectParticle p;
            p.sheet = sheet;
            p.uv = uv;
            p.tint = tint;
            const Vector3 direction = Direction(random(piece, 0), random(piece, 1));
            p.rotation = random(piece, 2) * 6.28318530718f;
            p.spin = Between(-0.12f, 0.12f, random(piece, 3));
            switch (kind)
            {
                case ParticleKind::Fire:
                    // Pieces of the blast thrown out in every direction, slowing, rising a little as hot
                    // air does, growing as they burn out.
                    p.position = center + direction * Between(0.05f, 0.2f, random(piece, 4));
                    p.velocity = direction * Between(0.025f, 0.06f, random(piece, 5)) + Vector3(0.0f, 0.01f, 0.0f);
                    p.drag = 0.92f;
                    p.gravity = -0.001f;
                    p.size = Between(0.28f, 0.46f, random(piece, 6));
                    p.growth = 0.008f;
                    p.life = static_cast<int>(Between(14.0f, 26.0f, random(piece, 7)));
                    break;
                case ParticleKind::Splash:
                    // Drops thrown up and out, falling back.
                    p.position = center + Vector3(direction.X * 0.25f, -0.2f, direction.Z * 0.25f);
                    p.velocity = Vector3(direction.X * 0.035f, Between(0.07f, 0.13f, random(piece, 5)), direction.Z * 0.035f);
                    p.drag = 0.985f;
                    p.gravity = 0.0085f;
                    p.size = Between(0.32f, 0.5f, random(piece, 6));
                    p.growth = -0.003f;
                    p.life = static_cast<int>(Between(20.0f, 30.0f, random(piece, 7)));
                    break;
                case ParticleKind::Sparkle:
                    // Sparks drifting off and up from the sparkle, twinkling smaller.
                    p.position = center + direction * 0.15f;
                    p.velocity = direction * 0.02f + Vector3(0.0f, 0.01f, 0.0f);
                    p.drag = 0.95f;
                    p.gravity = -0.0004f;
                    p.size = Between(0.22f, 0.36f, random(piece, 6));
                    p.growth = -0.008f;
                    p.life = static_cast<int>(Between(16.0f, 26.0f, random(piece, 7)));
                    break;
                case ParticleKind::Dust:
                    // Kicked up round his feet, spreading low and slowing, swelling as it thins.
                    p.position = center + Vector3(direction.X * 0.25f, 0.05f, direction.Z * 0.25f);
                    p.velocity = Vector3(direction.X * Between(0.018f, 0.04f, random(piece, 5)),
                                         Between(0.006f, 0.02f, random(piece, 4)),
                                         direction.Z * Between(0.018f, 0.04f, random(piece, 5)));
                    p.drag = 0.9f;
                    p.gravity = -0.0004f;
                    p.size = Between(0.5f, 0.75f, random(piece, 6));
                    p.growth = 0.012f;
                    p.life = static_cast<int>(Between(18.0f, 30.0f, random(piece, 7)));
                    break;
                case ParticleKind::Bubble:
                    // Rising faster as it goes, wobbling a little.
                    p.position = center + Vector3(direction.X * 0.12f, 0.0f, direction.Z * 0.12f);
                    p.velocity = Vector3(direction.X * 0.006f, Between(0.018f, 0.03f, random(piece, 5)), direction.Z * 0.006f);
                    p.drag = 1.0f;
                    p.gravity = -0.0006f;
                    p.size = Between(0.5f, 0.75f, random(piece, 6));
                    p.growth = 0.002f;
                    p.spin = 0.0f;
                    p.life = static_cast<int>(Between(40.0f, 60.0f, random(piece, 7)));
                    break;
                case ParticleKind::Ember:
                    // Off the lava's top, rising and drifting, burning out.
                    p.position = center + Vector3(Between(-0.45f, 0.45f, random(piece, 4)), 0.0f,
                                                  Between(-0.45f, 0.45f, random(piece, 8)));
                    p.velocity = Vector3(direction.X * 0.008f, Between(0.02f, 0.04f, random(piece, 5)), direction.Z * 0.008f);
                    p.drag = 0.985f;
                    p.gravity = -0.0002f;
                    p.size = Between(0.45f, 0.65f, random(piece, 6));
                    p.growth = -0.006f;
                    p.life = static_cast<int>(Between(35.0f, 55.0f, random(piece, 7)));
                    break;
                case ParticleKind::Flash:
                    // Stars thrown out every way, slowing, shrinking.
                    p.position = center + direction * 0.1f;
                    p.velocity = direction * Between(0.05f, 0.09f, random(piece, 5));
                    p.drag = 0.9f;
                    p.gravity = 0.0f;
                    p.size = Between(0.6f, 0.9f, random(piece, 6));
                    p.growth = -0.015f;
                    p.life = static_cast<int>(Between(18.0f, 28.0f, random(piece, 7)));
                    break;
            }
            p.glow = kind == ParticleKind::Fire                                     ? 2.5f
                     : kind == ParticleKind::Sparkle || kind == ParticleKind::Ember ? 3.0f
                     : kind == ParticleKind::Flash                                  ? 3.5f
                                                                                    : 1.0f;
            m_particles.push_back(p);
        }
    }

    void EffectParticles::Append(std::vector<SceneBillboardBatch>& batches, const Vector3& right, const Vector3& up,
                                 float lag) const
    {
        for (const auto& p : m_particles)
        {
            auto batch = std::find_if(batches.begin(), batches.end(),
                                      [&](const SceneBillboardBatch& b) { return b.sheet == p.sheet; });
            if (batch == batches.end())
            {
                batches.push_back(SceneBillboardBatch{p.sheet, 1.0f, {}});
                batch = batches.end() - 1;
            }
            const float t = static_cast<float>(p.age) / static_cast<float>(std::max(1, p.life));
            // Whole for its first half, then fading out.
            const float alpha = t < 0.5f ? 1.0f : 1.0f - (t - 0.5f) * 2.0f;
            SceneQuad quad;
            quad.center = p.position - p.velocity * lag;
            const float size = std::max(0.01f, p.size - p.growth * lag);
            quad.size = Vector2(size, size);
            quad.right = right;
            quad.up = up;
            quad.uv = p.uv;
            quad.rotation = p.rotation - p.spin * lag;
            quad.color = Fade(p.tint, alpha);
            quad.glow = p.glow;
            batch->quads.push_back(quad);
        }
    }
}
