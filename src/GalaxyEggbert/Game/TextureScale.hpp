#pragma once

// 2x textures (2026-11-09, ../ray.md): Content/icons2x/ and Content/backgrounds2x/ hold the sheets at
// twice the size, re-rendered from the original Ray Dream scenes (speedyblupi-hd). A sheet's layout is
// its 1x layout times two, gaps included, so code that maps logical (1x) pixels to texture coordinates
// only has to divide by the sheet's logical size: LogicalWidth / LogicalHeight below, never the
// texture's own size.
//
// Which size a run loads: the Setup's "Textures" (SaveData::GetTextureScale: 0 auto, 1, 2), or
// GALAXY_TEXTURES=1x / 2x / auto for tests. Galaxy draws on CNA's 800 x 480 logical surface, scaled up to
// the window; only the 3D effects' Ultra draws the world at the window's own pixels (the HUD stays on the
// logical surface). So 2x shows where Ultra draws on a large screen, and in close-ups. Auto: 2x on
// desktop with Ultra on a display at least 1000 pixels tall; 1x otherwise, and always on the web and on
// Android. Chosen once, before the textures load; a change applies at the next start.

#include <string>

namespace GalaxyEggbert::Game
{
    // The setting (0 auto, 1, 2).
    void SetTextureScaleSetting(int setting) noexcept;
    [[nodiscard]] int TextureScaleSetting() noexcept;
    // Resolves the setting (and GALAXY_TEXTURES) for a display `displayHeight` pixels tall, `nativeWorld`
    // when the world is drawn at the window's pixels (Ultra); the result is what ScaledContentPath loads
    // from now on. Returns 1 or 2.
    int ResolveTextureScale(int displayHeight, bool nativeWorld) noexcept;
    [[nodiscard]] int WantedTextureScale() noexcept;

    // "Content/icons/x.png" -> "Content/icons2x/x.png" (or backgrounds -> backgrounds2x) when 2x is
    // wanted and that file exists at exactly twice the 1x file's size (their PNG headers); otherwise the
    // path itself. *scale (optional) gets 2 or 1.
    [[nodiscard]] std::string ScaledContentPath(const std::string& path, int* scale = nullptr);

    // The scale a loaded texture was loaded at, by its address (default 1).
    void SetTextureScale(const void* texture, int scale);
    [[nodiscard]] int TextureScaleOf(const void* texture) noexcept;

    // A texture's logical (1x) size: its size over the scale it was loaded at.
    template <typename Texture>
    [[nodiscard]] float LogicalWidth(const Texture& texture) noexcept
    {
        return static_cast<float>(texture.getWidthProperty()) / static_cast<float>(TextureScaleOf(&texture));
    }
    template <typename Texture>
    [[nodiscard]] float LogicalHeight(const Texture& texture) noexcept
    {
        return static_cast<float>(texture.getHeightProperty()) / static_cast<float>(TextureScaleOf(&texture));
    }

    // PNG size from its header (IHDR); false if the file is not a readable PNG.
    bool PngSize(const std::string& path, int& width, int& height);
}
