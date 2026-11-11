#pragma once

// Helpers GalaxyEggbertGame's translation units share (GalaxyEggbertGame.cpp, GalaxyEggbertGameDraw.cpp,
// GalaxyEggbertGameScene.cpp). Not a public interface: only the CNA host includes it.

#include <GalaxyEggbert/Def/GameSpeed.hpp>
#include <GalaxyEggbert/Def/ObjectType.hpp>
#include <GalaxyEggbert/Game/GamepadInput.hpp>
#include <GalaxyEggbert/Game/Localization.hpp>

#include <filesystem>

namespace Microsoft::Xna::Framework::Graphics
{
    class GraphicsDevice;
}

namespace GalaxyEggbert::CNA::detail
{
    // The real model (avatars3d/blupi/, its vehicles too) is in metres
    // at 0.05 m per Ray Dream unit. The original game camera draws
    // 3.0309 px per unit (zoom 3.43654 x 60 px / frame height 68.03136)
    // and a block is 64 px (DIMOBJX), so a block is 21.116 units =
    // 1.0558 m, and the model keeps the sprites' size against blocks at
    // 1 / 1.0558.
    inline constexpr float kRealModelScale = 0.94714f;

    inline constexpr int kGoldenCaptureTicks[] = {60, 120, 180};
    inline constexpr int kGoldenCaptureCount =
        static_cast<int>(sizeof(kGoldenCaptureTicks) / sizeof(kGoldenCaptureTicks[0]));

    // The patrol enemies' per-direction/turn icon tables. Returns false (the caller keeps its own
    // GetObjIcon() fallback) for any type outside that family.
    bool TryGetPatrolIcon(GalaxyEggbert::Def::ObjectType type, bool patrolGoesLeftFromStart, int patrolStep,
                          int patrolTimeTicks, int& outIcon);

    // The first connected gamepad as keys (including a pad assigned to another XNA slot).
    GalaxyEggbert::Game::GamepadFrame ReadGamepadFrame();

    float GameSpeedFactor(GalaxyEggbert::Def::GameSpeed speed) noexcept;

    // The backbuffer's own size, which GetBackBufferData() reads (see its definition).
    void GetBackBufferSize(const Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int& w, int& h);

    // A saved world's thumbnail (ED-0013) from the frame just drawn.
    void SaveEditorThumbnail(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                             const std::filesystem::path& path);
}
