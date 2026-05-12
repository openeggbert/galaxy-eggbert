#include "GalaxyEggbert/InputPad.hpp"

#include "CNA/Platform.hpp"
#include "Microsoft/Devices/Sensors/AccelerometerFailedException.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchCollection.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/UnauthorizedAccessException.hpp"
#include "GalaxyEggbert/Config.hpp"
#include "GalaxyEggbert/IGame1.hpp"
#include "GalaxyEggbert/Misc.hpp"
#include "GalaxyEggbert/enums/SoundChannel.hpp"

#define INPUT_DEBUG(msg) CNA::Logger::DebugIf(msg, Config::INPUT_DETAILED_DEBUGGING_ENABLED);
#define INPUT_ENABLED

#ifndef INPUT_ENABLED
#define INPUT_DISABLED
#endif

namespace GalaxyEggbert
{
    IDATA(Def::Phase, Phase, InputPad)
    IDATA(int, SelectedGamer, InputPad)
    IDATA(TinyPoint, PixmapOrigin, InputPad)
    int InputPad::getTotalTouchOrClickProperty() const { return touchOrClickCount; }

    Def::ButtonGlyph InputPad::getButtonPressedProperty() const
    {
#ifdef INPUT_DISABLED
        return Def::ButtonGlyph::None;
#endif
        Def::ButtonGlyph result = buttonPressed;
        buttonPressed = Def::ButtonGlyph::None;
        return result;
    }

    IDATA(bool, ShowCheatMenu, InputPad)

    std::vector<Def::ButtonGlyph> InputPad::getButtonGlyphsProperty() const
    {
        std::vector<Def::ButtonGlyph> glyphs;
        glyphs.reserve(16);
#ifdef INPUT_DISABLED
        return glyphs;
#endif
        switch (getPhaseProperty())
        {
        case Def::Phase::Init:
            glyphs.push_back(Def::ButtonGlyph::InitGamerA);
            glyphs.push_back(Def::ButtonGlyph::InitGamerB);
            glyphs.push_back(Def::ButtonGlyph::InitGamerC);
            glyphs.push_back(Def::ButtonGlyph::InitSetup);
            glyphs.push_back(Def::ButtonGlyph::InitPlay);
            if (game1->getIsTrialModeProperty())
            {
                glyphs.push_back(Def::ButtonGlyph::InitBuy);
            }
            if (game1->getIsRankingModeProperty())
            {
                glyphs.push_back(Def::ButtonGlyph::InitRanking);
            }
            break;
        case Def::Phase::Play:
            glyphs.push_back(Def::ButtonGlyph::PlayPause);
            glyphs.push_back(Def::ButtonGlyph::PlayAction);
            glyphs.push_back(Def::ButtonGlyph::PlayJump);
            if (accelStarted)
            {
                glyphs.push_back(Def::ButtonGlyph::PlayDown);
            }
            glyphs.push_back(Def::ButtonGlyph::Cheat11);
            glyphs.push_back(Def::ButtonGlyph::Cheat12);
            glyphs.push_back(Def::ButtonGlyph::Cheat21);
            glyphs.push_back(Def::ButtonGlyph::Cheat22);
            glyphs.push_back(Def::ButtonGlyph::Cheat31);
            glyphs.push_back(Def::ButtonGlyph::Cheat32);
            break;
        case Def::Phase::Pause:
            glyphs.push_back(Def::ButtonGlyph::PauseMenu);
            if (mission != 1)
            {
                glyphs.push_back(Def::ButtonGlyph::PauseBack);
            }
            glyphs.push_back(Def::ButtonGlyph::PauseSetup);
            if (mission != 1 && mission % 10 != 0)
            {
                glyphs.push_back(Def::ButtonGlyph::PauseRestart);
            }
            glyphs.push_back(Def::ButtonGlyph::PauseContinue);
            break;
        case Def::Phase::Resume:
            glyphs.push_back(Def::ButtonGlyph::ResumeMenu);
            glyphs.push_back(Def::ButtonGlyph::ResumeContinue);
            break;
        case Def::Phase::Lost:
        case Def::Phase::Win:
            glyphs.push_back(Def::ButtonGlyph::WinLostReturn);
            break;
        case Def::Phase::Trial:
            glyphs.push_back(Def::ButtonGlyph::TrialBuy);
            glyphs.push_back(Def::ButtonGlyph::TrialCancel);
            break;
        case Def::Phase::MainSetup:
            glyphs.push_back(Def::ButtonGlyph::SetupSounds);
            glyphs.push_back(Def::ButtonGlyph::SetupJump);
            glyphs.push_back(Def::ButtonGlyph::SetupZoom);
            glyphs.push_back(Def::ButtonGlyph::SetupAccel);
            glyphs.push_back(Def::ButtonGlyph::SetupReset);
            glyphs.push_back(Def::ButtonGlyph::SetupReturn);
            break;
        case Def::Phase::PlaySetup:
            glyphs.push_back(Def::ButtonGlyph::SetupSounds);
            glyphs.push_back(Def::ButtonGlyph::SetupJump);
            glyphs.push_back(Def::ButtonGlyph::SetupZoom);
            glyphs.push_back(Def::ButtonGlyph::SetupAccel);
            glyphs.push_back(Def::ButtonGlyph::SetupReturn);
            break;
        case Def::Phase::Ranking:
            glyphs.push_back(Def::ButtonGlyph::RankingContinue);
            break;
        }
        if (showCheatMenu)
        {
            glyphs.push_back(Def::ButtonGlyph::Cheat1);
            glyphs.push_back(Def::ButtonGlyph::Cheat2);
            glyphs.push_back(Def::ButtonGlyph::Cheat3);
            glyphs.push_back(Def::ButtonGlyph::Cheat4);
            glyphs.push_back(Def::ButtonGlyph::Cheat5);
            glyphs.push_back(Def::ButtonGlyph::Cheat6);
            glyphs.push_back(Def::ButtonGlyph::Cheat7);
            glyphs.push_back(Def::ButtonGlyph::Cheat8);
            glyphs.push_back(Def::ButtonGlyph::Cheat9);
        }
        return glyphs;
    }

    TinyPoint InputPad::getPadCenterProperty() const
    {
#ifdef INPUT_DISABLED
        return {};
#endif
        TinyRect drawBounds = pixmap->getDrawBoundsProperty();
        int x = gameData->getJumpRightProperty() ? 100 : drawBounds.getWidthProperty() - 100;
        return {x, drawBounds.getHeightProperty() - 100};
    }

    InputPad::InputPad(IGame1* game1, Decor* decor, IPixmap* pixmap, ISound* sound, GameData* gameData) :
        game1(game1),
        decor(decor),
        pixmap(pixmap),
        sound(sound),
        gameData(gameData),
        accelSensor(Microsoft::Devices::Sensors::Accelerometer()),
        accelSlider(Slider(TinyPoint(320, 400), this->gameData->getAccelSensitivityProperty()))
    {
        using Microsoft::Devices::Sensors::AccelerometerReading;
        using Microsoft::Devices::Sensors::SensorBase;
        accelSensor.CurrentValueChanged +=
            [this](
            System::Object* /*sender*/,
            const Microsoft::Devices::Sensors::SensorReadingEventArgs<AccelerometerReading>&
            sensor_reading_event_args)
            {
                HandleAccelSensorCurrentValueChanged(sensor_reading_event_args);
            };

        lastButtonDown = Def::ButtonGlyph::None;
        buttonPressed = Def::ButtonGlyph::None;
    }

    void InputPad::StartMission(int mission)
    {
        this->mission = mission;
        accelWaitZero = true;
    }

    void InputPad::Update()
    {
#ifdef INPUT_DISABLED
        return;
#endif
        pressedGlyphs.clear();
        if (accelActive != gameData->getAccelActiveProperty())
        {
            accelActive = gameData->getAccelActiveProperty();
            if (accelActive)
            {
                StartAccel();
            }
            else
            {
                StopAccel();
            }
        }
        double horizontalChange = 0.0;
        double verticalChange = 0.0;
        int keyPress = 0;
        padPressed = false;
        Def::ButtonGlyph buttonGlyph = Def::ButtonGlyph::None;

        Microsoft::Xna::Framework::Input::Touch::TouchCollection touches{};
        bool touchScreenIsSupported = true;
        if (touchScreenIsSupported)
        {
            using Microsoft::Xna::Framework::Input::Touch::TouchPanel;
            touches = TouchPanel::GetState();
            touchOrClickCount = touches.getCountProperty();
        }

        std::vector<TinyPoint> touchesOrClicks;

        using Microsoft::Xna::Framework::Input::Touch::TouchLocation;
        using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
        if (touchScreenIsSupported)
            for (const TouchLocation& item : touches)
            {
                if (item.getStateProperty() == TouchLocationState::Pressed || item.getStateProperty() ==
                    TouchLocationState::Moved)
                {
                    TinyPoint touchPress{
                        static_cast<int>(item.getPositionProperty().X), static_cast<int>(item.getPositionProperty().Y)
                    };
                    touchesOrClicks.push_back(touchPress);
                }
            }

        using Microsoft::Xna::Framework::Input::MouseState;
        using Microsoft::Xna::Framework::Input::Mouse;
        using Microsoft::Xna::Framework::Input::ButtonState;
        MouseState mouseState = Mouse::GetState();
        if (mouseState.getLeftButtonProperty() == ButtonState::Pressed)
        {
            touchOrClickCount++;
            TinyPoint mouseClick(mouseState.getXProperty(), mouseState.getYProperty());
            touchesOrClicks.push_back(mouseClick);
        }

        float screenWidth = game1->getGraphics().getGraphicsDeviceProperty()->getViewportProperty().getWidthProperty();
        float screenHeight = game1->getGraphics().getGraphicsDeviceProperty()->getViewportProperty().
                                    getHeightProperty();
        float screenRatio = screenWidth / screenHeight;

        if ((CNA::getCurrentPlatform() == CNA::Platform::Android && screenRatio > 1.3333333333333333)
            /*|| Env.IMPL.isKNI()*/)
        {
            for (int i = 0; i < touchesOrClicks.size(); i++)
            {
                auto touchOrClick = touchesOrClicks[i];
                if (touchOrClick.X == -1) continue;

                float originalX = touchOrClick.X;
                float originalY = touchOrClick.Y;

                float widthHeightRatio = screenWidth / screenHeight;
                float heightRatio = 480 / screenHeight;
                float widthRatio = 640 / screenWidth;

                {
                    INPUT_DEBUG("-----");
                    INPUT_DEBUG("originalX=" + std::to_string(originalX));
                    INPUT_DEBUG("originalY=" + std::to_string(originalY));
                    INPUT_DEBUG("heightRatio=" + std::to_string(heightRatio));
                    INPUT_DEBUG("widthRatio=" + std::to_string(widthRatio));
                    INPUT_DEBUG("widthHeightRatio=" + std::to_string(widthHeightRatio));
                }
                if (screenHeight > 480)
                {
                    touchOrClick.X = (int)(originalX * heightRatio);
                    touchOrClick.Y = (int)(originalY * heightRatio);
                    touchesOrClicks[i] = touchOrClick;
                }

                using namespace std::string_literals;
                INPUT_DEBUG("new X"s + std::to_string(touchOrClick.X));
                INPUT_DEBUG("new Y"s + std::to_string(touchOrClick.Y));
            }
        }
        using Microsoft::Xna::Framework::Input::KeyboardState;
        using namespace Microsoft::Xna::Framework::Input;
        KeyboardState newKeyboardState = Keyboard::GetState();

        Keys keysToBeChecked[] = {
            Keys::LeftControl, Keys::Up, Keys::Right, Keys::Down, Keys::Left, Keys::Space, Keys::Escape,
        };
        for (Keys keys : keysToBeChecked)
        {
            if (newKeyboardState.IsKeyDown(keys)) touchesOrClicks.push_back(TinyPoint(-1, static_cast<int>(keys)));
        }
        static bool F11_pressed_previously = false;
        if (CNA::getCurrentPlatform() != CNA::Platform::Android && CNA::getCurrentPlatform() != CNA::Platform::Web &&
            newKeyboardState.IsKeyDown(Keys::F11) && !F11_pressed_previously)
        {
            F11_pressed_previously = true;
            game1->ToggleFullScreen();
            INPUT_DEBUG("F11 was pressed.");
        }
        else
        {
            F11_pressed_previously = false;
        }

        bool keyPressedUp = false;
        bool keyPressedDown = false;
        bool keyPressedLeft = false;
        bool keyPressedRight = false;
        for (TinyPoint touchOrClickItem : touchesOrClicks)
        {
            bool keyboardPressed = false;
            if (touchOrClickItem.X == -1)
            {
                keyboardPressed = true;
            }
            Keys keyPressed = keyboardPressed ? (Keys)touchOrClickItem.Y : Keys::None;
            keyPressedUp = keyPressed == Keys::Up ? true : keyPressedUp;
            keyPressedDown = keyPressed == Keys::Down ? true : keyPressedDown;
            keyPressedLeft = keyPressed == Keys::Left ? true : keyPressedLeft;
            keyPressedRight = keyPressed == Keys::Right ? true : keyPressedRight;

            {
                TinyPoint touchOrClick = keyboardPressed ? TinyPoint(1, 1) : touchOrClickItem;
                if (!accelStarted && Misc::IsInside(GetPadBounds(getPadCenterProperty(), padRadius), touchOrClick))
                {
                    padPressed = true;
                    padTouchPos = touchOrClick;
                }
                if (keyPressedUp || keyPressedDown || keyPressedLeft || keyPressedRight)
                {
                    padPressed = true;
                }
                INPUT_DEBUG(std::string("padPressed=") + (padPressed ? "true" : "false"));
                Def::ButtonGlyph pressedGlyph = ButtonDetect(touchOrClick);
                INPUT_DEBUG(std::string("pressedGlyph =") + std::to_string(static_cast<intcs>(pressedGlyph)));
                if (pressedGlyph != Def::ButtonGlyph::None)
                {
                    pressedGlyphs.push_back(pressedGlyph);
                }
                if (keyboardPressed)
                {
                    switch (keyPressed)
                    {
                    case Keys::LeftControl: pressedGlyph = Def::ButtonGlyph::PlayJump;
                        pressedGlyphs.push_back(pressedGlyph);
                        break;
                    case Keys::Space: pressedGlyph = Def::ButtonGlyph::PlayAction;
                        pressedGlyphs.push_back(pressedGlyph);
                        break;
                    case Keys::Escape: pressedGlyph = Def::ButtonGlyph::PlayPause;
                        pressedGlyphs.push_back(pressedGlyph);
                        break;
                    }
                }

                if ((getPhaseProperty() == Def::Phase::MainSetup || getPhaseProperty() == Def::Phase::PlaySetup) &&
                    accelSlider.Move(touchOrClick))
                {
                    gameData->setAccelSensitivityProperty(accelSlider.getValueProperty());
                }
                switch (pressedGlyph)
                {
                case Def::ButtonGlyph::PlayJump:
                    INPUT_DEBUG("Jumping detected");
                    accelWaitZero = false;
                    keyPress |= 1;
                    break;
                case Def::ButtonGlyph::PlayDown:
                    accelWaitZero = false;
                    keyPress |= 4;
                    break;
                case Def::ButtonGlyph::InitGamerA:
                case Def::ButtonGlyph::InitGamerB:
                case Def::ButtonGlyph::InitGamerC:
                case Def::ButtonGlyph::InitSetup:
                case Def::ButtonGlyph::InitPlay:
                case Def::ButtonGlyph::InitBuy:
                case Def::ButtonGlyph::InitRanking:
                case Def::ButtonGlyph::WinLostReturn:
                case Def::ButtonGlyph::TrialBuy:
                case Def::ButtonGlyph::TrialCancel:
                case Def::ButtonGlyph::SetupSounds:
                case Def::ButtonGlyph::SetupJump:
                case Def::ButtonGlyph::SetupZoom:
                case Def::ButtonGlyph::SetupAccel:
                case Def::ButtonGlyph::SetupReset:
                case Def::ButtonGlyph::SetupReturn:
                case Def::ButtonGlyph::PauseMenu:
                case Def::ButtonGlyph::PauseBack:
                case Def::ButtonGlyph::PauseSetup:
                case Def::ButtonGlyph::PauseRestart:
                case Def::ButtonGlyph::PauseContinue:
                case Def::ButtonGlyph::PlayPause:
                case Def::ButtonGlyph::PlayAction:
                case Def::ButtonGlyph::ResumeMenu:
                case Def::ButtonGlyph::ResumeContinue:
                case Def::ButtonGlyph::RankingContinue:
                case Def::ButtonGlyph::Cheat11:
                case Def::ButtonGlyph::Cheat12:
                case Def::ButtonGlyph::Cheat21:
                case Def::ButtonGlyph::Cheat22:
                case Def::ButtonGlyph::Cheat31:
                case Def::ButtonGlyph::Cheat32:
                case Def::ButtonGlyph::Cheat1:
                case Def::ButtonGlyph::Cheat2:
                case Def::ButtonGlyph::Cheat3:
                case Def::ButtonGlyph::Cheat4:
                case Def::ButtonGlyph::Cheat5:
                case Def::ButtonGlyph::Cheat6:
                case Def::ButtonGlyph::Cheat7:
                case Def::ButtonGlyph::Cheat8:
                case Def::ButtonGlyph::Cheat9:
                    accelWaitZero = false;
                    buttonGlyph = pressedGlyph;
                    showCheatMenu = false;
                    break;
                }
            }
        }

        if (
            Def::isNotOneOf(
                buttonGlyph, {
                    Def::ButtonGlyph::None,
                    Def::ButtonGlyph::PlayAction,
                    Def::ButtonGlyph::Cheat11,
                    Def::ButtonGlyph::Cheat12,
                    Def::ButtonGlyph::Cheat21,
                    Def::ButtonGlyph::Cheat22,
                    Def::ButtonGlyph::Cheat31,
                    Def::ButtonGlyph::Cheat32
                }
            )
            &&
            lastButtonDown == Def::ButtonGlyph::None)
        {
            TinyPoint pos(320, 240);
            sound->PlayImage(SoundChannel::SoundChannel0, pos);
        }
        if (buttonGlyph == Def::ButtonGlyph::None && lastButtonDown != Def::ButtonGlyph::None)
        {
            buttonPressed = lastButtonDown;
        }
        lastButtonDown = buttonGlyph;
        if (padPressed)
        {
            using namespace std::string_literals;
            using std::to_string;

            INPUT_DEBUG("getPadCenter().X="s + to_string(getPadCenterProperty().X));
            INPUT_DEBUG("getPadCenter().Y="s + to_string(getPadCenterProperty().Y));
            INPUT_DEBUG("padTouchPos.X="s + to_string(padTouchPos.X));
            INPUT_DEBUG("padTouchPos.Y="s + to_string(padTouchPos.Y));
            INPUT_DEBUG("keyPressedUp="s + to_string(keyPressedUp));
            INPUT_DEBUG("keyPressedDown="s + to_string(keyPressedDown));
            INPUT_DEBUG("keyPressedLeft="s + to_string(keyPressedLeft));
            INPUT_DEBUG("keyPressedRight="s + to_string(keyPressedRight));
            {
                if (keyPressedUp)
                {
                    padTouchPos.Y = getPadCenterProperty().Y - 30;
                    padTouchPos.X = getPadCenterProperty().X;
                    if (keyPressedLeft) padTouchPos.X = getPadCenterProperty().X - 30;
                    if (keyPressedRight) padTouchPos.X = getPadCenterProperty().X + 30;
                }
                if (keyPressedDown)
                {
                    padTouchPos.Y = getPadCenterProperty().Y + 30;
                    padTouchPos.X = getPadCenterProperty().X;
                    if (keyPressedLeft) padTouchPos.X = getPadCenterProperty().X - 30;
                    if (keyPressedRight) padTouchPos.X = getPadCenterProperty().X + 30;
                }
                if (keyPressedLeft)
                {
                    padTouchPos.X = getPadCenterProperty().X - 30;
                    padTouchPos.Y = getPadCenterProperty().Y;
                    if (keyPressedUp) padTouchPos.Y = getPadCenterProperty().Y - 30;
                    if (keyPressedDown) padTouchPos.Y = getPadCenterProperty().Y + 30;
                }
                if (keyPressedRight)
                {
                    padTouchPos.X = getPadCenterProperty().X + 30;
                    padTouchPos.Y = getPadCenterProperty().Y;
                    if (keyPressedUp) padTouchPos.Y = getPadCenterProperty().Y - 30;
                    if (keyPressedDown) padTouchPos.Y = getPadCenterProperty().Y + 30;
                }
            }
            double horizontalPosition = padTouchPos.X - getPadCenterProperty().X;
            double verticalPosition = padTouchPos.Y - getPadCenterProperty().Y;

            if (horizontalPosition > 20.0)
            {
                horizontalChange += 1.0;
                INPUT_DEBUG(" horizontalChange += 1.0;");
            }
            if (horizontalPosition < -20.0)
            {
                horizontalChange -= 1.0;
                INPUT_DEBUG(" horizontalChange -= 1.0;");
            }
            if (verticalPosition > 20.0)
            {
                verticalChange += 1.0;
                INPUT_DEBUG(" verticalPosition += 1.0;");
            }
            if (verticalPosition < -20.0)
            {
                verticalChange -= 1.0;
                INPUT_DEBUG(" verticalPosition -= 1.0;");
            }
        }
        if (accelStarted)
        {
            horizontalChange = accelSpeedX;
            verticalChange = 0.0;
            if (((unsigned int)keyPress & 4u) != 0)
            {
                verticalChange = 1.0;
            }
        }
        decor->SetSpeedX(horizontalChange);
        decor->SetSpeedY(verticalChange);
        decor->KeyChange(keyPress);
    }

    Def::ButtonGlyph InputPad::ButtonDetect(TinyPoint touchOrClick)
    {
#ifdef INPUT_DISABLED
        return Def::ButtonGlyph::None;
#endif
        const auto buttonGlyphs = getButtonGlyphsProperty();
        for (auto i = buttonGlyphs.rbegin(); i != buttonGlyphs.rend(); ++i)
        {
            Def::ButtonGlyph buttonGlyph = *i;
            TinyRect buttonRect = GetButtonRect(buttonGlyph);

            if (buttonGlyph == Def::ButtonGlyph::PlayJump || buttonGlyph == Def::ButtonGlyph::PlayAction || buttonGlyph
                == Def::ButtonGlyph::PlayDown || buttonGlyph == Def::ButtonGlyph::PlayPause)
            {
                buttonRect = Misc::Inflate(buttonRect, 20);
            }

            if (Misc::IsInside(buttonRect, touchOrClick))
            {
                return buttonGlyph;
            }
        }
        return Def::ButtonGlyph::None;
    }

    void InputPad::Draw()
    {
#ifdef INPUT_DISABLED
        return;
#endif
        if (!accelStarted && getPhaseProperty() == Def::Phase::Play)
        {
            pixmap->DrawIcon(PixmapChannel::Pad, 0, GetPadBounds(getPadCenterProperty(), padRadius / 2), 1.0, false);
            TinyPoint center = (padPressed ? padTouchPos : getPadCenterProperty());
            pixmap->DrawIcon(PixmapChannel::Pad, 1, GetPadBounds(center, padRadius / 2), 1.0, false);
        }
        for (Def::ButtonGlyph buttonGlyph : getButtonGlyphsProperty())
        {
            bool pressed = VECTOR_CONTAINS(pressedGlyphs, buttonGlyph);
            bool selected = false;
            if (buttonGlyph >= Def::ButtonGlyph::InitGamerA && buttonGlyph <= Def::ButtonGlyph::InitGamerC)
            {
                int selectedGamer = (int)(static_cast<intcs>(buttonGlyph) - 1);
                selected = selectedGamer == gameData->getSelectedGamerProperty();
            }
            if (buttonGlyph == Def::ButtonGlyph::SetupSounds)
            {
                selected = gameData->getSoundsProperty();
            }
            if (buttonGlyph == Def::ButtonGlyph::SetupJump)
            {
                selected = gameData->getJumpRightProperty();
            }
            if (buttonGlyph == Def::ButtonGlyph::SetupZoom)
            {
                selected = gameData->getAutoZoomProperty();
            }
            if (buttonGlyph == Def::ButtonGlyph::SetupAccel)
            {
                selected = gameData->getAccelActiveProperty();
            }
            pixmap->DrawInputButton(GetButtonRect(buttonGlyph), buttonGlyph, pressed, selected);
        }
        if ((getPhaseProperty() == Def::Phase::MainSetup || getPhaseProperty() == Def::Phase::PlaySetup) && gameData->
            getAccelActiveProperty())
        {
            accelSlider.Draw(*pixmap);
        }
    }

    TinyRect InputPad::GetPadBounds(TinyPoint center, int radius)
    {
        return TinyRect(center.X - radius, center.X + radius, center.Y - radius, center.Y + radius);
    }

    TinyRect InputPad::GetButtonRect(Def::ButtonGlyph glyph)
    {
#ifdef INPUT_DISABLED
        return {};
#endif
        TinyRect drawBounds = pixmap->getDrawBoundsProperty();
        double drawBoundsWidth = drawBounds.getWidthProperty();
        double drawBoundsHeight = drawBounds.getHeightProperty();
        double buttonSizeFactor1 = drawBoundsHeight / 5.0;
        double buttonSizeFactor2 = drawBoundsHeight * 140.0 / 480.0;
        double cheatButtonSizeFactor = drawBoundsHeight / 3.5;
        if (glyph >= Def::ButtonGlyph::Cheat1 && glyph <= Def::ButtonGlyph::Cheat9)
        {
            int cheatNumber = (int)(static_cast<intcs>(glyph) - 35);
            TinyRect result = TinyRect();
            result.Left = 80 * cheatNumber;
            result.Right = 80 * (cheatNumber + 1);
            result.Top = 0;
            result.Bottom = 80;
            return result;
        }
        int leftXForButtonsInLeftColumn = (int)(20.0 + buttonSizeFactor2 * 0.0);
        int rightXForButtonsInLeftColumn = (int)(20.0 + buttonSizeFactor2 * 0.5);
        switch (glyph)
        {
        case Def::ButtonGlyph::InitGamerA:
            {
                TinyRect result19 = TinyRect();
                result19.Left = leftXForButtonsInLeftColumn;
                result19.Right = rightXForButtonsInLeftColumn;
                result19.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 2.1);
                result19.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.6);
                return result19;
            }
        case Def::ButtonGlyph::InitGamerB:
            {
                TinyRect result18 = TinyRect();
                result18.Left = leftXForButtonsInLeftColumn;
                result18.Right = rightXForButtonsInLeftColumn;
                result18.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.6);
                result18.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.1);
                return result18;
            }
        case Def::ButtonGlyph::InitGamerC:
            {
                TinyRect result15 = TinyRect();
                result15.Left = leftXForButtonsInLeftColumn;
                result15.Right = rightXForButtonsInLeftColumn;
                result15.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.1);
                result15.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.6);
                return result15;
            }
        case Def::ButtonGlyph::InitSetup:
            {
                TinyRect result14 = TinyRect();
                result14.Left = leftXForButtonsInLeftColumn;
                result14.Right = rightXForButtonsInLeftColumn;
                result14.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.5);
                result14.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.0);
                return result14;
            }
        case Def::ButtonGlyph::InitPlay:
            {
                TinyRect result11 = TinyRect();
                result11.Left = (int)(drawBoundsWidth - 20.0 - buttonSizeFactor2 * 1.0);
                result11.Right = (int)(drawBoundsWidth - 20.0 - buttonSizeFactor2 * 0.0);
                result11.Top = (int)(drawBoundsHeight - 40.0 - buttonSizeFactor2 * 1.0);
                result11.Bottom = (int)(drawBoundsHeight - 40.0 - buttonSizeFactor2 * 0.0);
                return result11;
            }
        case Def::ButtonGlyph::InitBuy:
        case Def::ButtonGlyph::InitRanking:
            {
                TinyRect result10 = TinyRect();
                result10.Left = (int)(drawBoundsWidth - 20.0 - buttonSizeFactor2 * 0.75);
                result10.Right = (int)(drawBoundsWidth - 20.0 - buttonSizeFactor2 * 0.25);
                result10.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 2.1);
                result10.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.6);
                return result10;
            }
        case Def::ButtonGlyph::PauseMenu:
            {
                TinyRect result37 = TinyRect();
                result37.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * -0.21);
                result37.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 0.79);
                result37.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.2);
                result37.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.2);
                return result37;
            }
        case Def::ButtonGlyph::PauseBack:
            {
                TinyRect result36 = TinyRect();
                result36.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 0.79);
                result36.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 1.79);
                result36.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.2);
                result36.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.2);
                return result36;
            }
        case Def::ButtonGlyph::PauseSetup:
            {
                TinyRect result35 = TinyRect();
                result35.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 1.79);
                result35.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 2.79);
                result35.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.2);
                result35.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.2);
                return result35;
            }
        case Def::ButtonGlyph::PauseRestart:
            {
                TinyRect result34 = TinyRect();
                result34.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 2.79);
                result34.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 3.79);
                result34.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.2);
                result34.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.2);
                return result34;
            }
        case Def::ButtonGlyph::PauseContinue:
            {
                TinyRect result33 = TinyRect();
                result33.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 3.79);
                result33.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 4.79);
                result33.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.2);
                result33.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.2);
                return result33;
            }
        case Def::ButtonGlyph::ResumeMenu:
            {
                TinyRect result32 = TinyRect();
                result32.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 1.29);
                result32.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 2.29);
                result32.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.2);
                result32.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.2);
                return result32;
            }
        case Def::ButtonGlyph::ResumeContinue:
            {
                TinyRect result31 = TinyRect();
                result31.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 2.29);
                result31.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 3.29);
                result31.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.2);
                result31.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.2);
                return result31;
            }
        case Def::ButtonGlyph::WinLostReturn:
            {
                TinyRect result30 = TinyRect();
                result30.Left = (int)((double)getPixmapOriginProperty().X + drawBoundsWidth - buttonSizeFactor1 * 2.2);
                result30.Right = (int)((double)getPixmapOriginProperty().X + drawBoundsWidth - buttonSizeFactor1 * 1.2);
                result30.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor1 * 0.2);
                result30.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor1 * 1.2);
                return result30;
            }
        case Def::ButtonGlyph::TrialBuy:
            {
                TinyRect result29 = TinyRect();
                result29.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 2.5);
                result29.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 3.5);
                result29.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.1);
                result29.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.1);
                return result29;
            }
        case Def::ButtonGlyph::TrialCancel:
            {
                TinyRect result28 = TinyRect();
                result28.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 3.5);
                result28.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 4.5);
                result28.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.1);
                result28.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.1);
                return result28;
            }
        case Def::ButtonGlyph::RankingContinue:
            {
                TinyRect result27 = TinyRect();
                result27.Left = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 3.5);
                result27.Right = (int)((double)getPixmapOriginProperty().X + buttonSizeFactor2 * 4.5);
                result27.Top = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 2.1);
                result27.Bottom = (int)((double)getPixmapOriginProperty().Y + buttonSizeFactor2 * 3.1);
                return result27;
            }
        case Def::ButtonGlyph::SetupSounds:
            {
                TinyRect result26 = TinyRect();
                result26.Left = leftXForButtonsInLeftColumn;
                result26.Right = rightXForButtonsInLeftColumn;
                result26.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 2.0);
                result26.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.5);
                return result26;
            }
        case Def::ButtonGlyph::SetupJump:
            {
                TinyRect result25 = TinyRect();
                result25.Left = leftXForButtonsInLeftColumn;
                result25.Right = rightXForButtonsInLeftColumn;
                result25.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.5);
                result25.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.0);
                return result25;
            }
        case Def::ButtonGlyph::SetupZoom:
            {
                TinyRect result24 = TinyRect();
                result24.Left = leftXForButtonsInLeftColumn;
                result24.Right = rightXForButtonsInLeftColumn;
                result24.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.0);
                result24.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.5);
                return result24;
            }
        case Def::ButtonGlyph::SetupAccel:
            {
                TinyRect result23 = TinyRect();
                result23.Left = leftXForButtonsInLeftColumn;
                result23.Right = rightXForButtonsInLeftColumn;
                result23.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.5);
                result23.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.0);
                return result23;
            }
        case Def::ButtonGlyph::SetupReset:
            {
                TinyRect result22 = TinyRect();
                result22.Left = (int)(450.0 + buttonSizeFactor2 * 0.0);
                result22.Right = (int)(450.0 + buttonSizeFactor2 * 0.5);
                result22.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 2.0);
                result22.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 1.5);
                return result22;
            }
        case Def::ButtonGlyph::SetupReturn:
            {
                TinyRect result21 = TinyRect();
                result21.Left = (int)(drawBoundsWidth - 20.0 - buttonSizeFactor2 * 0.8);
                result21.Right = (int)(drawBoundsWidth - 20.0 - buttonSizeFactor2 * 0.0);
                result21.Top = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.8);
                result21.Bottom = (int)(drawBoundsHeight - 20.0 - buttonSizeFactor2 * 0.0);
                return result21;
            }
        case Def::ButtonGlyph::PlayPause:
            {
                TinyRect result20 = TinyRect();
                result20.Left = (int)(drawBoundsWidth - buttonSizeFactor1 * 0.7);
                result20.Right = (int)(drawBoundsWidth - buttonSizeFactor1 * 0.2);
                result20.Top = (int)(buttonSizeFactor1 * 0.2);
                result20.Bottom = (int)(buttonSizeFactor1 * 0.7);
                return result20;
            }
        case Def::ButtonGlyph::PlayAction:
            {
                if (gameData->getJumpRightProperty())
                {
                    TinyRect result16 = TinyRect();
                    result16.Left = (int)((double)drawBounds.getWidthProperty() - buttonSizeFactor1 * 1.2);
                    result16.Right = (int)((double)drawBounds.getWidthProperty() - buttonSizeFactor1 * 0.2);
                    result16.Top = (int)(drawBoundsHeight - buttonSizeFactor1 * 2.6);
                    result16.Bottom = (int)(drawBoundsHeight - buttonSizeFactor1 * 1.6);
                    return result16;
                }
                TinyRect result17 = TinyRect();
                result17.Left = (int)(buttonSizeFactor1 * 0.2);
                result17.Right = (int)(buttonSizeFactor1 * 1.2);
                result17.Top = (int)(drawBoundsHeight - buttonSizeFactor1 * 2.6);
                result17.Bottom = (int)(drawBoundsHeight - buttonSizeFactor1 * 1.6);
                return result17;
            }
        case Def::ButtonGlyph::PlayJump:
            {
                if (gameData->getJumpRightProperty())
                {
                    TinyRect result12 = TinyRect();
                    result12.Left = (int)((double)drawBounds.getWidthProperty() - buttonSizeFactor1 * 1.2);
                    result12.Right = (int)((double)drawBounds.getWidthProperty() - buttonSizeFactor1 * 0.2);
                    result12.Top = (int)(drawBoundsHeight - buttonSizeFactor1 * 1.2);
                    result12.Bottom = (int)(drawBoundsHeight - buttonSizeFactor1 * 0.2);
                    return result12;
                }
                TinyRect result13 = TinyRect();
                result13.Left = (int)(buttonSizeFactor1 * 0.2);
                result13.Right = (int)(buttonSizeFactor1 * 1.2);
                result13.Top = (int)(drawBoundsHeight - buttonSizeFactor1 * 1.2);
                result13.Bottom = (int)(drawBoundsHeight - buttonSizeFactor1 * 0.2);
                return result13;
            }
        case Def::ButtonGlyph::PlayDown:
            {
                if (gameData->getJumpRightProperty())
                {
                    TinyRect result8 = TinyRect();
                    result8.Left = (int)(buttonSizeFactor1 * 0.2);
                    result8.Right = (int)(buttonSizeFactor1 * 1.2);
                    result8.Top = (int)(drawBoundsHeight - buttonSizeFactor1 * 1.2);
                    result8.Bottom = (int)(drawBoundsHeight - buttonSizeFactor1 * 0.2);
                    return result8;
                }
                TinyRect result9 = TinyRect();
                result9.Left = (int)((double)drawBounds.getWidthProperty() - buttonSizeFactor1 * 1.2);
                result9.Right = (int)((double)drawBounds.getWidthProperty() - buttonSizeFactor1 * 0.2);
                result9.Top = (int)(drawBoundsHeight - buttonSizeFactor1 * 1.2);
                result9.Bottom = (int)(drawBoundsHeight - buttonSizeFactor1 * 0.2);
                return result9;
            }
        case Def::ButtonGlyph::Cheat11:
            {
                TinyRect result7 = TinyRect();
                result7.Left = (int)(cheatButtonSizeFactor * 0.0);
                result7.Right = (int)(cheatButtonSizeFactor * 1.0);
                result7.Top = (int)(cheatButtonSizeFactor * 0.0);
                result7.Bottom = (int)(cheatButtonSizeFactor * 1.0);
                return result7;
            }
        case Def::ButtonGlyph::Cheat12:
            {
                TinyRect result6 = TinyRect();
                result6.Left = (int)(cheatButtonSizeFactor * 0.0);
                result6.Right = (int)(cheatButtonSizeFactor * 1.0);
                result6.Top = (int)(cheatButtonSizeFactor * 1.0);
                result6.Bottom = (int)(cheatButtonSizeFactor * 2.0);
                return result6;
            }
        case Def::ButtonGlyph::Cheat21:
            {
                TinyRect result5 = TinyRect();
                result5.Left = (int)(cheatButtonSizeFactor * 1.0);
                result5.Right = (int)(cheatButtonSizeFactor * 2.0);
                result5.Top = (int)(cheatButtonSizeFactor * 0.0);
                result5.Bottom = (int)(cheatButtonSizeFactor * 1.0);
                return result5;
            }
        case Def::ButtonGlyph::Cheat22:
            {
                TinyRect result4 = TinyRect();
                result4.Left = (int)(cheatButtonSizeFactor * 1.0);
                result4.Right = (int)(cheatButtonSizeFactor * 2.0);
                result4.Top = (int)(cheatButtonSizeFactor * 1.0);
                result4.Bottom = (int)(cheatButtonSizeFactor * 2.0);
                return result4;
            }
        case Def::ButtonGlyph::Cheat31:
            {
                TinyRect result3 = TinyRect();
                result3.Left = (int)(cheatButtonSizeFactor * 2.0);
                result3.Right = (int)(cheatButtonSizeFactor * 3.0);
                result3.Top = (int)(cheatButtonSizeFactor * 0.0);
                result3.Bottom = (int)(cheatButtonSizeFactor * 1.0);
                return result3;
            }
        case Def::ButtonGlyph::Cheat32:
            {
                TinyRect result2 = TinyRect();
                result2.Left = (int)(cheatButtonSizeFactor * 2.0);
                result2.Right = (int)(cheatButtonSizeFactor * 3.0);
                result2.Top = (int)(cheatButtonSizeFactor * 1.0);
                result2.Bottom = (int)(cheatButtonSizeFactor * 2.0);
                return result2;
            }
        default:
            return TinyRect();
        }
    }

    void InputPad::StartAccel()
    {
#ifdef INPUT_DISABLED
        return;
#endif
        try
        {
            accelSensor.Start();
            accelStarted = true;
        }
        catch (Microsoft::Devices::Sensors::AccelerometerFailedException)
        {
            accelStarted = false;
        }
        catch (System::UnauthorizedAccessException)
        {
            accelStarted = false;
        }
    }

    void InputPad::StopAccel()
    {
#ifdef INPUT_DISABLED
        return;
#endif
        if (accelStarted)
        {
            try
            {
                accelSensor.Stop();
            }
            catch (Microsoft::Devices::Sensors::AccelerometerFailedException)
            {
            }
            accelStarted = false;
        }
    }


    void InputPad::HandleAccelSensorCurrentValueChanged(
        Microsoft::Devices::Sensors::SensorReadingEventArgs<Microsoft::Devices::Sensors::AccelerometerReading> e)
    {
#ifdef INPUT_DISABLED
        return;
#endif
        //IL_0001: Unknown result type (might be due to invalid IL or missing references)
        //IL_0006: Unknown result type (might be due to invalid IL or missing references)

        Microsoft::Devices::Sensors::AccelerometerReading sensorReading = e.getSensorReadingProperty();
        float y = ((Microsoft::Devices::Sensors::AccelerometerReading)(sensorReading)).getAccelerationProperty().Y;
        float sensitivityThreshold = (1.0f - (float)gameData->getAccelSensitivityProperty()) * 0.06f + 0.04f;
        float adjustedThreshold = (accelLastState ? (sensitivityThreshold * 0.6f) : sensitivityThreshold);
        if (y > adjustedThreshold)
        {
            accelSpeedX = 0.0 - std::min((double)y * 0.25 / (double)sensitivityThreshold + 0.25, 1.0);
        }
        else if (y < 0.0f - adjustedThreshold)
        {
            accelSpeedX = std::min((double)(0.0f - y) * 0.25 / (double)sensitivityThreshold + 0.25, 1.0);
        }
        else
        {
            accelSpeedX = 0.0;
        }
        accelLastState = accelSpeedX != 0.0;
        if (accelWaitZero)
        {
            if (accelSpeedX == 0.0)
            {
                accelWaitZero = false;
            }
            else
            {
                accelSpeedX = 0.0;
            }
        }
    }
}
