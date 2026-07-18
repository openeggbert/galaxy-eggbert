#pragma once

#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

namespace GalaxyEggbert::CNA
{
    // In-game 3D world editor (plan.md section 6, EDITOR-1xx tasks) -- lets a
    // player create/edit/save/play-test their own .vwr worlds from a new
    // GamePhase::Editor mode. Not a mobile-eggbert feature: content-creation
    // tooling, explicitly exempt from the project's faithful-remake rule
    // (see plan.md section 6 / CLAUDE.md).
    //
    // Milestone EDITOR-100 (phase plumbing only): Update()/Draw() are
    // currently no-ops. Free-fly camera control (EDITOR-101), voxel
    // raycasting (EDITOR-102), and block/object editing land in later
    // milestones -- see plan file for the full ordering.
    class GEWorldEditor
    {
    public:
        void Update(const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
                    const Microsoft::Xna::Framework::Input::MouseState& mouse,
                    float dt, int viewportWidth, int viewportHeight);

        void Draw();
    };
}
