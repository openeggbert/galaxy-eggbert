#pragma once

#include "WorldEditor.hpp"
#include <GalaxyEggbert/Game/GamepadInput.hpp>
#include <GalaxyEggbert/Game/InputPad.hpp>

#include <functional>
#include <map>
#include <memory>

namespace GalaxyEggbert::Editor
{
// Keyboard-free editor front end. Its output drives the existing editor commands, palette
// and unsaved-change protection; it does not edit worlds or bypass those commands.
class EditorDeviceControls
{
  public:
    using Rect = GalaxyEggbert::Game::QuadBatch::Rect;
    enum class Tool
    {
        Save,
        Undo,
        Redo,
        Select,
        Cancel,
        Remove,
        Turn,
        Variant,
        Pick,
        End,
        Field,
        Increase,
        Decrease,
        DeleteObject,
        Copy,
        Paste,
        TurnCopy,
        Mirror,
        Box,
        Keep,
        Reset,
        Music,
        Help,
        Stop,
        Count
    };
    struct Result
    {
        Microsoft::Xna::Framework::Input::KeyboardState keyboard;
        Microsoft::Xna::Framework::Input::MouseState mouse;
        WorldEditor::Navigation navigation;
    };
    EditorDeviceControls();
    ~EditorDeviceControls();
    void Reset() noexcept
    {
        initialized_ = false;
    }
    Result Update(const Microsoft::Xna::Framework::Input::KeyboardState &keyboard,
                  const Microsoft::Xna::Framework::Input::MouseState &mouse,
                  const GalaxyEggbert::Game::GamepadFrame &pad,
                  const std::vector<GalaxyEggbert::Game::InputPad::TouchPoint> &touches, bool touchCapable,
                  bool editing, bool paletteOpen, int width, int height, float dt,
                  const std::function<bool(int, int)> &overPalette = {});
    void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice &device, int width, int height);
    [[nodiscard]] bool PointerMode() const noexcept
    {
        return pointerMode_;
    }
    [[nodiscard]] bool Selecting() const noexcept
    {
        return selecting_;
    }
    [[nodiscard]] static Rect ToolsRect(int width, int height);
    [[nodiscard]] static Rect ToolRect(Tool tool, int width, int height);
    [[nodiscard]] static Rect MoveRect(int width, int height);
    [[nodiscard]] static Rect UpRect(int width, int height);
    [[nodiscard]] static Rect DownRect(int width, int height);

  private:
    struct Rendering;
    std::unique_ptr<Rendering> rendering_;
    [[nodiscard]] int HitControl(float x, float y, int width, int height) const;
    bool editing_ = false, initialized_ = false, pointerMode_ = false;
    bool padConnected_ = false, touchCapable_ = false, paletteOpen_ = false;
    bool toolsOpen_ = false, selecting_ = false, keep_ = false, yWasDown_ = false;
    bool backWasDown_ = false, mouseWasDown_ = false, padAWasDown_ = false;
    bool ignorePadA_ = false;
    int padControl_ = -1;
    int mouseControl_ = -1, quietFrames_ = 0;
    int uiTouch_ = -1, lookTouch_ = -1;
    float cursorX_ = 0, cursorY_ = 0, lookX_ = 0, lookY_ = 0;
    float lookStartX_ = 0, lookStartY_ = 0;
    bool touchCenteredAim_ = false, lookMoved_ = false;
    std::map<int, int> touchControls_;
    std::map<int, GalaxyEggbert::Game::InputPad::TouchPoint> observedTouches_;
};
} // namespace GalaxyEggbert::Editor
