#pragma once

#include "IPixmap.hpp"
#include "ISound.hpp"

namespace GalaxyEggbert
{
    enum class JaugeMode : intcs
    {
        Empty = 0,
        Red = 1, // danger
        Blue = 2, // water
        Yellow = 3 // charge
    };

    inline intcs ToRaw(JaugeMode mode)
    {
        return static_cast<intcs>(mode);
    }

    inline JaugeMode ToJaugeMode(intcs mode)
    {
        return static_cast<JaugeMode>(mode);
    }

    /**
 * @brief Draws and manages a small HUD gauge/progress bar.
 *
 * Jauge is used for a 124x22 pixel gauge sprite. It draws the empty gauge
 * background and, depending on the current level, draws the filled part from
 * the selected gauge mode row.
 *
 * The gauge level is clamped to the range 0..100. The filled part has a maximum
 * width of 114 pixels and starts after a 6 pixel left border.
 *
 * @note Status: IMPLEMENTED
 */
    class Jauge
    {
        IPixmap* m_pixmap;

        ISound* m_sound;

        bool m_bHide;

        TinyPoint m_pos;

        TinyPoint m_dim;

        JaugeMode m_mode;

        int m_level;

        bool m_bMinimizeRedraw;

        bool m_bRedraw;

        double m_zoom;

    public:
        /**
 * @brief Gets the drawing zoom used for this gauge.
 *
 * @return Current gauge zoom factor.
 *
 * @note Status: IMPLEMENTED
 */
        [[nodiscard]] double getZoomProperty() const;
        /**
         * @brief Sets the drawing zoom used for this gauge.
         *
         * @param v New gauge zoom factor.
         *
         * @note Status: IMPLEMENTED
         */
        void setZoomProperty(double v);

        Jauge();
        /**
         * @brief Creates a gauge with its pixmap, sound object, position, mode, and redraw behavior.
         *
         * Stores the rendering and sound interfaces, sets the gauge position and mode,
         * initializes its fixed sprite dimensions to 124x22 pixels, hides the gauge,
         * resets the level to 0, and marks it for redraw.
         *
         * @param pixmap Pixmap renderer used to draw the gauge.
         * @param sound Sound interface kept for compatibility with the original game. Currently unused.
         * @param pos Top-left position of the gauge.
         * @param mode Gauge sprite row/mode to use for the filled part.
         * @param bMinimizeRedraw If true, drawing is skipped unless the gauge was marked dirty.
         * @return Always true.
         *
         * @note Status: IMPLEMENTED
         */
        bool Create(IPixmap* pixmap, ISound* sound, TinyPoint pos, JaugeMode mode, bool bMinimizeRedraw);
        /**
         * @brief Draws the gauge if it is visible and needs redraw.
         *
         * Draws the 124x22 empty gauge background from channel 5. If the current level
         * is greater than zero, it also draws the filled part using the selected mode
         * row. When minimized redraw is enabled, drawing is skipped unless the gauge
         * was marked for redraw.
         *
         * @note Status: IMPLEMENTED
         */
        void Draw();
        /**
         * @brief Marks the gauge as needing redraw.
         *
         * @note Status: IMPLEMENTED
         */
        void Redraw();
        /**
         * @brief Gets the current gauge level.
         *
         * @return Level in the range 0..100.
         *
         * @note Status: IMPLEMENTED
         */
        int GetLevel();
        /**
         * @brief Sets the current gauge level.
         *
         * The value is clamped to the range 0..100. If the value changes, the gauge is
         * marked for redraw.
         *
         * @param level New gauge level.
         *
         * @note Status: IMPLEMENTED
         */
        void SetLevel(int level);
        /**
         * @brief Gets the current gauge mode.
         *
         * The mode selects which 22 pixel high row of the gauge texture is used for the
         * filled portion.
         *
         * @return Current gauge mode.
         *
         * @note Status: IMPLEMENTED
         */
        JaugeMode GetMode();

        /**
         * @brief Sets the current gauge mode.
         *
         * If the mode changes, the gauge is marked for redraw.
         *
         * @param mode New gauge mode.
         *
         * @note Status: IMPLEMENTED
         */
        void SetMode(JaugeMode mode);
        /**
         * @brief Sets the current gauge mode.
         *
         * If the mode changes, the gauge is marked for redraw.
         *
         * @param mode New gauge mode.
         *
         * @note Status: IMPLEMENTED
         */
        void SetMode(int mode)
        {
            SetMode(ToJaugeMode(mode));
        }

        /**
         * @brief Returns whether the gauge is hidden.
         *
         * @return True if the gauge is hidden; false if it should be drawn.
         *
         * @note Status: IMPLEMENTED
         */
        bool GetHide();
        /**
         * @brief Shows or hides the gauge.
         *
         * If the visibility changes, the gauge is marked for redraw.
         *
         * @param bHide True to hide the gauge; false to show it.
         *
         * @note Status: IMPLEMENTED
         */
        void SetHide(bool bHide);
        /**
         * @brief Gets the gauge position.
         *
         * @return Top-left gauge position.
         *
         * @note Status: IMPLEMENTED
         */
        [[nodiscard]] TinyPoint GetPos() const;
        /**
         * @brief Marks the gauge as needing redraw.
         *
         * This is equivalent to Redraw().
         *
         * @note Status: IMPLEMENTED
         */
        void SetRedraw();
    };
}
