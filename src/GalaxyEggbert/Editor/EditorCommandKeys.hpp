#pragma once

#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <unordered_set>
#include <deque>

namespace GalaxyEggbert::Editor
{
    // Retain native variant/turn commands between updates, in arrival order.
    // Separate queued presses with a release tick for the editor's edge handling.
    class EditorCommandKeys
    {
    public:
        void Record(Microsoft::Xna::Framework::Input::Keys key)
        {
            using Microsoft::Xna::Framework::Input::Keys;
            if ((key == Keys::V || key == Keys::O) && pending_.size() < 256) pending_.push_back(key);
        }
        void Reset() noexcept { pending_.clear(); releaseNext_ = true; }

        [[nodiscard]] Microsoft::Xna::Framework::Input::KeyboardState Next(
            const Microsoft::Xna::Framework::Input::KeyboardState& physical)
        {
            using namespace Microsoft::Xna::Framework::Input;
            const auto pressed = physical.GetPressedKeys();
            std::unordered_set<Keys> keys(pressed.begin(), pressed.end());
            keys.erase(Keys::V);
            keys.erase(Keys::O);
            if (releaseNext_) releaseNext_ = false;
            else if (!pending_.empty())
            {
                keys.insert(pending_.front());
                pending_.pop_front();
                releaseNext_ = true;
            }
            return KeyboardState(keys);
        }

    private:
        std::deque<Microsoft::Xna::Framework::Input::Keys> pending_;
        bool releaseNext_ = false;
    };
}
