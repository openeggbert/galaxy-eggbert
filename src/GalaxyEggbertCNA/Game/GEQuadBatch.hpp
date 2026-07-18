#pragma once

#include <Easy3D/BillboardMeshRenderer.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace GalaxyEggbert::CNA::GEQuadBatch
{
    // Generic 2D screen-space rect/quad drawing primitives, extracted from
    // GEInputPad (plan.md EDITOR-106) so the editor's own UI (palette,
    // toolbar) can reuse the exact same drawing approach instead of
    // inventing a new one. Stateless free functions -- no GraphicsDevice
    // ownership, no cached textures; callers supply everything each call,
    // same shape GEInputPad's own private helpers already had.

    // Axis-aligned rect in whatever 2D space the caller is working in
    // (GEInputPad uses its own fixed 640x480 reference space; the editor
    // UI below uses actual viewport pixels directly -- this type doesn't
    // care which).
    struct Rect
    {
        float x0, y0, x1, y1;
    };

    [[nodiscard]] bool InRect(float x, float y, const Rect& r) noexcept;

    // One screen-space quad queued for FlushQuads() -- position + UV only,
    // no per-quad color (draw color/alpha is a per-FlushQuads-call effect
    // uniform, same as GEInputPad's original shape).
    struct Quad
    {
        float x0, y0, x1, y1;
        float u0, v0, u1, v1;
    };

    // Appends one axis-aligned quad (2 triangles, 4 vertices) to existing
    // output arrays.
    void AppendQuadUv(std::vector<Easy3D::BillboardVertex>& vertices,
                      std::vector<std::uint32_t>& indices,
                      float x0, float y0, float x1, float y1,
                      float u0, float v0, float u1, float v1);

    // Appends one quad rotated about its own center by rotationDegrees
    // (positive = clockwise on screen, standard XNA SpriteBatch convention
    // in this Y-down space).
    void AppendRotatedQuadUv(std::vector<Easy3D::BillboardVertex>& vertices,
                             std::vector<std::uint32_t>& indices,
                             float centerX, float centerY, float halfW, float halfH,
                             float rotationDegrees,
                             float u0, float v0, float u1, float v1);

    // Builds a fresh BillboardMeshRenderer from @p quads and draws it with
    // an orthographic projection matching @p viewportW x @p viewportH (0,0
    // at the top-left, matching screen pixel coordinates) -- @p effect's
    // World/View/Projection/Alpha are set here; TextureEnabled/Texture/
    // VertexColorEnabled must already be configured by the caller. A no-op
    // if @p quads is empty (leaves @p renderer untouched). @p alpha is
    // restored to 1.0 on @p effect after drawing.
    void FlushQuads(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                    Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                    std::unique_ptr<Easy3D::BillboardMeshRenderer>& renderer,
                    const std::vector<Quad>& quads, int viewportW, int viewportH,
                    float alpha);
}
