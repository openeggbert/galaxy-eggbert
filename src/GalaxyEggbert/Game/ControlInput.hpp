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

    // Remap physical Ctrl only in the tank, before the gamepad's synthetic Jump key.
    // In the helicopter Ctrl and gamepad A both climb; F is its keyboard glue drop.
    [[nodiscard]] inline Microsoft::Xna::Framework::Input::KeyboardState VehicleFireKeyboard(
        const Microsoft::Xna::Framework::Input::KeyboardState& keyboard, bool inTank)
    {
        using namespace Microsoft::Xna::Framework::Input;
        if (!inTank || !keyboard.IsKeyDown(Keys::LeftControl)) return keyboard;
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
        int page = 0;
        bool previousPageHeld = false, nextPageHeld = false, pagePointerHeld = false;
        // A cheat clicked in the list (InputPad::CheatHelpEntryAt's index), until TakeChosen(). A
        // press already down when the list opened (the `p` of `help` tapped on the cheats' keyboard)
        // chooses nothing until it is let go.
        int chosen = -1;
        bool entryPointerHeld = false;

        void OpenCheats() noexcept
        {
            shown = cheats = true;
            page = 0;
            previousPageHeld = nextPageHeld = pagePointerHeld = false;
            entryPointerHeld = true;
            chosen = -1;
        }
        [[nodiscard]] int TakeChosen() noexcept
        {
            const int taken = chosen;
            chosen = -1;
            return taken;
        }
        // F1's controls help, opened by the cheats' keyboard's F1 key (a physical F1 toggles it in Update()).
        void OpenControls() noexcept
        {
            shown = true;
            cheats = false;
            page = 0;
            previousPageHeld = nextPageHeld = pagePointerHeld = false;
        }

        [[nodiscard]] static int PageArrow(float x, float y, int width, int height) noexcept
        {
            const float scale = std::min(width/640.f,height/480.f);
            if (scale <= 0) return 0;
            x = (x - (width-640*scale)*.5f) / scale;
            y = (y - (height-480*scale)*.5f) / scale;
            if (y < 410 || y > 438) return 0;
            if (x >= 200 && x <= 260) return -1;
            if (x >= 370 && x <= 430) return 1;
            return 0;
        }

        [[nodiscard]] static bool OverClose(float x, float y, int width, int height) noexcept
        {
            const float scale=std::min(width/640.f,height/480.f);
            const float ox=(width-640*scale)*.5f,oy=(height-480*scale)*.5f;
            return scale>0 && x>=ox+550*scale && x<=ox+620*scale &&
                   y>=oy+18*scale && y<=oy+54*scale;
        }

        // @p entry: the cheat under the pointer (InputPad::CheatHelpEntryAt), -1 for none. Pressed
        // there, it is chosen and the help closes, the press consumed through its release as Close's.
        // @p pages: how many pages the cheats' help has (InputPad::CheatHelpPageCount(): two, more with
        // large text on a touch screen); Left / LB / [<] go back, Right / RB / [>] on, round.
        [[nodiscard]] bool Update(const Microsoft::Xna::Framework::Input::KeyboardState& keys,
            bool pointerDown=false, bool overClose=false, int pageArrow=0, int entry=-1, int pages=2) noexcept
        {
            using Microsoft::Xna::Framework::Input::Keys;
            const bool wasPointerHeld=pointerCloseHeld;
            if (!pointerDown) pointerCloseHeld=false;
            const bool wasPagePointerHeld = pagePointerHeld;
            if (!pointerDown) pagePointerHeld = false;
            const bool wasEntryPointerHeld = entryPointerHeld;
            entryPointerHeld = pointerDown;
            const bool previous = keys.IsKeyDown(Keys::Left) || keys.IsKeyDown(Keys::A) ||
                keys.IsKeyDown(Keys::PageUp) || keys.IsKeyDown(Keys::LeftShift); // gamepad LB
            const bool next = keys.IsKeyDown(Keys::Right) || keys.IsKeyDown(Keys::D) ||
                keys.IsKeyDown(Keys::PageDown) || keys.IsKeyDown(Keys::RightControl); // gamepad RB
            if (shown && cheats)
            {
                pages = std::max(1, pages);
                page = ((page % pages) + pages) % pages; // the page count can change (touch capability)
                int step = 0;
                if (previous && !previousPageHeld) --step;
                if (next && !nextPageHeld) ++step;
                if (pointerDown && !wasPagePointerHeld && pageArrow != 0) step += pageArrow < 0 ? -1 : 1;
                page = ((page + step) % pages + pages) % pages;
                if (pointerDown) pagePointerHeld = true;
            }
            previousPageHeld = previous;
            nextPageHeld = next;
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
            else if (wasShown && shown && cheats && pointerDown && !wasEntryPointerHeld && pageArrow == 0 &&
                     entry >= 0)
            {
                chosen = entry;
                shown = cheats = false;
                pointerCloseHeld = true;
            }
            return wasShown || shown || wasPointerHeld || pointerCloseHeld;
        }
    };
}
