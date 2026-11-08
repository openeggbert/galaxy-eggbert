#include <GalaxyEggbert/Editor/EditorDeviceControls.hpp>

#include <cmath>
#include <iostream>

int main()
{
    using namespace GalaxyEggbert::Editor;
    using namespace GalaxyEggbert::Game;
    using namespace Microsoft::Xna::Framework::Input;
    using TP = InputPad::TouchPoint;
    bool ok = true;
    const auto check = [&](bool pass, const char *description) {
        std::cout << (pass ? "PASS: " : "FAIL: ") << description << '\n';
        ok &= pass;
    };
    const MouseState mouse(400, 240, 0, ButtonState::Released, ButtonState::Released, ButtonState::Released,
                           ButtonState::Released, ButtonState::Released);
    {
        EditorDeviceControls controls;
        GamepadFrame pad;
        pad.connected = true;
        pad.leftY = 1;
        pad.rightX = .8f;
        pad.rightTrigger = .75f;
        pad.a = true;
        const auto frame = controls.Update({}, mouse, pad, {}, false, true, false, 800, 480, 1.f / 60);
        check(frame.navigation.forward == 1 && frame.navigation.yaw > 0 &&
                  frame.navigation.vertical == .75f && frame.keyboard.IsKeyDown(Keys::Space) &&
                  !frame.keyboard.IsKeyDown(Keys::LeftControl),
              "gamepad independently flies, looks, rises and invokes Place without keyboard emulation");
        pad.y = true;
        auto pointer = controls.Update({}, mouse, pad, {}, false, true, false, 800, 480, 1.f / 60);
        check(controls.PointerMode() && pointer.navigation.forward == 0 && pointer.navigation.yaw == 0,
              "Y switches to a UI pointer and stops flying/turning the camera");
        (void)controls.Update({}, mouse, pad, {}, false, true, false, 800, 480, 1.f / 60);
        check(controls.PointerMode(), "holding Y cannot repeatedly change pointer modes");
        pad = {};
        const auto unplugged = controls.Update({}, mouse, pad, {}, false, true, false, 800, 480, 1.f / 60);
        check(unplugged.navigation.forward == 0 && !unplugged.keyboard.IsKeyDown(Keys::Space),
              "unplugging the controller releases movement and placement");
    }
    {
        EditorDeviceControls controls;
        const auto move = EditorDeviceControls::MoveRect(800, 480);
        const float cx = (move.x0 + move.x1) * .5f, cy = (move.y0 + move.y1) * .5f;
        auto frame =
            controls.Update({}, mouse, {}, {{1, cx, move.y0, true, false}, {2, 400, 200, true, false}}, true,
                            true, false, 800, 480, 1.f / 60);
        check(frame.navigation.forward == 1 && frame.mouse.getLeftButtonProperty() == ButtonState::Released,
              "a movement finger does not click Place underneath the touch pad");
        frame = controls.Update({}, mouse, {}, {{1, cx, move.y0, false, false}, {2, 440, 215, false, false}},
                                true, true, false, 800, 480, 1.f / 60);
        check(frame.navigation.forward == 1 && frame.navigation.yaw > 0 && frame.navigation.pitch < 0,
              "one touch finger moves while a second independently looks around");
        frame = controls.Update({}, mouse, {}, {}, true, true, false, 800, 480, 1.f / 60);
        check(frame.navigation.forward == 0 && frame.navigation.yaw == 0,
              "interrupted contacts cannot leave the movement or look pad latched");
        const auto up = EditorDeviceControls::UpRect(800, 480);
        frame = controls.Update({}, mouse, {}, {{3, (up.x0 + up.x1) / 2, (up.y0 + up.y1) / 2, true, false}},
                                true, true, false, 800, 480, 1.f / 60);
        check(frame.navigation.vertical == 1, "the visible Up button raises the camera without Ctrl");
        const auto tools = EditorDeviceControls::ToolsRect(800, 480);
        const float tx = (tools.x0 + tools.x1) / 2, ty = (tools.y0 + tools.y1) / 2;
        (void)controls.Update({}, mouse, {}, {{4, tx, ty, true, false}}, true, true, false, 800, 480,
                              1.f / 60);
        (void)controls.Update({}, mouse, {}, {{4, tx, ty, false, true}}, true, true, false, 800, 480,
                              1.f / 60);
        const auto select = EditorDeviceControls::ToolRect(EditorDeviceControls::Tool::Select, 800, 480);
        const float sx = (select.x0 + select.x1) / 2, sy = (select.y0 + select.y1) / 2;
        (void)controls.Update({}, mouse, {}, {{5, sx, sy, true, false}}, true, true, false, 800, 480,
                              1.f / 60);
        frame =
            controls.Update({}, mouse, {}, {{5, sx, sy, false, true}}, true, true, false, 800, 480, 1.f / 60);
        check(controls.Selecting() && frame.keyboard.IsKeyDown(Keys::LeftShift) && frame.navigation.keepAim,
              "touch Select 3D supplies the same selection modifier and preserves the aimed cell");
        const auto save = EditorDeviceControls::ToolRect(EditorDeviceControls::Tool::Save, 800, 480);
        const float vx = (save.x0 + save.x1) / 2, vy = (save.y0 + save.y1) / 2;
        (void)controls.Update({}, mouse, {}, {{6, vx, vy, true, false}}, true, true, false, 800, 480,
                              1.f / 60);
        frame =
            controls.Update({}, mouse, {}, {{6, vx, vy, false, true}}, true, true, false, 800, 480, 1.f / 60);
        check(frame.keyboard.IsKeyDown(Keys::Enter), "touch Save uses the existing save command");
        frame = controls.Update({}, mouse, {}, {}, true, true, false, 800, 480, 1.f / 60);
        check(!frame.keyboard.IsKeyDown(Keys::Enter), "releasing Save cannot save repeatedly");
    }
    {
        EditorDeviceControls controls;
        GamepadFrame pad;
        pad.connected = true;
        pad.a = true;
        const auto frame = controls.Update({}, mouse, pad, {}, false, false, false, 800, 480, 1.f / 60);
        check(frame.mouse.getLeftButtonProperty() == ButtonState::Pressed &&
                  !frame.keyboard.IsKeyDown(Keys::Space) && controls.PointerMode(),
              "gamepad menus use a visible pointer and mouse click rather than game-play bindings");
    }
    {
        EditorDeviceControls controls;
        auto frame = controls.Update({}, mouse, {}, {{7, 710, 200, false, false}}, true, false, false, 800,
                                     480, 1.f / 60);
        check(frame.mouse.getLeftButtonProperty() == ButtonState::Pressed &&
                  frame.mouse.getXProperty() == 710,
              "a first Moved snapshot still starts a touch menu click");
        frame = controls.Update({}, mouse, {}, {}, true, false, false, 800, 480, 1.f / 60);
        check(frame.mouse.getLeftButtonProperty() == ButtonState::Released,
              "a disappearing contact releases the menu click even without a Released snapshot");
    }
    return ok ? 0 : 1;
}
