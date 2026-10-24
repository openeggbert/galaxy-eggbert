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
    // icon it replaces, the time in its "Frames" clip, whether the icon is the mirrored render
    // and the yaw it was turned by:
    //
    //     icon <sheet> <icon> <seconds> <mirrored 0|1> <yaw degrees> <iou>
    //
    // Sheets are named after their PNGs (element, object-m, explo, blupi, blupi1). The game keeps
    // choosing every object's icon exactly as before; an icon listed here is drawn as its model
    // instead of a billboard, any other icon stays a billboard. Engine-agnostic, so
    // tools/VerifyObjectModelTable.cpp checks it without a graphics context.
    struct ObjectModelPose
    {
        int model = -1;           // the caller's index of the model the icons.txt came with
        float seconds = 0.0f;     // time in the model's "Frames" clip
        bool mirrored = false;    // draw mirrored in X (after the yaw)
        float yawDegrees = 0.0f;  // turn about +Y, +Z toward +X (XNA's CreateRotationY)
    };

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
