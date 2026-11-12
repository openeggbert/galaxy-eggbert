#include "EditorDeviceControls.hpp"
#include <GalaxyEggbert/Game/TextureScale.hpp>

#include <GalaxyEggbert/Game/Localization.hpp>
#include <GalaxyEggbert/Game/TextMetrics.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_set>

namespace GalaxyEggbert::Editor
{
namespace
{
using Keys = Microsoft::Xna::Framework::Input::Keys;
using Tool = EditorDeviceControls::Tool;
using Rect = EditorDeviceControls::Rect;
constexpr int kTools = 100, kMove = 101, kUp = 102, kDown = 103;
struct Entry
{
    const char *name;
    Keys key;
};
constexpr std::array<Entry, int(Tool::Count)> kEntries{{{Game::TrMark("Save"), Keys::Enter},
                                                        {Game::TrMark("Undo"), Keys::U},
                                                        {Game::TrMark("Redo"), Keys::R},
                                                        {Game::TrMark("Select 3D"), Keys::None},
                                                        {Game::TrMark("Cancel"), Keys::Escape},
                                                        {Game::TrMark("Remove"), Keys::Back},
                                                        {Game::TrMark("Turn"), Keys::O},
                                                        {Game::TrMark("Variant"), Keys::V},
                                                        {Game::TrMark("Pick object"), Keys::G},
                                                        {Game::TrMark("Set end"), Keys::T},
                                                        {Game::TrMark("Next field"), Keys::Tab},
                                                        {Game::TrMark("Value +"), Keys::OemPlus},
                                                        {Game::TrMark("Value -"), Keys::OemMinus},
                                                        {Game::TrMark("Delete obj"), Keys::Delete},
                                                        {Game::TrMark("Copy box"), Keys::C},
                                                        {Game::TrMark("Paste"), Keys::P},
                                                        {Game::TrMark("Turn copy"), Keys::Q},
                                                        {Game::TrMark("Mirror"), Keys::X},
                                                        {Game::TrMark("Box fill"), Keys::F},
                                                        {Game::TrMark("Keep"), Keys::None},
                                                        {Game::TrMark("Reset XYZ"), Keys::NumPad5},
                                                        {Game::TrMark("Music"), Keys::M},
                                                        {Game::TrMark("Help"), Keys::H},
                                                        {Game::TrMark("Stop"), Keys::F10}}};
float Scale(int h)
{
    return std::clamp(float(h) / 480.0f, 0.65f, 1.75f);
}
float Axis(float value)
{
    return std::abs(value) <= 0.18f ? 0.0f : std::copysign((std::abs(value) - 0.18f) / 0.82f, value);
}
bool Inside(float x, float y, Rect r)
{
    return GalaxyEggbert::Game::QuadBatch::InRect(x, y, r);
}
} // namespace

struct EditorDeviceControls::Rendering
{
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> flat, text;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> font;
    std::unique_ptr<CnaEngine::Graphics::GpuMesh> panels, labels, active, cursor;
};
EditorDeviceControls::EditorDeviceControls() : rendering_(std::make_unique<Rendering>())
{
}
EditorDeviceControls::~EditorDeviceControls() = default;

EditorDeviceControls::Rect EditorDeviceControls::ToolsRect(int w, int h)
{
    const float s = Scale(h);
    return {w - 94 * s, 8 * s, w - 8 * s, 42 * s};
}
EditorDeviceControls::Rect EditorDeviceControls::ToolRect(Tool tool, int w, int h)
{
    const float s = Scale(h), bw = 86 * s, bh = 32 * s, gap = 4 * s;
    const int columns = w < 580 * s ? 3 : 4;
    const float x = std::max(56 * s, w - 8 * s - columns * (bw + gap)) + (int(tool) % columns) * (bw + gap);
    const float y = 50 * s + (int(tool) / columns) * (bh + gap);
    return {x, y, x + bw, y + bh};
}
EditorDeviceControls::Rect EditorDeviceControls::MoveRect(int, int h)
{
    const float s = Scale(h);
    return {70 * s, h - 170 * s, 174 * s, h - 66 * s};
}
EditorDeviceControls::Rect EditorDeviceControls::UpRect(int w, int h)
{
    const float s = Scale(h);
    return {w - 62 * s, h - 164 * s, w - 10 * s, h - 120 * s};
}
EditorDeviceControls::Rect EditorDeviceControls::DownRect(int w, int h)
{
    const float s = Scale(h);
    return {w - 62 * s, h - 112 * s, w - 10 * s, h - 68 * s};
}
int EditorDeviceControls::HitControl(float x, float y, int w, int h) const
{
    if (!editing_)
        return -1;
    if (Inside(x, y, ToolsRect(w, h)))
        return kTools;
    if (toolsOpen_ && !paletteOpen_)
        for (int i = 0; i < int(Tool::Count); ++i)
            if (Inside(x, y, ToolRect(Tool(i), w, h)))
                return i;
    if (touchCapable_ && !paletteOpen_ && !toolsOpen_)
    {
        if (Inside(x, y, MoveRect(w, h)))
            return kMove;
        if (Inside(x, y, UpRect(w, h)))
            return kUp;
        if (Inside(x, y, DownRect(w, h)))
            return kDown;
    }
    return -1;
}

EditorDeviceControls::Result EditorDeviceControls::Update(
    const Microsoft::Xna::Framework::Input::KeyboardState &keyboard,
    const Microsoft::Xna::Framework::Input::MouseState &mouse, const GalaxyEggbert::Game::GamepadFrame &pad,
    const std::vector<GalaxyEggbert::Game::InputPad::TouchPoint> &contacts, bool touchCapable, bool editing,
    bool paletteOpen, int w, int h, float dt, const std::function<bool(int, int)> &overPalette)
{
    using namespace Microsoft::Xna::Framework::Input;
    if (!initialized_ || editing != editing_)
    {
        editing_ = editing;
        initialized_ = true;
        pointerMode_ = !editing;
        cursorX_ = editing ? w * .5f : w - 90 * Scale(h);
        cursorY_ = editing ? h * .5f : h - 110 * Scale(h);
        toolsOpen_ = selecting_ = keep_ = false;
        touchControls_.clear();
        observedTouches_.clear();
        uiTouch_ = lookTouch_ = mouseControl_ = -1;
        mouseWasDown_ = false;
        padControl_ = -1;
        padAWasDown_ = false;
        ignorePadA_ = false;
    }
    // A platform snapshot can first expose a finger as Moved, or remove it before our next
    // fixed tick sees Released. Recover those edges from contact identity as well as state.
    auto touches = contacts;
    for (auto &touch : touches)
        if (!touch.released && !observedTouches_.contains(touch.id))
            touch.pressed = true;
    for (auto [id, touch] : observedTouches_)
        if (std::none_of(contacts.begin(), contacts.end(), [id](const auto &t) { return t.id == id; }))
        {
            touch.pressed = false;
            touch.released = true;
            touches.push_back(touch);
        }
    observedTouches_.clear();
    for (const auto &touch : contacts)
        if (!touch.released)
            observedTouches_[touch.id] = touch;
    touchCapable_ = touchCapable;
    paletteOpen_ = paletteOpen;
    padConnected_ = pad.connected;
    if (paletteOpen_)
        toolsOpen_ = false;
    const auto pressed = keyboard.GetPressedKeys();
    std::unordered_set<Keys> keys(pressed.begin(), pressed.end());
    Result result;
    result.mouse = mouse;
    auto &nav = result.navigation;
    if (keyboard.IsKeyDown(Keys::Escape))
        selecting_ = false;
    const auto fire = [&](int control) {
        if (control == kTools)
        {
            toolsOpen_ = !toolsOpen_;
            return;
        }
        if (control < 0 || control >= int(Tool::Count))
            return;
        const auto tool = Tool(control);
        if (tool == Tool::Select)
            selecting_ = !selecting_;
        else if (tool == Tool::Keep)
            keep_ = !keep_;
        else
            keys.insert(kEntries[control].key);
        if (tool == Tool::Cancel || (tool == Tool::Select && !selecting_))
        {
            selecting_ = false;
            keys.insert(Keys::Escape);
        }
    };
    const auto move = [&](int control, float x, float y) {
        if (control >= kMove)
            nav.centeredAim = true;
        if (control == kUp)
            nav.vertical += 1;
        else if (control == kDown)
            nav.vertical -= 1;
        else if (control == kMove)
        {
            const auto r = MoveRect(w, h);
            const float half = (r.x1 - r.x0) * .5f;
            nav.strafe += Axis(std::clamp((x - (r.x0 + r.x1) * .5f) / half, -1.f, 1.f));
            nav.forward -= Axis(std::clamp((y - (r.y0 + r.y1) * .5f) / half, -1.f, 1.f));
        }
    };
    quietFrames_ = touches.empty() ? std::max(0, quietFrames_ - 1) : 15;
    bool uiDown = false, touchUiFrame = false, consumeMouse = false;
    bool left = mouse.getLeftButtonProperty() == ButtonState::Pressed;
    int mx = mouse.getXProperty(), my = mouse.getYProperty();
    if ((!pad.connected || pointerMode_) && HitControl(float(mx), float(my), w, h) >= 0)
        nav.keepAim = true;
    if (quietFrames_ == 0 && editing_)
    {
        const int over = HitControl(float(mx), float(my), w, h);
        if (left && !mouseWasDown_)
            mouseControl_ = over;
        if (mouseControl_ >= 0)
        {
            consumeMouse = true;
            if (left)
                move(mouseControl_, float(mx), float(my));
            else
            {
                if (over == mouseControl_)
                    fire(mouseControl_);
                mouseControl_ = -1;
            }
            nav.keepAim = true;
        }
        mouseWasDown_ = left;
    }
    for (const auto &touch : touches)
    {
        if (touch.pressed)
        {
            const int control = HitControl(touch.x, touch.y, w, h);
            if (control >= 0)
                touchControls_[touch.id] = control;
            else if (!editing_ || (overPalette && overPalette(int(touch.x), int(touch.y))))
            {
                if (uiTouch_ < 0)
                    uiTouch_ = touch.id;
            }
            else if (lookTouch_ < 0)
            {
                lookTouch_ = touch.id;
                lookStartX_ = lookX_ = touch.x;
                lookStartY_ = lookY_ = touch.y;
                lookMoved_ = false;
                touchCenteredAim_ = false;
            }
        }
        auto own = touchControls_.find(touch.id);
        if (own != touchControls_.end())
        {
            const int control = own->second;
            if (touch.released)
            {
                if (HitControl(touch.x, touch.y, w, h) == control)
                    fire(control);
                touchControls_.erase(own);
            }
            else
                move(control, touch.x, touch.y);
            if (control < kMove)
                nav.keepAim = true;
        }
        else if (touch.id == uiTouch_)
        {
            touchUiFrame = true;
            uiDown = !touch.released;
            cursorX_ = touch.x;
            cursorY_ = touch.y;
            if (touch.released)
                uiTouch_ = -1;
        }
        else if (touch.id == lookTouch_)
        {
            if (!touch.released)
            {
                const float dx = touch.x - lookX_, dy = touch.y - lookY_;
                if (std::hypot(touch.x - lookStartX_, touch.y - lookStartY_) > 6)
                    lookMoved_ = true;
                if (lookMoved_)
                {
                    nav.yaw += dx * .004f;
                    nav.pitch -= dy * .004f;
                    touchCenteredAim_ = true;
                }
                lookX_ = touch.x;
                lookY_ = touch.y;
                cursorX_ = touch.x;
                cursorY_ = touch.y;
            }
            else
            {
                lookTouch_ = -1;
                nav.keepAim = lookMoved_;
            }
        }
    }
    // Interrupted contacts must not leave a pad/button latched.
    const auto present = [&](int id) {
        return std::any_of(touches.begin(), touches.end(),
                           [&](const auto &t) { return t.id == id && !t.released; });
    };
    for (auto it = touchControls_.begin(); it != touchControls_.end();)
        if (!present(it->first))
            it = touchControls_.erase(it);
        else
            ++it;
    if (uiTouch_ >= 0 && !present(uiTouch_))
        uiTouch_ = -1;
    if (lookTouch_ >= 0 && !present(lookTouch_))
        lookTouch_ = -1;
    if (pad.connected)
    {
        if (editing_ && pad.y && !yWasDown_)
        {
            pointerMode_ = !pointerMode_;
            ignorePadA_ = pad.a;
            padControl_ = -1;
        }
        if (!pad.a)
            ignorePadA_ = false;
        const bool a = pad.a && !ignorePadA_;
        if (!editing_ || pointerMode_)
        {
            // The mouse goes on working with a gamepad plugged in (2026-11-10): moving it takes the
            // pad's cursor there, and its button clicks as A does.
            if (mouse.getXProperty() != realMouseX_ || mouse.getYProperty() != realMouseY_)
            {
                cursorX_ = static_cast<float>(mouse.getXProperty());
                cursorY_ = static_cast<float>(mouse.getYProperty());
            }
            const float dx = Axis(std::abs(pad.rightX) > .18f ? pad.rightX : pad.leftX) +
                             float(pad.dpadRight) - float(pad.dpadLeft);
            const float dy = Axis(std::abs(pad.rightY) > .18f ? pad.rightY : pad.leftY) + float(pad.dpadUp) -
                             float(pad.dpadDown);
            cursorX_ = std::clamp(cursorX_ + dx * 520 * Scale(h) * dt, 1.f, float(w - 2));
            cursorY_ = std::clamp(cursorY_ - dy * 520 * Scale(h) * dt, 1.f, float(h - 2));
            const int over = HitControl(cursorX_, cursorY_, w, h);
            if (over >= 0)
                nav.keepAim = true;
            if (a && !padAWasDown_)
                padControl_ = over;
            if (padControl_ >= 0)
            {
                consumeMouse = true;
                nav.keepAim = true;
                if (a)
                    move(padControl_, cursorX_, cursorY_);
                else
                {
                    if (over == padControl_)
                        fire(padControl_);
                    padControl_ = -1;
                }
            }
            mx = int(cursorX_);
            my = int(cursorY_);
            left = (a || mouse.getLeftButtonProperty() == ButtonState::Pressed) && !consumeMouse;
        }
        else
        {
            nav.strafe += Axis(pad.leftX);
            nav.forward += Axis(pad.leftY);
            nav.vertical += pad.rightTrigger - pad.leftTrigger;
            nav.yaw += Axis(pad.rightX) * 2.2f * dt;
            nav.pitch += Axis(pad.rightY) * 2.2f * dt;
            nav.centeredAim = true;
            nav.keepAim = false;
            mx = w / 2;
            my = h / 2;
            left = false;
            if (a)
                keys.insert(Keys::Space);
            if (pad.x)
                keys.insert(Keys::Back);
            if (pad.dpadLeft)
                keys.insert(Keys::NumPad4);
            if (pad.dpadRight)
                keys.insert(Keys::NumPad6);
            if (pad.dpadUp)
                keys.insert(Keys::NumPad8);
            if (pad.dpadDown)
                keys.insert(Keys::NumPad2);
            if (pad.leftShoulder)
                keys.insert(Keys::NumPad7);
            if (pad.rightShoulder)
                keys.insert(Keys::NumPad9);
        }
        if (pad.b)
        {
            keys.insert(Keys::Escape);
            selecting_ = false;
        }
        if (pad.start)
            keys.insert(Keys::Enter);
        if (pad.back && !backWasDown_)
            keys.insert(editing_ ? Keys::F10 : Keys::Escape);
        padAWasDown_ = a;
    }
    yWasDown_ = pad.y;
    backWasDown_ = pad.back;
    realMouseX_ = mouse.getXProperty();
    realMouseY_ = mouse.getYProperty();
    if (selecting_)
        keys.insert(Keys::LeftShift);
    nav.selecting = selecting_;
    if (keep_)
        keys.insert(Keys::RightAlt);
    if (touchUiFrame)
    {
        mx = int(cursorX_);
        my = int(cursorY_);
        left = uiDown;
    }
    else if (quietFrames_ > 0)
    {
        mx = int(cursorX_);
        my = int(cursorY_);
        left = false;
        nav.centeredAim |= touchCenteredAim_;
    }
    if (consumeMouse)
        left = false;
    result.mouse =
        MouseState(mx, my, mouse.getScrollWheelValueProperty(),
                   left ? ButtonState::Pressed : ButtonState::Released, mouse.getMiddleButtonProperty(),
                   mouse.getRightButtonProperty(), mouse.getXButton1Property(), mouse.getXButton2Property());
    result.keyboard = KeyboardState(keys);
    return result;
}

void EditorDeviceControls::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice &device, int w, int h)
{
    if (!initialized_ || (!editing_ && !padConnected_))
        return;
    using namespace Microsoft::Xna::Framework::Graphics;
    using GalaxyEggbert::Game::QuadBatch::FlushQuads;
    using GalaxyEggbert::Game::QuadBatch::Quad;
    if (!rendering_->flat)
    {
        rendering_->flat = std::make_unique<BasicEffect>(device);
        rendering_->flat->VertexColorEnabled = false;
        rendering_->flat->setTextureEnabledProperty(false);
        {
            int scale = 1;
            rendering_->font = std::make_unique<Texture2D>(Game::ScaledContentPath("Content/icons/text.png", &scale), device);
            Game::SetTextureScale(rendering_->font.get(), scale);
        }
        rendering_->text = std::make_unique<BasicEffect>(device);
        rendering_->text->VertexColorEnabled = false;
        rendering_->text->setTextureEnabledProperty(true);
        rendering_->text->setTextureProperty(rendering_->font.get());
    }
    std::vector<Quad> panels, active, labels, cursor;
    const float s = Scale(h), tw = Game::LogicalWidth(*rendering_->font),
                th = Game::LogicalHeight(*rendering_->font);
    const auto label = [&](std::string_view text, Rect r, float font = .43f) {
        // Smaller when a translation is wider than its button.
        const float width = GalaxyEggbert::Game::TextMetrics::Width(text) * s;
        if (width * font > r.x1 - r.x0 - 4 * s && width > 0) font = (r.x1 - r.x0 - 4 * s) / width;
        float x = (r.x0 + r.x1) * .5f - GalaxyEggbert::Game::TextMetrics::Width(text) * font * s * .5f;
        const float y = (r.y0 + r.y1) * .5f - 12 * font * s;
        for (std::size_t at = 0; at < text.size();)
        {
            int rank = GalaxyEggbert::Game::TextMetrics::NextGlyph(text, at), col = rank % 16, row = rank / 16;
            labels.push_back({x, y, x + 32 * font * s, y + 32 * font * s, col * 32 / tw, row * 32 / th,
                              (col + 1) * 32 / tw, (row + 1) * 32 / th});
            x += GalaxyEggbert::Game::TextMetrics::GlyphAdvance(rank) * font * s;
        }
    };
    const auto button = [&](Rect r, const char *name, bool on = false) {
        (on ? active : panels).push_back({r.x0, r.y0, r.x1, r.y1, 0, 0, 1, 1});
        label(Game::Tr(name), r);
    };
    if (editing_)
    {
        button(ToolsRect(w, h), toolsOpen_ ? Game::TrMark("Close tools") : Game::TrMark("Tools"), toolsOpen_);
        if (toolsOpen_ && !paletteOpen_)
            for (int i = 0; i < int(Tool::Count); ++i)
                button(ToolRect(Tool(i), w, h), kEntries[i].name,
                       (Tool(i) == Tool::Select && selecting_) || (Tool(i) == Tool::Keep && keep_));
        if (touchCapable_ && !paletteOpen_ && !toolsOpen_)
        {
            const auto r = MoveRect(w, h);
            button(r, Game::TrMark("Move"));
            label("^", {r.x0, r.y0, r.x1, r.y0 + 30 * s});
            label("v", {r.x0, r.y1 - 30 * s, r.x1, r.y1});
            label("<", {r.x0, r.y0, r.x0 + 28 * s, r.y1});
            label(">", {r.x1 - 28 * s, r.y0, r.x1, r.y1});
            button(UpRect(w, h), Game::TrMark("Up"));
            button(DownRect(w, h), Game::TrMark("Down"));
        }
        if (padConnected_ && !paletteOpen_ && !toolsOpen_)
        {
            const Rect hint{float(w) - 370 * s, 46 * s, float(w) - 8 * s, 67 * s};
            panels.push_back({hint.x0, hint.y0, hint.x1, hint.y1, 0, 0, 1, 1});
            label(pointerMode_ ? Game::Tr("Y: fly  A: click") : Game::Tr("Y: pointer  A: place  X: remove"), hint, .38f);
        }
    }
    if (padConnected_ && pointerMode_)
    {
        cursor.push_back({cursorX_ - 8 * s, cursorY_ - s, cursorX_ + 8 * s, cursorY_ + s, 0, 0, 1, 1});
        cursor.push_back({cursorX_ - s, cursorY_ - 8 * s, cursorX_ + s, cursorY_ + 8 * s, 0, 0, 1, 1});
    }
    device.setBlendStateProperty(BlendState::NonPremultiplied);
    rendering_->flat->setDiffuseColorProperty({.80f, .90f, .98f});
    FlushQuads(device, *rendering_->flat, rendering_->panels, panels, w, h, .88f);
    rendering_->flat->setDiffuseColorProperty({1, .81f, .25f});
    FlushQuads(device, *rendering_->flat, rendering_->active, active, w, h, .95f);
    FlushQuads(device, *rendering_->flat, rendering_->cursor, cursor, w, h, 1);
    rendering_->text->setDiffuseColorProperty({0, 0, 0});
    FlushQuads(device, *rendering_->text, rendering_->labels, labels, w, h, 1);
    device.setBlendStateProperty(BlendState::Opaque);
}
} // namespace GalaxyEggbert::Editor
