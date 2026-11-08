#pragma once

#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>

namespace GalaxyEggbert::Game
{
    struct PlayDirections
    {
        float turn = 0.0f;
        float move = 0.0f;
        bool backUp = false;
    };

    // Aliases are combined per direction: holding W and Up still walks at normal speed.
    [[nodiscard]] inline PlayDirections ReadPlayDirections(
        const Microsoft::Xna::Framework::Input::KeyboardState& keys) noexcept
    {
        using Microsoft::Xna::Framework::Input::Keys;
        return {
            float(keys.IsKeyDown(Keys::Right) || keys.IsKeyDown(Keys::D)) -
                float(keys.IsKeyDown(Keys::Left) || keys.IsKeyDown(Keys::A)),
            float(keys.IsKeyDown(Keys::Up) || keys.IsKeyDown(Keys::W)) -
                float(keys.IsKeyDown(Keys::Down) || keys.IsKeyDown(Keys::S)),
            keys.IsKeyDown(Keys::X) || keys.IsKeyDown(Keys::RightControl)};
    }

    // Independent of the game phase: help freezes the current screen and consumes its closing
    // key, including Escape/Start, so dismissing it cannot also pause or leave a menu.
    struct ControlsHelp
    {
        bool shown = false;
        bool cheats = false;
        bool f1WasDown = false;
        bool escapeWasDown = false;

        [[nodiscard]] bool Update(const Microsoft::Xna::Framework::Input::KeyboardState& keys) noexcept
        {
            using Microsoft::Xna::Framework::Input::Keys;
            const bool wasShown = shown;
            const bool f1 = keys.IsKeyDown(Keys::F1);
            const bool escape = keys.IsKeyDown(Keys::Escape);
            if (f1 && !f1WasDown)
            {
                shown = !shown;
                if (shown) cheats = false;
            }
            else if (escape && !escapeWasDown) shown = false;
            f1WasDown = f1;
            escapeWasDown = escape;
            return wasShown || shown;
        }
    };
}
