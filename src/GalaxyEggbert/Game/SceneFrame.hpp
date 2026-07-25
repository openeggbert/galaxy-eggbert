#pragma once

// SceneFrame -- the engine-neutral, per-frame description of what to draw
// (renderers.md, Phase 1). The simulation (already graphics-free) populates a
// SceneFrame each frame; a renderer consumes ONLY this, never the live sim
// state. This is the seam that lets two interchangeable renderers exist behind
// one IGameRenderer (a lightweight Easy3D renderer today, a hi-fi renderer
// later) and that gives the golden/behavioral harness (REMAKE-ANALYSIS.md,
// P0-1) a single deterministic artifact to capture and diff.
//
// Deliberately depends on NO Easy3D/CNA graphics types -- only on the
// engine-agnostic enums (GalaxyEggbert::Def::ObjectType, GalaxyEggbert::Def::SecretPower) and the graphics-free
// controller enums (AnimState, VehicleMode). That keeps it headless-testable
// and consumable by either renderer. Positions are plain world-space floats in
// the same raw grid space the sim uses; the renderer applies its own
// render-space shift/camera (see NEXT.md "Coordinate invariants").
//
// Status: data contract only, not yet wired. Draw() is not yet repointed to
// consume it (Phase 1 step 3) and IGameRenderer does not exist yet (Phase 2).
// HUD is intentionally left out of this first cut -- the real HUD (Hud, 640x480
// reference space) is rich enough that modelling it blind would be a guess; it
// is added once a build is available to verify the mapping against Hud.

#include "BlupiController.hpp"          // AnimState, VehicleMode (graphics-free)
#include <GalaxyEggbert/Def/ObjectType.hpp>
#include <GalaxyEggbert/Def/SecretPower.hpp>

#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Minimal world-space vector, kept local so SceneFrame pulls in no
    // Easy3D/CNA math header. The renderer converts to its own vector type.
    struct SceneVec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct SceneCamera
    {
        SceneVec3 position;
        SceneVec3 target;       // look-at point in world space
        float fovRadians = 0.0f;
        bool thirdPerson = false;
    };

    // One dynamic object (pickup / enemy / effect / lift / crate). Carries the
    // SEMANTIC state (type + animation phase + patrol direction), not a resolved
    // 2D icon: the lightweight renderer resolves the icon exactly as today (via
    // ObjectIcons::GetObjIcon(type, phase)), while a future hi-fi renderer can
    // map (type, phase) to a 3D model instead. Position is `current*` from
    // MobileObjSpec (the live, moved position), not `posStart`.
    struct SceneObject
    {
        GalaxyEggbert::Def::ObjectType type{};
        SceneVec3 position;
        float phase = 0.0f;     // per-instance animation tick counter
        float direction = 1.0f; // patrol direction (+1 toward posEnd, -1 back)
        bool active = true;     // false once a one-shot pickup is collected
    };

    // Blupi's presentation state. `visible` is false while he is invisible in
    // first-person (collision-only) or hidden mid death-loss Voyage; the
    // renderer decides how to show him (2D indicator, placeholder model, or a
    // real 3D model once one exists).
    struct SceneBlupi
    {
        SceneVec3 position;
        BlupiController::AnimState animState = BlupiController::AnimState::Stop;
        BlupiController::VehicleMode vehicleMode = BlupiController::VehicleMode::None;
        GalaxyEggbert::Def::SecretPower secretPower = GalaxyEggbert::Def::SecretPower::None;
        int facing = 1;         // +1 right, -1 left (2D sprite mirror axis)
        bool visible = false;
    };

    struct SceneEnvironment
    {
        int skyRegion = 0;      // world.skyRegion(); selects Content/backgrounds/decorNNN.png
    };

    // The whole per-frame draw description. Static terrain is NOT re-listed here
    // every frame -- it is a mesh the renderer builds once on world load and
    // rebuilds only when `terrainDirty` is set (a world/editor edit), matching
    // how terrain is actually rendered today. Only per-frame dynamic content
    // travels through the SceneFrame.
    struct SceneFrame
    {
        SceneCamera camera;
        SceneBlupi blupi;
        SceneEnvironment environment;
        std::vector<SceneObject> objects;
        bool terrainDirty = false;   // renderer (re)builds the static terrain mesh when true
        // HUD: deferred to a build-verified pass (see file header note).
    };
}
