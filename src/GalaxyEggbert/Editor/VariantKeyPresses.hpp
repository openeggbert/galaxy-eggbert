#pragma once

#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <unordered_set>

namespace GalaxyEggbert::Editor
{
    // Each native non-repeat V press becomes one editor edge, including a whole
    // down/up between updates. Separate queued presses with a release tick.
    class VariantKeyPresses
    {
    public:
        void Record() noexcept { if (pending_ < 256) ++pending_; }
        void Reset() noexcept { pending_ = 0; releaseNext_ = true; }

        [[nodiscard]] Microsoft::Xna::Framework::Input::KeyboardState Next(
            const Microsoft::Xna::Framework::Input::KeyboardState& physical)
        {
            using namespace Microsoft::Xna::Framework::Input;
            const auto pressed = physical.GetPressedKeys();
            std::unordered_set<Keys> keys(pressed.begin(), pressed.end());
            keys.erase(Keys::V);
            if (releaseNext_) releaseNext_ = false;
            else if (pending_ > 0)
            {
                --pending_;
                keys.insert(Keys::V);
                releaseNext_ = true;
            }
            return KeyboardState(keys);
        }

    private:
        unsigned pending_ = 0;
        bool releaseNext_ = false;
    };
}
