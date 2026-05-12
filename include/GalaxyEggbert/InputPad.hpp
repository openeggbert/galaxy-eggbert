#pragma once

#include "Microsoft/Devices/Sensors/Accelerometer.hpp"
#include "Microsoft/Devices/Sensors/AccelerometerReading.hpp"
#include "GalaxyEggbert/Decor.hpp"
#include "GalaxyEggbert/IPixmap.hpp"
#include "GalaxyEggbert/Slider.hpp"
#include "GalaxyEggbert/ISound.hpp"

#define VECTOR_CONTAINS(vector, element) count( vector .begin(), vector. end(), element );

namespace GalaxyEggbert
{
    class IGame1;

    class InputPad
    {
        static constexpr int padRadius = 140;

        mutable IGame1* game1;

        mutable Decor* decor;

        mutable IPixmap* pixmap;

        mutable ISound* sound;

        mutable GameData* gameData;

        mutable std::vector<Def::ButtonGlyph> pressedGlyphs;

        mutable Microsoft::Devices::Sensors::Accelerometer accelSensor;

        mutable Slider accelSlider;

        bool padPressed = false;

        bool showCheatMenu = false;

        TinyPoint padTouchPos;

        Def::ButtonGlyph lastButtonDown;

        mutable Def::ButtonGlyph buttonPressed;

        int touchOrClickCount = 0;

        bool accelStarted = false;;

        bool accelActive = false;

        double accelSpeedX = 0.0f;

        bool accelLastState = false;

        bool accelWaitZero = false;

        int mission = 0;

    public:
        DDATA(Def::Phase, Phase)
        DDATA(int, SelectedGamer)
        DDATA(TinyPoint, PixmapOrigin)

        [[nodiscard]] int getTotalTouchOrClickProperty() const;

        [[nodiscard]] Def::ButtonGlyph getButtonPressedProperty() const;
        DDATA(bool, ShowCheatMenu)

        [[nodiscard]] std::vector<Def::ButtonGlyph> getButtonGlyphsProperty() const;
        // Returns the point of the center of the pad on the screen.
        [[nodiscard]] TinyPoint getPadCenterProperty() const;

        InputPad(IGame1* game1, Decor* decor, IPixmap* pixmap, ISound* sound, GameData* gameData);

        void StartMission(int mission);

        void Update();

    private:
        Def::ButtonGlyph ButtonDetect(TinyPoint touchOrClick);

    public:
        void Draw();

    private:
        TinyRect GetPadBounds(TinyPoint center, int radius);

    public:
        TinyRect GetButtonRect(Def::ButtonGlyph glyph);

    private:
        void StartAccel();

        void StopAccel();

        void HandleAccelSensorCurrentValueChanged(
            Microsoft::Devices::Sensors::SensorReadingEventArgs<Microsoft::Devices::Sensors::AccelerometerReading> e);
    };
}
