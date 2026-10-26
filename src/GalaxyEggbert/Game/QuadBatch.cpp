#include "QuadBatch.hpp"

#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <cmath>

namespace GalaxyEggbert::Game::QuadBatch
{
    bool InRect(float x, float y, const Rect& r) noexcept
    {
        return x >= r.x0 && x < r.x1 && y >= r.y0 && y < r.y1;
    }

    void AppendQuadUv(CnaEngine::Graphics::MeshBuilder& mesh,
                      float x0, float y0, float x1, float y1,
                      float u0, float v0, float u1, float v1)
    {
        const std::uint32_t c0 = mesh.AddVertex(TexturedVertex({x0, y0, 0.0f}, {u0, v0}));
        const std::uint32_t c1 = mesh.AddVertex(TexturedVertex({x1, y0, 0.0f}, {u1, v0}));
        const std::uint32_t c2 = mesh.AddVertex(TexturedVertex({x1, y1, 0.0f}, {u1, v1}));
        const std::uint32_t c3 = mesh.AddVertex(TexturedVertex({x0, y1, 0.0f}, {u0, v1}));
        mesh.AddQuad(c0, c1, c2, c3);
    }

    void AppendRotatedQuadUv(CnaEngine::Graphics::MeshBuilder& mesh,
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

        const std::uint32_t c0 = mesh.AddVertex(TexturedVertex({x0, y0, 0.0f}, {u0, v0}));
        const std::uint32_t c1 = mesh.AddVertex(TexturedVertex({x1, y1, 0.0f}, {u1, v0}));
        const std::uint32_t c2 = mesh.AddVertex(TexturedVertex({x2, y2, 0.0f}, {u1, v1}));
        const std::uint32_t c3 = mesh.AddVertex(TexturedVertex({x3, y3, 0.0f}, {u0, v1}));
        mesh.AddQuad(c0, c1, c2, c3);
    }

    void FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                    Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                    std::unique_ptr<CnaEngine::Graphics::GpuMesh>& renderer,
                    const std::vector<Quad>& quads, int viewportW, int viewportH,
                    float alpha)
    {
        if (quads.empty())
        {
            return;
        }
        auto mesh = MakeTexturedMeshBuilder();
        for (const Quad& q : quads)
        {
            AppendQuadUv(mesh, q.x0, q.y0, q.x1, q.y1, q.u0, q.v0, q.u1, q.v1);
        }

        effect.World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.View = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect.Projection = Microsoft::Xna::Framework::Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(viewportW), static_cast<float>(viewportH), 0.0f, 0.0f, 1.0f);
        effect.setAlphaProperty(alpha);
        // BasicEffect premultiplies its colour by Alpha, as XNA's stock effects do, while every
        // overlay here blends NonPremultiplied over straight-alpha PNGs: the colour was scaled by
        // alpha twice, so the menu's 0.3 panels came out a dark veil where the original's
        // (premultiplied SpriteBatch) are a light one (2026-10-26, plan §10). Dividing the
        // diffuse colour by alpha leaves the texture's own colour, scaled once by the blend.
        const auto diffuse = effect.getDiffuseColorProperty();
        if (alpha > 0.0f && alpha < 1.0f)
        {
            effect.setDiffuseColorProperty(diffuse * (1.0f / alpha));
        }

        renderer = UploadTexturedMesh(device, mesh.Mesh());
        renderer->Draw(device, effect);
        effect.setAlphaProperty(1.0f);
        effect.setDiffuseColorProperty(diffuse);
    }
}
