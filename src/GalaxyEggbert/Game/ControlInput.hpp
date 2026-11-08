#pragma once

#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <algorithm>
#include <unordered_set>

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

    // Remap the physical keyboard before adding the gamepad's synthetic Jump key. Thus
    // Left Ctrl fires in an armed vehicle while a gamepad A still jumps/climbs normally.
    [[nodiscard]] inline Microsoft::Xna::Framework::Input::KeyboardState VehicleFireKeyboard(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard, bool armedVehicle)
    {
        using namespace Microsoft::Xna::Framework::Input;
        if (!armedVehicle || !keyboard.IsKeyDown(Keys::LeftControl)) return keyboard;
        const auto pressed=keyboard.GetPressedKeys();
        std::unordered_set<Keys> keys(pressed.begin(),pressed.end());
        keys.erase(Keys::LeftControl);
        keys.insert(Keys::F);
        return KeyboardState(keys);
    }

    // Independent of the game phase: help freezes the current screen and consumes its closing
    // key, including Escape/Start, so dismissing it cannot also pause or leave a menu.
    struct ControlsHelp
    {
        bool shown = false;
        bool cheats = false;
        bool f1WasDown = false;
        bool escapeWasDown = false;
        bool pointerCloseHeld = false;

        [[nodiscard]] static bool OverClose(float x, float y, int width, int height) noexcept
        {
            const float scale=std::min(width/640.f,height/480.f);
            const float ox=(width-640*scale)*.5f,oy=(height-480*scale)*.5f;
            return scale>0 && x>=ox+550*scale && x<=ox+620*scale &&
                   y>=oy+18*scale && y<=oy+54*scale;
        }

        [[nodiscard]] bool Update(const Microsoft::Xna::Framework::Input::KeyboardState& keys,
            bool pointerDown=false, bool overClose=false) noexcept
        {
            using Microsoft::Xna::Framework::Input::Keys;
            const bool wasPointerHeld=pointerCloseHeld;
            if (!pointerDown) pointerCloseHeld=false;
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
            if (shown && pointerDown && overClose)
            {
                shown=false;
                cheats=false;
                pointerCloseHeld=true;
            }
            return wasShown || shown || wasPointerHeld || pointerCloseHeld;
        }
    };
}
