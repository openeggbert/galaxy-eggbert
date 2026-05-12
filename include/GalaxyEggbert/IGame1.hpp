#pragma once

#include <any>

#include "Def.hpp"
#include "InputPad.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDeviceManager.hpp"
#include "System/EventArgs.hpp"

#define readonly mutable

namespace GalaxyEggbert
{
    class IGame1
    {
    protected:
        ~IGame1() = default;

    public:
        [[nodiscard]] virtual bool getIsRankingModeProperty() const = 0;

    public:
        [[nodiscard]] virtual bool getIsTrialModeProperty() const = 0;

    protected:
        virtual void Initialize() = 0;

        virtual void LoadContent() = 0;

        virtual void UnloadContent() = 0;

        virtual void OnDeactivated(std::any sender, System::EventArgs args) = 0;

        virtual void OnActivated(std::any sender, System::EventArgs args) = 0;

        virtual void OnExiting(Microsoft::Xna::Framework::ExitingEventArgs args) = 0;

        virtual void Update(Microsoft::Xna::Framework::GameTime& gameTime) = 0;

    private:
        virtual void MissionBack() = 0;

        virtual void StartMission(int mission) = 0;

        virtual void ContinueMission() = 0;

        virtual void CheatAction(Def::ButtonGlyph glyph) = 0;

    protected:
        virtual void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) = 0;

    private:
        virtual void DrawBackgroundFade() = 0;

        virtual void DrawButtonsBackground() = 0;

        virtual void DrawButtonsText() = 0;

        virtual void DrawButtonGamerText(Def::ButtonGlyph glyph, int gamer) = 0;

        virtual void DrawTextRightButton(Def::ButtonGlyph glyph, int res) = 0;

        virtual void DrawTextRightButton(Def::ButtonGlyph glyph, std::string text) = 0;

        virtual void DrawTextUnderButton(Def::ButtonGlyph glyph, int res) = 0;

        virtual void DrawWaitProgress() = 0;

        virtual void DrawDebug() = 0;

        virtual void SetGamer(int gamer) = 0;

        virtual void SetPhase(Def::Phase phase) = 0;

        virtual void SetPhase(Def::Phase phase, int mission) = 0;

        virtual void MemorizeGamerProgress() = 0;

    public:
        virtual void ToggleFullScreen() = 0;

        virtual bool IsFullScreen() = 0;

        virtual Microsoft::Xna::Framework::Graphics::GraphicsDeviceManager getGraphics() = 0;
        ////

        [[nodiscard]] virtual Microsoft::Xna::Framework::Content::ContentManager& getContentProperty() = 0;

        [[nodiscard]] virtual Microsoft::Xna::Framework::Graphics::GraphicsDevice& getGraphicsDeviceProperty() = 0;
    };
}
