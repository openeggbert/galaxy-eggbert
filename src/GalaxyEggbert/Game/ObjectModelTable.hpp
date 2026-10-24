#pragma once

#include "ObjectDefinitionRegistry.hpp"

#include <cstdint>
#include <istream>
#include <string>
#include <unordered_map>

namespace GalaxyEggbert::Game
{
    // Which 3D object model draws which sprite icon (plan.md Phase 7B, BB3D-010). The models in
    // avatars3d/objects/<name>/ are converted from the Ray Dream scenes the sprites were rendered
    // from (tools/rd4_to_gltf/build_object_library.py); each one's icons.txt lists, per sprite
    // icon it replaces, the time in its "Frames" clip, how the icon mirrors the render (0 not,
    // 1 left-right, 2 upside down: the hanging mine is the mine's sprite flipped) and the yaw
    // it was turned by:
    //
    //     icon <sheet> <icon> <seconds> <mirror 0|1|2> <yaw degrees> [<iou> [<icon pixels> [<dx> <dy>]]]
    //
    // dx/dy: the sprite's offset in its cell, pixels right and down, applied after the yaw and
    // flip (the green pillars sit up to 8 pixels off their cell's centre).
    //
    // Sheets are named after their PNGs (element, object-m, explo, blupi, blupi1). The game keeps
    // choosing every object's icon exactly as before; an icon listed here is drawn as its model
    // instead of a billboard, any other icon stays a billboard. Engine-agnostic, so
    // tools/VerifyObjectModelTable.cpp checks it without a graphics context.
    struct ObjectModelPose
    {
        int model = -1;           // the caller's index of the model the icons.txt came with
        float seconds = 0.0f;     // time in the model's "Frames" clip
        int mirror = 0;           // after the yaw: 0 none, 1 left-right, 2 upside down
        float yawDegrees = 0.0f;  // turn about +Y, +Z toward +X (XNA's CreateRotationY)
        int iconPixels = 0;       // the icon's square in game pixels (64 a block), 0 if not given
        int offsetX = 0;          // the sprite's offset, game pixels right (after yaw and flip)
        int offsetY = 0;          // and down
    };

    // A flipped icon is the sprite image flipped about its centre line, which is half a pixel
    // left of (above) the model's origin, the sprite camera's axis: x' = -x - kFlipShift left-
    // right, y' = -y + kFlipShift upside down, in blocks (a sprite pixel is 1/64 block).
    constexpr float kFlipShift = 1.0f / 64.0f;

    class ObjectModelTable
    {
    public:
        // Adds the icon lines of one model's icons.txt; comments (#) and blank lines are skipped.
        // Returns false, with the line in ``error``, on a malformed line or an unknown sheet; the
        // lines before it stay added. A later model listing the same icon replaces the earlier.
        bool AddModel(int model, std::istream& iconsTxt, std::string* error = nullptr);

        [[nodiscard]] const ObjectModelPose* Find(ObjectTextureSource sheet, int icon) const;
        [[nodiscard]] std::size_t Size() const noexcept { return poses_.size(); }

        // "element" -> Element, "object-m" -> ObjectM, "explo" -> Explo, "blupi" -> Blupi,
        // "blupi1" -> Blupi1.
        static bool ParseSheet(const std::string& name, ObjectTextureSource& sheet);

    private:
        static std::uint32_t Key(ObjectTextureSource sheet, int icon) noexcept;

        std::unordered_map<std::uint32_t, ObjectModelPose> poses_;
    };
}
