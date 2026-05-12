#include "GalaxyEggbert/Game1.hpp"

#include <string>

#include "CNA/Logger.hpp"
#include "Microsoft/Xna/Framework/GamerServices/Guide.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseCursor.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/EventArgs.hpp"
#include "System/String.hpp"
#include "GalaxyEggbert/Helper.hpp"
#include "GalaxyEggbert/MyResource.hpp"
#include "GalaxyEggbert/Pixmap.hpp"
#include "GalaxyEggbert/Sound.hpp"
#include "GalaxyEggbert/Tables.hpp"
#include "GalaxyEggbert/Text.hpp"

namespace GalaxyEggbert
{
    bool Game1::getIsRankingModeProperty() const
    {
        if (!simulateTrialMode)
        {
            return isTrialMode;
        }
        return true;
    }

    bool Game1::getIsTrialModeProperty() const { return false; }

    Game1::Game1() : graphics(this),
                     gameData(), startTime(System::TimeSpan(0)),
                     pixmap(std::make_shared<Pixmap>(this, graphics)),
                     sound(std::make_shared<Sound>(this, gameData)),
                     decor(),
                     waitJauge(),
                     inputPad(this, &decor, pixmap.get(), sound.get(), &gameData)
    {
        Tables::Init();
        getWindowProperty().setTitleProperty("Speedy Blupi");

        Exiting += [this](
            System::Object* /*sender*/,
            const Microsoft::Xna::Framework::ExitingEventArgs& args)
            {
                OnExiting(args);
            };

#if KNI
        Deactivated += OnDeactivated;
        Activated += OnActivated;
#endif

        bool touchPanelConnected = true;
        using Microsoft::Xna::Framework::Input::Touch::TouchPanel;
        if (!TouchPanel::GetCapabilities().getIsConnectedProperty())
        {
            Game::setIsMouseVisibleProperty(true);
            using Microsoft::Xna::Framework::Input::Mouse;
            using Microsoft::Xna::Framework::Input::MouseCursor;
            Mouse::SetCursor(MouseCursor::Arrow); //TODO: Is it XNA 4.0?
        }

        graphics.setIsFullScreenProperty(false);
        string content = "Content";
        Game::getContentProperty().setRootDirectoryProperty(content);
        Game::setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(500000L));
        Game::setInactiveSleepTimeProperty(System::TimeSpan::FromSeconds(1.0));
        missionToStart1 = -1;
        missionToStart2 = -1;


        decor.Create(sound.get(), pixmap.get(), &gameData);
        TinyPoint pos
        {
            196,
            426
        };

        waitJauge.Create(pixmap.get(), sound.get(), pos, JaugeMode::Yellow, false);
        waitJauge.SetHide(false);
        waitJauge.setZoomProperty(2.0);
        phase = Def::Phase::None;
        fadeOutPhase = Def::Phase::None;

        SetPhase(Def::Phase::First);
    }

    Game1::~Game1()
    {
    }

    void Game1::Initialize()
    {
        Microsoft::Xna::Framework::Game::Initialize();
    }

    void Game1::LoadContent()
    {
        CNA::Logger::Trace("Game1::LoadContent");
        pixmap->BackgroundCache("wait");
    }

    void Game1::UnloadContent()
    {
    }

    void Game1::OnDeactivated(std::any sender, System::EventArgs args)
    {
        if (phase == Def::Phase::Play)
        {
            decor.CurrentWrite();
        }
        else
        {
            decor.CurrentDelete();
        }

        Game::OnDeactivated(sender, args);
    }

    void Game1::OnActivated(std::any sender, System::EventArgs args)
    {
        continueMission = 1;
        Game::OnActivated(sender, args);
    }

    void Game1::OnExiting(const Microsoft::Xna::Framework::ExitingEventArgs args)
    {
        decor.CurrentDelete();
    }

    void Game1::Update(Microsoft::Xna::Framework::GameTime& gameTime)
    {
        // CNA::Logger::Trace("Game1::Update");
        using Microsoft::Xna::Framework::Input::GamePad;
        using Microsoft::Xna::Framework::PlayerIndex;
        using Microsoft::Xna::Framework::Input::ButtonState;
        if (GamePad::GetState(PlayerIndex::One).getButtonsProperty().getBackProperty() == ButtonState::Pressed)
        {
            if (phase == Def::Phase::Play)
            {
                SetPhase(Def::Phase::Pause);
            }
            else if (phase == Def::Phase::PlaySetup)
            {
                SetPhase(Def::Phase::Play, -1);
            }
            else if (phase != Def::Phase::Init)
            {
                SetPhase(Def::Phase::Init);
            }
            else
            {
                Exit();
            }
            return;
        }
        phaseTime++;
        if (fadeOutPhase != Def::Phase::None)
        {
            if (phaseTime >= 20)
            {
                SetPhase(fadeOutPhase);
            }
            return;
        }
        if (missionToStart2 != -1)
        {
            SetPhase(Def::Phase::Play, missionToStart2);
            return;
        }
        if (phase == Def::Phase::First)
        {
            startTime = gameTime.getTotalGameTimeProperty();
            pixmap->LoadContent();
            sound->LoadContent();
            gameData.Read();
            inputPad.setPixmapOriginProperty(pixmap->getOriginProperty());
            SetPhase(Def::Phase::Wait);
            return;
        }
        if (phase == Def::Phase::Wait)
        {
            if (continueMission == 2)
            {
                continueMission = 0;
                if (decor.CurrentRead())
                {
                    SetPhase(Def::Phase::Resume);
                    return;
                }
            }
            long num = gameTime.getTotalGameTimeProperty().getTicksProperty() - startTime.getTicksProperty();
            waitProgress = (double)num / 50000000.0;
            if (waitProgress > 1.0)
            {
                SetPhase(Def::Phase::Init);
            }
            return;
        }
        inputPad.Update();
        Def::ButtonGlyph buttonPressed = inputPad.getButtonPressedProperty();
        if (buttonPressed >= Def::ButtonGlyph::InitGamerA && buttonPressed <= Def::ButtonGlyph::InitGamerC)
        {
            SetGamer((int)(static_cast<intcs>(buttonPressed) - 1));
            return;
        }
        switch (buttonPressed)
        {
        case Def::ButtonGlyph::InitSetup:
            SetPhase(Def::Phase::MainSetup);
            return;
        case Def::ButtonGlyph::PauseSetup:
            SetPhase(Def::Phase::PlaySetup);
            return;
        case Def::ButtonGlyph::SetupSounds:
            gameData.setSoundsProperty(!gameData.getSoundsProperty());
            gameData.Write();
            return;
        case Def::ButtonGlyph::SetupJump:
            gameData.setJumpRightProperty(!gameData.getJumpRightProperty());
            gameData.Write();
            return;
        case Def::ButtonGlyph::SetupZoom:
            gameData.setAutoZoomProperty(!gameData.getAutoZoomProperty());
            gameData.Write();
            return;
        case Def::ButtonGlyph::SetupAccel:
            gameData.setAccelActiveProperty(!gameData.getAccelActiveProperty());
            gameData.Write();
            return;
        case Def::ButtonGlyph::SetupReset:
            gameData.Reset();
            gameData.Write();
            return;
        case Def::ButtonGlyph::SetupReturn:
            if (playSetup)
            {
                SetPhase(Def::Phase::Play, -1);
            }
            else
            {
                SetPhase(Def::Phase::Init);
            }
            return;
        case Def::ButtonGlyph::InitPlay:
            SetPhase(Def::Phase::Play, 1);
            return;
        case Def::ButtonGlyph::PlayPause:
            SetPhase(Def::Phase::Pause);
            return;
        case Def::ButtonGlyph::WinLostReturn:
        case Def::ButtonGlyph::PauseMenu:
        case Def::ButtonGlyph::ResumeMenu:
            SetPhase(Def::Phase::Init);
            break;
        }
        switch (buttonPressed)
        {
        case Def::ButtonGlyph::ResumeContinue:
            ContinueMission();
            return;
        case Def::ButtonGlyph::InitBuy:
        case Def::ButtonGlyph::TrialBuy:
            Microsoft::Xna::Framework::GamerServices::Guide::Show(PlayerIndex::One);
            SetPhase(Def::Phase::Init);
            return;
        case Def::ButtonGlyph::InitRanking:
            SetPhase(Def::Phase::Ranking);
            return;
        case Def::ButtonGlyph::TrialCancel:
        case Def::ButtonGlyph::RankingContinue:
            SetPhase(Def::Phase::Init);
            return;
        case Def::ButtonGlyph::PauseBack:
            MissionBack();
            return;
        case Def::ButtonGlyph::PauseRestart:
            SetPhase(Def::Phase::Play, mission);
            return;
        case Def::ButtonGlyph::PauseContinue:
            SetPhase(Def::Phase::Play, -1);
            return;
        case Def::ButtonGlyph::Cheat11:
        case Def::ButtonGlyph::Cheat12:
        case Def::ButtonGlyph::Cheat21:
        case Def::ButtonGlyph::Cheat22:
        case Def::ButtonGlyph::Cheat31:
        case Def::ButtonGlyph::Cheat32:
            if (buttonPressed == cheatGeste[cheatGesteIndex])
            {
                cheatGesteIndex++;
                if (cheatGesteIndex == cheatGesteLength)
                {
                    cheatGesteIndex = 0;
                    inputPad.setShowCheatMenuProperty(true);
                }
            }
            else
            {
                cheatGesteIndex = 0;
            }
            break;
        default:
            if (buttonPressed != Def::ButtonGlyph::None)
            {
                cheatGesteIndex = 0;
            }
            break;
        }
        if (buttonPressed >= Def::ButtonGlyph::Cheat1 && buttonPressed <= Def::ButtonGlyph::Cheat9)
        {
            CheatAction(buttonPressed);
        }
        if (phase == Def::Phase::Play)
        {
            decor.setButtonPressedProperty(buttonPressed);
            decor.MoveStep();
            int num2 = decor.IsTerminated();
            if (num2 != 0)
            {
                CNA::Logger::Debug("Game1::Update levelTerminated term=" + std::to_string(num2)
                    + " mission=" + std::to_string(mission));
            }
            if (num2 == -1)
            {
                MemorizeGamerProgress();
                SetPhase(Def::Phase::Lost);
            }
            else if (num2 == -2)
            {
                MemorizeGamerProgress();
                SetPhase(Def::Phase::Win);
            }
            else if (num2 >= 1)
            {
                MemorizeGamerProgress();
                StartMission(num2);
            }
        }
        Game::Update(gameTime);
    }

    void Game1::MissionBack()
    {
        int num = mission;
        if (num == 1)
        {
            SetPhase(Def::Phase::Init);
            return;
        }
        num = ((num % 10 == 0) ? 1 : (num / 10 * 10));
        SetPhase(Def::Phase::Play, num);
    }

    void Game1::StartMission(int mission)
    {
        if (mission > 20 && mission % 10 > 1 && getIsTrialModeProperty())
        {
            SetPhase(Def::Phase::Trial);
            return;
        }
        this->mission = mission;
        if (this->mission != 1)
        {
            gameData.setLastWorldProperty(this->mission / 10);
        }
        decor.Read(0, this->mission, false);
        decor.LoadImages();
        decor.SetMission(this->mission);
        decor.SetNbVies(gameData.getNbViesProperty());
        decor.InitializeDoors(gameData);
        decor.AdaptDoors(false);
        decor.MainSwitchInitialize(gameData.getLastWorldProperty());
        decor.PlayPrepare(false);
        decor.StartSound();
        inputPad.StartMission(this->mission);
    }

    void Game1::ContinueMission()
    {
        SetPhase(Def::Phase::Play, -2);
        mission = decor.GetMission();
        if (mission != 1)
        {
            gameData.setLastWorldProperty(mission / 10);
        }
        decor.LoadImages();
        decor.StartSound();
        inputPad.StartMission(mission);
    }

    void Game1::CheatAction(Def::ButtonGlyph glyph)
    {
        switch (glyph)
        {
        case Def::ButtonGlyph::Cheat1:
            decor.CheatAction(Tables::CheatCodes::OpenDoors);
            break;
        case Def::ButtonGlyph::Cheat2:
            decor.CheatAction(Tables::CheatCodes::SuperBlupi);
            break;
        case Def::ButtonGlyph::Cheat3:
            decor.CheatAction(Tables::CheatCodes::ShowSecret);
            break;
        case Def::ButtonGlyph::Cheat4:
            decor.CheatAction(Tables::CheatCodes::LayEgg);
            break;
        case Def::ButtonGlyph::Cheat5:
            gameData.Reset();
            break;
        case Def::ButtonGlyph::Cheat6:
            simulateTrialMode = !simulateTrialMode;
            break;
        case Def::ButtonGlyph::Cheat7:
            decor.CheatAction(Tables::CheatCodes::CleanAll);
            break;
        case Def::ButtonGlyph::Cheat8:
            decor.CheatAction(Tables::CheatCodes::AllTreasure);
            break;
        case Def::ButtonGlyph::Cheat9:
            decor.CheatAction(Tables::CheatCodes::EndGoal);
            break;
        }
    }

    void Game1::Draw(const Microsoft::Xna::Framework::GameTime& gameTime)
    {
        // CNA::Logger::Trace("Game1::Draw");
        if (continueMission == 1)
        {
            continueMission = 2;
        }
        if (phase == Def::Phase::Wait || phase == Def::Phase::Init || phase == Def::Phase::Pause || phase ==
            Def::Phase::Resume || phase == Def::Phase::Lost || phase == Def::Phase::Win || phase ==
            Def::Phase::MainSetup || phase == Def::Phase::PlaySetup || phase == Def::Phase::Trial || phase ==
            Def::Phase::Ranking)
        {
            pixmap->DrawBackground();
            if (fadeOutPhase == Def::Phase::None && missionToStart1 != -1)
            {
                missionToStart2 = missionToStart1;
                missionToStart1 = -1;
            }
            else
            {
                DrawBackgroundFade();
                if (fadeOutPhase == Def::Phase::None)
                {
                    DrawButtonsBackground();
                    inputPad.Draw();
                    DrawButtonsText();
                }
            }
        }
        else if (phase == Def::Phase::Play)
        {
            decor.Build();
            inputPad.Draw();
        }
        if (phase == Def::Phase::Wait)
        {
            DrawWaitProgress();
        }
        Game::Draw(gameTime);
    }

    void Game1::DrawBackgroundFade()
    {
        if (phase == Def::Phase::Init)
        {
            double num = std::min((double)phaseTime / 20.0, 1.0);
            TinyRect rect;
            double opacity;
            if (fadeOutPhase == Def::Phase::MainSetup)
            {
                num = (1.0 - num) * (1.0 - num);
                TinyRect tinyRect = TinyRect();
                tinyRect.Left = (int)(720.0 - 640.0 * num);
                tinyRect.Right = (int)(1360.0 - 640.0 * num);
                tinyRect.Top = 0;
                tinyRect.Bottom = 160;
                rect = tinyRect;
                opacity = num * num;
            }
            else
            {
                num = ((fadeOutPhase != Def::Phase::None) ? (1.0 - num * 2.0) : (1.0 - (1.0 - num) * (1.0 - num)));
                TinyRect tinyRect2 = TinyRect();
                tinyRect2.Left = 80;
                tinyRect2.Right = 720;
                tinyRect2.Top = (int)(-160.0 + num * 160.0);
                tinyRect2.Bottom = (int)(0.0 + num * 160.0);
                rect = tinyRect2;
                opacity = 1.0;
            }
            pixmap->DrawIcon(PixmapChannel::SpeedyBlupiBackground, 0, rect, opacity, false);
        }
        if (phase == Def::Phase::Init)
        {
            double num = std::min((double)phaseTime / 20.0, 1.0);
            double opacity;
            if (fadeOutPhase == Def::Phase::MainSetup)
            {
                opacity = (1.0 - num) * (1.0 - num);
                num = 1.0;
            }
            else if (fadeOutPhase == Def::Phase::None)
            {
                num = 0.5 + num / 2.0;
                opacity = std::min(num * num, 1.0);
            }
            else
            {
                opacity = 1.0 - num;
                num = 1.0 + num * 10.0;
            }
            TinyRect tinyRect3 = TinyRect();
            tinyRect3.Left = (int)(468.0 - 205.0 * num);
            tinyRect3.Right = (int)(468.0 + 205.0 * num);
            tinyRect3.Top = (int)(280.0 - 190.0 * num);
            tinyRect3.Bottom = (int)(280.0 + 190.0 * num);
            TinyRect rect = tinyRect3;
            pixmap->DrawIcon(PixmapChannel::BlupiYoupieBackground, 0, rect, opacity, 0.0, false);
        }
        if (phase == Def::Phase::Pause || phase == Def::Phase::Resume)
        {
            if (fadeOutPhase == Def::Phase::Play)
            {
                double num = std::min((double)phaseTime / 20.0, 1.0);
                double opacity = 1.0 - num;
                num = 1.0 + num * 10.0;
                TinyRect tinyRect4 = TinyRect();
                tinyRect4.Left = (int)(418.0 - 205.0 * num);
                tinyRect4.Right = (int)(418.0 + 205.0 * num);
                tinyRect4.Top = (int)(190.0 - 190.0 * num);
                tinyRect4.Bottom = (int)(190.0 + 190.0 * num);
                TinyRect rect = tinyRect4;
                pixmap->DrawIcon(PixmapChannel::BlupiYoupieBackground, 0, rect, opacity, 0.0, false);
            }
            else if (fadeOutPhase == Def::Phase::PlaySetup)
            {
                double num = std::min((double)phaseTime / 20.0, 1.0);
                num *= num;
                TinyRect tinyRect5 = TinyRect();
                tinyRect5.Left = (int)(213.0 + 800.0 * num);
                tinyRect5.Right = (int)(623.0 + 800.0 * num);
                tinyRect5.Top = 0;
                tinyRect5.Bottom = 0;
                TinyRect rect = tinyRect5;
                pixmap->DrawIcon(PixmapChannel::BlupiYoupieBackground, 0, rect, 1.0, 0.0, false);
            }
            else
            {
                double num;
                if (fadeOutPhase == Def::Phase::None)
                {
                    num = std::min((double)phaseTime / 15.0, 1.0);
                }
                else
                {
                    num = std::min((double)phaseTime / 15.0, 1.0);
                    num = 1.0 - num;
                }
                TinyRect tinyRect6 = TinyRect();
                tinyRect6.Left = (int)(418.0 - 205.0 * num);
                tinyRect6.Right = (int)(418.0 + 205.0 * num);
                tinyRect6.Top = (int)(190.0 - 190.0 * num);
                tinyRect6.Bottom = (int)(190.0 + 190.0 * num);
                TinyRect rect = tinyRect6;
                double rotation = 0.0;
                if (num < 1.0)
                {
                    rotation = (1.0 - num) * (1.0 - num) * 360.0 * 1.0;
                }
                if (rect.getWidthProperty() > 0 && rect.getHeightProperty() > 0)
                {
                    pixmap->DrawIcon(PixmapChannel::BlupiYoupieBackground, 0, rect, 1.0, rotation, false);
                }
            }
        }
        if (phase == Def::Phase::MainSetup || phase == Def::Phase::PlaySetup)
        {
            double num = std::min((double)phaseTime / 20.0, 1.0);
            num = 1.0 - (1.0 - num) * (1.0 - num);
            double num2;
            if (phaseTime < 20)
            {
                num2 = (double)phaseTime / 20.0;
                num2 = 1.0 - (1.0 - num2) * (1.0 - num2);
            }
            else
            {
                num2 = 1.0 + ((double)phaseTime - 20.0) / 400.0;
            }
            if (fadeOutPhase != Def::Phase::None)
            {
                num = 1.0 - num;
                num2 = 1.0 - num2;
            }
            TinyRect tinyRect7 = TinyRect();
            tinyRect7.Left = (int)(720.0 - 640.0 * num);
            tinyRect7.Right = (int)(1360.0 - 640.0 * num);
            tinyRect7.Top = 0;
            tinyRect7.Bottom = 160;
            TinyRect rect = tinyRect7;
            pixmap->DrawIcon(PixmapChannel::SpeedyBlupiBackground, 0, rect, num * num, false);
            TinyRect tinyRect8 = TinyRect();
            tinyRect8.Left = 487;
            tinyRect8.Right = 713;
            tinyRect8.Top = 148;
            tinyRect8.Bottom = 374;
            TinyRect rect2 = tinyRect8;
            TinyRect tinyRect9 = TinyRect();
            tinyRect9.Left = 118;
            tinyRect9.Right = 570;
            tinyRect9.Top = 268;
            tinyRect9.Bottom = 720;
            TinyRect rect3 = tinyRect9;
            double opacity = 0.5 - num * 0.4;
            double rotation = (0.0 - num2) * 100.0 * 2.5;
            pixmap->DrawIcon(PixmapChannel::GearBackground, 0, rect2, opacity, rotation, false);
            pixmap->DrawIcon(PixmapChannel::GearBackground, 0, rect3, opacity, (0.0 - rotation) * 0.5, false);
        }
        if (phase == Def::Phase::Lost)
        {
            double num = std::min((double)phaseTime / 100.0, 1.0);
            TinyRect tinyRect10 = TinyRect();
            tinyRect10.Left = (int)(418.0 - 205.0 * num);
            tinyRect10.Right = (int)(418.0 + 205.0 * num);
            tinyRect10.Top = (int)(238.0 - 190.0 * num);
            tinyRect10.Bottom = (int)(238.0 + 190.0 * num);
            TinyRect rect = tinyRect10;
            double rotation = 0.0;
            if (num < 1.0)
            {
                rotation = (1.0 - num) * (1.0 - num) * 360.0 * 6.0;
            }
            if (rect.getWidthProperty() > 0 && rect.getHeightProperty() > 0)
            {
                pixmap->DrawIcon(PixmapChannel::BlupiYoupieBackground, 0, rect, 1.0, rotation, false);
            }
        }
        if (phase == Def::Phase::Win)
        {
            double num = std::sin(static_cast<double>(phaseTime) / 3.0) / 2.0 + 1.0;
            TinyRect tinyRect11 = TinyRect();
            tinyRect11.Left = (int)(418.0 - 205.0 * num);
            tinyRect11.Right = (int)(418.0 + 205.0 * num);
            tinyRect11.Top = (int)(238.0 - 190.0 * num);
            tinyRect11.Bottom = (int)(238.0 + 190.0 * num);
            TinyRect rect = tinyRect11;
            pixmap->DrawIcon(PixmapChannel::BlupiYoupieBackground, 0, rect, 1.0, 0.0, false);
        }
    }

    void Game1::DrawButtonsBackground()
    {
        if (phase == Def::Phase::Init)
        {
            TinyRect drawBounds = pixmap->getDrawBoundsProperty();
            int width = drawBounds.getWidthProperty();
            int height = drawBounds.getHeightProperty();
            TinyRect tinyRect = TinyRect();
            tinyRect.Left = 10;
            tinyRect.Right = 260;
            tinyRect.Top = height - 325;
            tinyRect.Bottom = height - 10;
            TinyRect rect = tinyRect;
            pixmap->DrawIcon(PixmapChannel::Pad, 15, rect, 0.3, false);
            TinyRect tinyRect2 = TinyRect();
            tinyRect2.Left = width - 170;
            tinyRect2.Right = width - 10;
            tinyRect2.Top = height - ((getIsTrialModeProperty() || getIsRankingModeProperty()) ? 325 : 195);
            tinyRect2.Bottom = height - 10;
            rect = tinyRect2;
            pixmap->DrawIcon(PixmapChannel::Pad, 15, rect, 0.3, false);
        }
    }

    void Game1::DrawButtonsText()
    {
        if (phase == Def::Phase::Init)
        {
            DrawButtonGamerText(Def::ButtonGlyph::InitGamerA, 0);
            DrawButtonGamerText(Def::ButtonGlyph::InitGamerB, 1);
            DrawButtonGamerText(Def::ButtonGlyph::InitGamerC, 2);
            DrawTextUnderButton(Def::ButtonGlyph::InitPlay, MyResource::TX_BUTTON_PLAY);
            DrawTextRightButton(Def::ButtonGlyph::InitSetup, MyResource::TX_BUTTON_SETUP);
            if (getIsTrialModeProperty())
            {
                DrawTextUnderButton(Def::ButtonGlyph::InitBuy, MyResource::TX_BUTTON_BUY);
            }
            if (getIsRankingModeProperty())
            {
                DrawTextUnderButton(Def::ButtonGlyph::InitRanking, MyResource::TX_BUTTON_RANKING);
            }
        }
        if (phase == Def::Phase::Pause)
        {
            DrawTextUnderButton(Def::ButtonGlyph::PauseMenu, MyResource::TX_BUTTON_MENU);
            if (mission != 1)
            {
                DrawTextUnderButton(Def::ButtonGlyph::PauseBack, MyResource::TX_BUTTON_BACK);
            }
            DrawTextUnderButton(Def::ButtonGlyph::PauseSetup, MyResource::TX_BUTTON_SETUP);
            if (mission != 1 && mission % 10 != 0)
            {
                DrawTextUnderButton(Def::ButtonGlyph::PauseRestart, MyResource::TX_BUTTON_RESTART);
            }
            DrawTextUnderButton(Def::ButtonGlyph::PauseContinue, MyResource::TX_BUTTON_CONTINUE);
        }
        if (phase == Def::Phase::Resume)
        {
            DrawTextUnderButton(Def::ButtonGlyph::ResumeMenu, MyResource::TX_BUTTON_MENU);
            DrawTextUnderButton(Def::ButtonGlyph::ResumeContinue, MyResource::TX_BUTTON_CONTINUE);
        }
        if (phase == Def::Phase::MainSetup || phase == Def::Phase::PlaySetup)
        {
            DrawTextRightButton(Def::ButtonGlyph::SetupSounds, MyResource::TX_BUTTON_SETUP_SOUNDS);
            DrawTextRightButton(Def::ButtonGlyph::SetupJump, MyResource::TX_BUTTON_SETUP_JUMP);
            DrawTextRightButton(Def::ButtonGlyph::SetupZoom, MyResource::TX_BUTTON_SETUP_ZOOM);
            DrawTextRightButton(Def::ButtonGlyph::SetupAccel, MyResource::TX_BUTTON_SETUP_ACCEL);
            if (phase == Def::Phase::MainSetup)
            {
                string text = Helper::formatString(
                    MyResource::LoadString(MyResource::TX_BUTTON_SETUP_RESET),
                    STRING_VECTOR(
                        std::string(1, static_cast<char>('A' + gameData.getSelectedGamerProperty()))
                    )
                );
                DrawTextRightButton(Def::ButtonGlyph::SetupReset, text);
            }
        }
        if (phase == Def::Phase::Trial)
        {
            TinyPoint tinyPoint{};
            tinyPoint.X = 360;
            tinyPoint.Y = 50;
            TinyPoint pos = tinyPoint;
            Text::DrawText(*pixmap.get(), pos, MyResource::LoadString(MyResource::TX_TRIAL1), 0.9);
            pos.Y += 40;
            Text::DrawText(*pixmap.get(), pos, MyResource::LoadString(MyResource::TX_TRIAL2), 0.7);
            pos.Y += 25;
            Text::DrawText(*pixmap.get(), pos, MyResource::LoadString(MyResource::TX_TRIAL3), 0.7);
            pos.Y += 25;
            Text::DrawText(*pixmap.get(), pos, MyResource::LoadString(MyResource::TX_TRIAL4), 0.7);
            pos.Y += 25;
            Text::DrawText(*pixmap.get(), pos, MyResource::LoadString(MyResource::TX_TRIAL5), 0.7);
            pos.Y += 25;
            Text::DrawText(*pixmap.get(), pos, MyResource::LoadString(MyResource::TX_TRIAL6), 0.7);
            DrawTextUnderButton(Def::ButtonGlyph::TrialBuy, MyResource::TX_BUTTON_BUY);
            DrawTextUnderButton(Def::ButtonGlyph::TrialCancel, MyResource::TX_BUTTON_BACK);
        }
        if (phase == Def::Phase::Ranking)
        {
            DrawTextUnderButton(Def::ButtonGlyph::RankingContinue, MyResource::TX_BUTTON_BACK);
        }
    }

    void Game1::DrawButtonGamerText(Def::ButtonGlyph glyph, int gamer)
    {
        TinyRect buttonRect = inputPad.GetButtonRect(glyph);
        int nbVies;
        int mainDoors;
        int secondaryDoors;
        gameData.GetGamerInfo(gamer, nbVies, mainDoors, secondaryDoors);
        TinyPoint tinyPoint;
        tinyPoint.X = buttonRect.Right + 5 - pixmap->getOriginProperty().X;
        tinyPoint.Y = buttonRect.Top + 3 - pixmap->getOriginProperty().Y;
        TinyPoint pos = tinyPoint;
        string text = Helper::formatString(
            MyResource::LoadString(MyResource::TX_GAMER_TITLE),
            STRING_VECTOR(std::string(1, static_cast<char>('A' + gamer)))
        );
        Text::DrawText(*pixmap.get(), pos, text, 0.7);
        TinyPoint tinyPoint2;
        tinyPoint2.X = buttonRect.Right + 5 - pixmap->getOriginProperty().X;
        tinyPoint2.Y = buttonRect.Top + 25 - pixmap->getOriginProperty().Y;
        pos = tinyPoint2;
        text = Helper::formatString(MyResource::LoadString(MyResource::TX_GAMER_MDOORS),
                                    STRING_VECTOR(std::to_string(mainDoors)));
        Text::DrawText(*pixmap.get(), pos, text, 0.45);
        TinyPoint tinyPoint3;
        tinyPoint3.X = buttonRect.Right + 5 - pixmap->getOriginProperty().X;
        tinyPoint3.Y = buttonRect.Top + 39 - pixmap->getOriginProperty().Y;
        pos = tinyPoint3;
        text = Helper::formatString(MyResource::LoadString(MyResource::TX_GAMER_SDOORS),
                                    STRING_VECTOR(std::to_string(secondaryDoors)));
        Text::DrawText(*pixmap.get(), pos, text, 0.45);
        TinyPoint tinyPoint4;
        tinyPoint4.X = buttonRect.Right + 5 - pixmap->getOriginProperty().X;
        tinyPoint4.Y = buttonRect.Top + 53 - pixmap->getOriginProperty().Y;
        pos = tinyPoint4;
        text = Helper::formatString(MyResource::LoadString(MyResource::TX_GAMER_LIFES),
                                    STRING_VECTOR(std::to_string(nbVies)));
        Text::DrawText(*pixmap.get(), pos, text, 0.45);
    }

    void Game1::DrawTextRightButton(Def::ButtonGlyph glyph, int res)
    {
        DrawTextRightButton(glyph, MyResource::LoadString(res));
    }

    void Game1::DrawTextRightButton(Def::ButtonGlyph glyph, string text)
    {
        TinyRect buttonRect = inputPad.GetButtonRect(glyph);
        std::vector<std::string> array = System::String::Split(text, '\n');
        if (array.size() == 2)
        {
            TinyPoint tinyPoint;
            tinyPoint.X = buttonRect.Right + 10 - pixmap->getOriginProperty().X;
            tinyPoint.Y = (buttonRect.Top + buttonRect.Bottom) / 2 - 20 - pixmap->getOriginProperty().Y;
            TinyPoint pos = tinyPoint;
            Text::DrawText(*pixmap.get(), pos, array[0], 0.7);
            pos.Y += 24;
            Text::DrawText(*pixmap.get(), pos, array[1], 0.7);
        }
        else
        {
            TinyPoint tinyPoint2;
            tinyPoint2.X = buttonRect.Right + 10 - pixmap->getOriginProperty().X;
            tinyPoint2.Y = (buttonRect.Top + buttonRect.Bottom) / 2 - 8 - pixmap->getOriginProperty().Y;
            TinyPoint pos2 = tinyPoint2;
            Text::DrawText(*pixmap.get(), pos2, text, 0.7);
        }
    }

    void Game1::DrawTextUnderButton(Def::ButtonGlyph glyph, int res)
    {
        TinyRect buttonRect = inputPad.GetButtonRect(glyph);
        TinyPoint tinyPoint;
        tinyPoint.X = (buttonRect.Left + buttonRect.Right) / 2 - pixmap->getOriginProperty().X;
        tinyPoint.Y = buttonRect.Bottom + 2 - pixmap->getOriginProperty().Y;
        TinyPoint pos = tinyPoint;
        string text = MyResource::LoadString(res);
        Text::DrawTextCenter(*pixmap.get(), pos, text, 0.7);
    }

    void Game1::DrawWaitProgress()
    {
        if (continueMission != 0)
        {
            return;
        }
        for (int i = 0; i < waitTableLength; i++)
        {
            if (waitProgress <= waitTable[i * 2])
            {
                const int level = (int)waitTable[i * 2 + 1];

                CNA::Logger::Debug(
                    "DrawWaitProgress: waitProgress=" + std::to_string(waitProgress) +
                    ", i=" + std::to_string(i) +
                    ", threshold=" + std::to_string(waitTable[i * 2]) +
                    ", level=" + std::to_string(level));
                waitJauge.SetLevel(level);
                break;
            }
        }
        waitJauge.Draw();
    }

    void Game1::DrawDebug()
    {
        TinyPoint tinyPoint;
        tinyPoint.X = 10;
        tinyPoint.Y = 20;
        TinyPoint pos = tinyPoint;
        Text::DrawText(*pixmap.get(), pos, TO_STRING(inputPad.getTotalTouchOrClickProperty()), 1.0);
    }

    void Game1::SetGamer(int gamer)
    {
        gameData.setSelectedGamerProperty(gamer);
        gameData.Write();
    }

    void Game1::SetPhase(Def::Phase phase)
    {
        SetPhase(phase, 0);
    }

    void Game1::SetPhase(Def::Phase phase, int mission)
    {
        if (mission != -2)
        {
            if (missionToStart2 == -1)
            {
                if ((this->phase == Def::Phase::Init || this->phase == Def::Phase::MainSetup || this->phase ==
                        Def::Phase::PlaySetup || this->phase == Def::Phase::Pause || this->phase == Def::Phase::Resume)
                    &&
                    fadeOutPhase == Def::Phase::None)
                {
                    fadeOutPhase = phase;
                    fadeOutMission = mission;
                    phaseTime = 0;
                    return;
                }
                if (phase == Def::Phase::Play)
                {
                    fadeOutPhase = Def::Phase::None;
                    if (fadeOutMission != -1)
                    {
                        missionToStart1 = fadeOutMission;
                        return;
                    }
                    mission = fadeOutMission;
                    decor.LoadImages();
                }
            }
            else
            {
                mission = missionToStart2;
            }
        }
        this->phase = phase;
        fadeOutPhase = Def::Phase::None;
        inputPad.setPhaseProperty(this->phase);
        playSetup = this->phase == Def::Phase::PlaySetup;
        isTrialMode = Microsoft::Xna::Framework::GamerServices::Guide::getIsTrialModeProperty();
        phaseTime = 0;
        missionToStart2 = -1;
        decor.StopSound();
        switch (this->phase)
        {
        case Def::Phase::Init:
            pixmap->BackgroundCache("init");
            break;
        case Def::Phase::Pause:
        case Def::Phase::Resume:
            pixmap->BackgroundCache("pause");
            break;
        case Def::Phase::Lost:
            pixmap->BackgroundCache("lost");
            break;
        case Def::Phase::Win:
            pixmap->BackgroundCache("win");
            break;
        case Def::Phase::MainSetup:
        case Def::Phase::PlaySetup:
            pixmap->BackgroundCache("setup");
            break;
        case Def::Phase::Trial:
            pixmap->BackgroundCache("trial");
            break;
        case Def::Phase::Ranking:
            pixmap->BackgroundCache("pause");
            break;
        case Def::Phase::Play:
            decor.setDrawBoundsProperty(pixmap->getDrawBoundsProperty());
            break;
        }
        if (this->phase == Def::Phase::Play && mission > 0)
        {
            StartMission(mission);
        }
    }

    void Game1::MemorizeGamerProgress()
    {
        CNA::Logger::Debug("Game1::MemorizeGamerProgress begin mission=" + std::to_string(mission)
            + " nbVies=" + std::to_string(decor.GetNbVies()));
        gameData.setNbViesProperty(decor.GetNbVies());
        decor.MemorizeDoors(gameData);
        gameData.Write();
        CNA::Logger::Debug("Game1::MemorizeGamerProgress end mission=" + std::to_string(mission));
    }

    void Game1::ToggleFullScreen()
    {
        this->graphics.ToggleFullScreen();
    }

    bool Game1::IsFullScreen() { return this->graphics.getIsFullScreenProperty(); }

    Microsoft::Xna::Framework::Graphics::GraphicsDeviceManager Game1::getGraphics()
    {
        return graphics;
    }

    Microsoft::Xna::Framework::Content::ContentManager& Game1::getContentProperty()
    {
        return Game::getContentProperty();
    }

    Microsoft::Xna::Framework::Graphics::GraphicsDevice& Game1::getGraphicsDeviceProperty()
    {
        return Game::getGraphicsDeviceProperty();
    }

    GetTypeNameCPP(Game1, "GalaxyEggbert::Game1")
}
