#include "QuadBatch.hpp"

#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <cmath>

namespace GalaxyEggbert::Game::QuadBatch
{
    bool InRect(float x, float y, const Rect& r) noexcept
    {
        return x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1;
    }

    void AppendQuadUv(std::vector<Easy3D::BillboardVertex>& vertices,
                      std::vector<std::uint32_t>& indices,
                      float x0, float y0, float x1, float y1,
                      float u0, float v0, float u1, float v1)
    {
        const auto base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back({{x0, y0, 0.0f}, {u0, v0}});
        vertices.push_back({{x1, y0, 0.0f}, {u1, v0}});
        vertices.push_back({{x1, y1, 0.0f}, {u1, v1}});
        vertices.push_back({{x0, y1, 0.0f}, {u0, v1}});
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    void AppendRotatedQuadUv(std::vector<Easy3D::BillboardVertex>& vertices,
                             std::vector<std::uint32_t>& indices,
                             float centerX, float centerY, float halfW, float halfH,
                             float rotationDegrees,
                             float u0, float v0, float u1, float v1)
    {
        const float rad = rotationDegrees * (3.14159265f / 180.0f);
        const float c = std::cos(rad);
        const float s = std::sin(rad);
        const auto rotate = [&](float dx, float dy, float& outX, float& outY)
        {
            outX = centerX + dx * c - dy * s;
            outY = centerY + dx * s + dy * c;
        };
        float x0, y0, x1, y1, x2, y2, x3, y3;
        rotate(-halfW, -halfH, x0, y0);
        rotate(halfW, -halfH, x1, y1);
        rotate(halfW, halfH, x2, y2);
        rotate(-halfW, halfH, x3, y3);

        const auto base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back({{x0, y0, 0.0f}, {u0, v0}});
        vertices.push_back({{x1, y1, 0.0f}, {u1, v0}});
        vertices.push_back({{x2, y2, 0.0f}, {u1, v1}});
        vertices.push_back({{x3, y3, 0.0f}, {u0, v1}});
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    void FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                    Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                    std::unique_ptr<Easy3D::BillboardMeshRenderer>& renderer,
                    const std::vector<Quad>& quads, int viewportW, int viewportH,
                    float alpha)
    {
        if (quads.empty())
        {
            return;
        }
        std::vector<Easy3D::BillboardVertex> vertices;
        std::vector<std::uint32_t> indices;
        vertices.reserve(quads.size() * 4);
        indices.reserve(quads.size() * 6);
        for (const Quad& q : quads)
        {
            AppendQuadUv(vertices, indices, q.x0, q.y0, q.x1, q.y1, q.u0, q.v0, q.u1, q.v1);
        }

        effect.World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
        effect.setAlphaProperty(alpha);

        renderer = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
        renderer->Draw(device, effect);
        effect.setAlphaProperty(1.0f);
    }
}
