#pragma once

#include <any>

#include "Decor.hpp"
#include "Def.hpp"
#include "IGame1.hpp"
#include "Jauge.hpp"
#include "IPixmap.hpp"
#include "ISound.hpp"
#include "InputPad.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDeviceManager.hpp"
#include "System/TimeSpan.hpp"
#include "System/EventArgs.hpp"

#define readonly mutable

namespace GalaxyEggbert
{
    class Game1 : public Microsoft::Xna::Framework::Game, public IGame1
    {
        static constexpr int waitTableLength = 24;
        //pairs: threshold, level
        static constexpr double waitTable[waitTableLength] =
        {
            0.1, 7.0, 0.2, 20.0, 0.25, 22.0, 0.45, 50.0, 0.6, 53.0,
            0.65, 58.0, 0.68, 60.0, 0.8, 70.0, 0.84, 75.0, 0.9, 84.0,
            0.94, 91.0, 1.0, 100.0
        };

        static constexpr int cheatGesteLength = 10;

        static constexpr Def::ButtonGlyph cheatGeste[cheatGesteLength] =
        {
            Def::ButtonGlyph::Cheat12,
            Def::ButtonGlyph::Cheat22,
            Def::ButtonGlyph::Cheat32,
            Def::ButtonGlyph::Cheat12,
            Def::ButtonGlyph::Cheat11,
            Def::ButtonGlyph::Cheat21,
            Def::ButtonGlyph::Cheat22,
            Def::ButtonGlyph::Cheat21,
            Def::ButtonGlyph::Cheat31,
            Def::ButtonGlyph::Cheat32
        };

        readonly Microsoft::Xna::Framework::Graphics::GraphicsDeviceManager graphics;

        readonly std::shared_ptr<IPixmap> pixmap;

        readonly std::shared_ptr<ISound> sound;

        readonly Decor decor;

        readonly InputPad inputPad;

        readonly GameData gameData;

        Def::Phase phase;

        System::TimeSpan startTime;

        int missionToStart1;

        int missionToStart2;

        int mission = 0;

        int cheatGesteIndex = 0;

        int continueMission = 0;

        Jauge waitJauge;

        double waitProgress = 0.0;

        bool isTrialMode = false;;

        bool simulateTrialMode{false};

        bool playSetup = false;

        int phaseTime{0};

        Def::Phase fadeOutPhase;

        int fadeOutMission{0};

    public:
        [[nodiscard]] bool getIsRankingModeProperty() const override;

        [[nodiscard]] bool getIsTrialModeProperty() const override;

        Game1();
        virtual ~Game1();

    protected:
        void Initialize() override;

        void LoadContent() override;

        void UnloadContent() override;

        void OnDeactivated(std::any sender, System::EventArgs args) override;

        void OnActivated(std::any sender, System::EventArgs args) override;

        void OnExiting(Microsoft::Xna::Framework::ExitingEventArgs args) override;

        void Update(Microsoft::Xna::Framework::GameTime& gameTime) override;

    private:
        void MissionBack() override;

        void StartMission(int mission) override;

        void ContinueMission() override;

        void CheatAction(Def::ButtonGlyph glyph) override;

    protected:
        void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override;

    private:
        void DrawBackgroundFade() override;

        void DrawButtonsBackground() override;

        void DrawButtonsText() override;

        void DrawButtonGamerText(Def::ButtonGlyph glyph, int gamer) override;

        void DrawTextRightButton(Def::ButtonGlyph glyph, int res) override;

        void DrawTextRightButton(Def::ButtonGlyph glyph, std::string text) override;

        void DrawTextUnderButton(Def::ButtonGlyph glyph, int res) override;

        void DrawWaitProgress() override;

        void DrawDebug() override;

        void SetGamer(int gamer) override;

        void SetPhase(Def::Phase phase) override;

        void SetPhase(Def::Phase phase, int mission) override;

        void MemorizeGamerProgress() override;

    public:
        void ToggleFullScreen() override;
        bool IsFullScreen() override;

        Microsoft::Xna::Framework::Graphics::GraphicsDeviceManager getGraphics() override;

        [[nodiscard]] Microsoft::Xna::Framework::Content::ContentManager& getContentProperty() override;

        [[nodiscard]] Microsoft::Xna::Framework::Graphics::GraphicsDevice& getGraphicsDeviceProperty() override;
        GetTypeNameHPP()
    };
};
