#include "GEWorldEditor.hpp"

#include "GEVoxelRaycast.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kPitchLimit = 1.55334f; // ~89 degrees -- avoids the gimbal flip straight up/down
        constexpr float kMouseLookRadiansPerPixel = 0.0025f;
        constexpr float kMinFlySpeed = 1.0f;
        constexpr float kMaxFlySpeed = 200.0f;
        constexpr float kScrollSpeedStepPerNotch = 1.15f; // multiplicative -- stays useful close-up and world-spanning
        constexpr float kMaxRaycastDistance = 200.0f; // > the 100^3 world's ~173-unit diagonal
    }

    void GEWorldEditor::EnterEditing(float startX, float startY, float startZ) noexcept
    {
        camX_ = startX;
        camY_ = startY;
        camZ_ = startZ;
        yaw_ = 0.0f;
        pitch_ = -0.35f;
        mouseLookHeldLastFrame_ = false;
        hasLastScrollWheelValue_ = false;
        hasHighlight_ = false;
        highlightRenderer_.Hide();
    }

    void GEWorldEditor::Update(const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
                               const Microsoft::Xna::Framework::Input::MouseState& mouse,
                               float dt, int /*viewportWidth*/, int /*viewportHeight*/,
                               Easy3D::Camera3D& camera,
                               Worlds::World& world)
    {
        using Microsoft::Xna::Framework::Input::ButtonState;
        using Microsoft::Xna::Framework::Input::Keys;
        using Microsoft::Xna::Framework::Input::Mouse;

        const bool rightHeld = mouse.getRightButtonProperty() == ButtonState::Pressed;
        if (rightHeld && !mouseLookHeldLastFrame_)
        {
            // Just started holding -- capture the cursor and switch to
            // relative-delta mouse reporting. The X/Y already read into
            // `mouse` above were captured before relative mode took
            // effect, so deliberately skip applying a look delta THIS
            // frame; every frame after this one reports true per-frame
            // deltas instead of absolute position.
            Mouse::SetCaptureEXT(true);
            Mouse::setIsRelativeMouseModeEXTProperty(true);
        }
        else if (!rightHeld && mouseLookHeldLastFrame_)
        {
            Mouse::setIsRelativeMouseModeEXTProperty(false);
            Mouse::SetCaptureEXT(false);
        }
        else if (rightHeld)
        {
            yaw_ += static_cast<float>(mouse.getXProperty()) * kMouseLookRadiansPerPixel;
            pitch_ -= static_cast<float>(mouse.getYProperty()) * kMouseLookRadiansPerPixel;
            pitch_ = std::clamp(pitch_, -kPitchLimit, kPitchLimit);
        }
        mouseLookHeldLastFrame_ = rightHeld;

        // Scroll wheel adjusts fly speed (multiplicative per notch, so it
        // stays useful at both close-up and whole-world-spanning
        // distances). getScrollWheelValueProperty() is CUMULATIVE since
        // startup, not per-frame -- the first reading only establishes a
        // baseline, it never adjusts speed itself.
        const int scrollWheelValue = mouse.getScrollWheelValueProperty();
        if (hasLastScrollWheelValue_)
        {
            const int deltaNotches = (scrollWheelValue - lastScrollWheelValue_) / 120;
            if (deltaNotches != 0)
            {
                flySpeed_ *= std::pow(kScrollSpeedStepPerNotch, static_cast<float>(deltaNotches));
                flySpeed_ = std::clamp(flySpeed_, kMinFlySpeed, kMaxFlySpeed);
            }
        }
        lastScrollWheelValue_ = scrollWheelValue;
        hasLastScrollWheelValue_ = true;

        // Forward/right derived from yaw/pitch, matching GEBlupiController's
        // own horizontal-plane convention exactly (forward = (sin(yaw),
        // -cos(yaw)) in XZ, yaw 0 = facing -Z) so this feels consistent
        // with the rest of the game's camera/movement code.
        const float cosPitch = std::cos(pitch_);
        const Easy3D::Camera3D::Vector3 forward(
            std::sin(yaw_) * cosPitch, std::sin(pitch_), -std::cos(yaw_) * cosPitch);
        const Easy3D::Camera3D::Vector3 right(std::cos(yaw_), 0.0f, std::sin(yaw_));
        const Easy3D::Camera3D::Vector3 worldUp(0.0f, 1.0f, 0.0f);

        float moveX = 0.0f;
        float moveY = 0.0f;
        float moveZ = 0.0f;
        const auto move = [&moveX, &moveY, &moveZ](const Easy3D::Camera3D::Vector3& axis, float amount)
        {
            moveX += axis.X * amount;
            moveY += axis.Y * amount;
            moveZ += axis.Z * amount;
        };
        if (keyboard.IsKeyDown(Keys::W)) move(forward, 1.0f);
        if (keyboard.IsKeyDown(Keys::S)) move(forward, -1.0f);
        if (keyboard.IsKeyDown(Keys::D)) move(right, 1.0f);
        if (keyboard.IsKeyDown(Keys::A)) move(right, -1.0f);
        if (keyboard.IsKeyDown(Keys::Space)) move(worldUp, 1.0f);
        if (keyboard.IsKeyDown(Keys::LeftControl)) move(worldUp, -1.0f);

        camX_ += moveX * flySpeed_ * dt;
        camY_ += moveY * flySpeed_ * dt;
        camZ_ += moveZ * flySpeed_ * dt;

        camera.SetPosition(Easy3D::Camera3D::Vector3(camX_, camY_, camZ_));
        camera.SetTarget(Easy3D::Camera3D::Vector3(
            camX_ + forward.X, camY_ + forward.Y, camZ_ + forward.Z));
        camera.SetUp(worldUp);

        // Voxel raycast (plan.md EDITOR-102) -- camera position/direction
        // are in RENDER space (shifted by -kWorldCenterX/Z from @p world's
        // own raw grid space, see GEVoxelRaycast.hpp); only X/Z need the
        // shift reversed, Y is unshifted in both spaces.
        const RaycastHit hit = Raycast(world,
                                        camX_ + static_cast<float>(GEWorldRuntime::kWorldCenterX), camY_,
                                        camZ_ + static_cast<float>(GEWorldRuntime::kWorldCenterZ),
                                        forward.X, forward.Y, forward.Z,
                                        kMaxRaycastDistance);
        hasHighlight_ = hit.hit;
        if (hasHighlight_)
        {
            highlightX_ = static_cast<float>(hit.x) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
            highlightY_ = static_cast<float>(hit.y);
            highlightZ_ = static_cast<float>(hit.z) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
            hitCellX_ = hit.x;
            hitCellY_ = hit.y;
            hitCellZ_ = hit.z;
            hitNormalX_ = hit.normalX;
            hitNormalY_ = hit.normalY;
            hitNormalZ_ = hit.normalZ;
        }

        // Editing tools (plan.md EDITOR-103) -- edge-triggered on the
        // press, matching every existing click-handler in this codebase
        // (e.g. GEInputPad's "!mouseDown && mouseWasDown_" idiom, just
        // inverted here to trigger on press rather than release since
        // there's no on-screen button geometry to still be hovering over).
        using ButtonState = Microsoft::Xna::Framework::Input::ButtonState;
        const bool leftHeld = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        const bool middleHeld = mouse.getMiddleButtonProperty() == ButtonState::Pressed;
        const bool enterHeld = keyboard.IsKeyDown(Microsoft::Xna::Framework::Input::Keys::Enter);

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        if (leftHeld && !leftHeldLastFrame_ && hasHighlight_)
        {
            const int placeX = static_cast<int>(hitCellX_) + hitNormalX_;
            const int placeY = static_cast<int>(hitCellY_) + hitNormalY_;
            const int placeZ = static_cast<int>(hitCellZ_) + hitNormalZ_;
            if (placeX >= 0 && placeX < blocksPerAxis &&
                placeY >= 0 && placeY < blocksPerAxis &&
                placeZ >= 0 && placeZ < blocksPerAxis)
            {
                world.setBlock(static_cast<std::uint16_t>(placeX), static_cast<std::uint16_t>(placeY),
                                static_cast<std::uint16_t>(placeZ),
                                Worlds::Block::make(GalaxyEggbert::BlockTypes::RockPile));
                needsPresentationRebuild_ = true;
            }
        }
        else if (middleHeld && !middleHeldLastFrame_ && hasHighlight_)
        {
            world.setBlock(hitCellX_, hitCellY_, hitCellZ_, Worlds::Block::air());
            needsPresentationRebuild_ = true;
        }
        else if (enterHeld && !enterHeldLastFrame_ && !worldPath_.empty())
        {
            world.saveToFile(worldPath_);
        }
        leftHeldLastFrame_ = leftHeld;
        middleHeldLastFrame_ = middleHeld;
        enterHeldLastFrame_ = enterHeld;
    }

    bool GEWorldEditor::ConsumeNeedsPresentationRebuild() noexcept
    {
        const bool result = needsPresentationRebuild_;
        needsPresentationRebuild_ = false;
        return result;
    }

    void GEWorldEditor::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                             const Easy3D::Camera3D& camera)
    {
        if (hasHighlight_)
        {
            highlightRenderer_.ShowCell(device, highlightX_, highlightY_, highlightZ_);
            highlightRenderer_.Draw(device, camera);
        }
        else
        {
            highlightRenderer_.Hide();
        }
    }
}
