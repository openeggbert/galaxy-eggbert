#pragma once

#include <optional>

#include "Def.hpp"
#include "IGame1.hpp"
#include "IPixmap.hpp"

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "GalaxyEggbert/TinyPoint.hpp"
#include "GalaxyEggbert/TinyRect.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"

namespace GalaxyEggbert
{
    using Microsoft::Xna::Framework::Graphics::Texture2D;

    class Pixmap : public IPixmap
    {
        IGame1* game1;

        Microsoft::Xna::Framework::Graphics::GraphicsDeviceManager& graphics;

        double zoom = 0.0;

        double originX = 0.0;

        double originY = 0.0;

        double hotSpotZoom = 1.0;

        double hotSpotX = 0.0;

        double hotSpotY = 0.0;

        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch;

        Texture2D bitmapText;

        Texture2D bitmapButton;

        Texture2D bitmapJauge;

        Texture2D bitmapBlupi;

        Texture2D bitmapBlupi1;

        Texture2D bitmapObject;

        Texture2D bitmapElement;

        Texture2D bitmapExplo;

        Texture2D bitmapPad;

        Texture2D bitmapSpeedyBlupi;

        Texture2D bitmapBlupiYoupie;

        Texture2D bitmapGear;

        Texture2D bitmapBackground;

        Microsoft::Xna::Framework::Vector2 origin;

        Microsoft::Xna::Framework::Graphics::SpriteEffects effect;

    public:
        [[nodiscard]] TinyRect getDrawBoundsProperty() override;
        [[nodiscard]] TinyPoint getOriginProperty() const override;

        Pixmap(IGame1* game1, Microsoft::Xna::Framework::Graphics::GraphicsDeviceManager& graphics);

        Pixmap(const Pixmap&) = delete;
        Pixmap& operator=(const Pixmap&) = delete;

        ~Pixmap() override = default;

        /**
 * @brief Converts a point from hotspot-scaled coordinates to HUD coordinates.
 *
 * This method transforms the given point by reversing the current hotspot zoom
 * around the hotspot position, then subtracting the pixmap origin. The result is
 * a HUD-space point suitable for drawing or positioning UI elements relative to
 * the current viewport.
 *
 * @param pos The point in hotspot-scaled coordinates.
 * @return The corresponding point in HUD coordinates.
 *
 * @note Status: IMPLEMENTED
 */
        TinyPoint HotSpotToHud(TinyPoint pos) override;

        /**
 * @brief Sets the hotspot transform used by this pixmap.
 *
 * Stores the zoom factor and hotspot position used when converting between
 * hotspot-scaled coordinates and HUD or pixmap coordinates.
 *
 * @param zoom The hotspot zoom factor.
 * @param x The X coordinate of the hotspot center.
 * @param y The Y coordinate of the hotspot center.
 *
 * @note Status: IMPLEMENTED
 */
        void SetHotSpot(double zoom, double x, double y) override;

        /**
 * @brief Draws an on-screen input button for the given button glyph.
 *
 * Selects the correct icon from the input button sprite sheet and draws it
 * inside the given rectangle. The visual scale depends on whether the button
 * is currently pressed. Some menu/setup buttons also use the selected state to
 * choose an alternate icon.
 *
 * Cheat buttons draw a generic icon and then render the cheat label text in
 * the center of the button.
 *
 * Some multi-part cheat glyphs are intentionally ignored because they do not
 * draw anything in this method.
 *
 * @param rect Destination rectangle for the button.
 * @param glyph Button glyph describing which icon or label should be drawn.
 * @param pressed Whether the button is currently pressed.
 * @param selected Whether the button is currently selected.
 *
 * @note Status: IMPLEMENTED
 */
        void DrawInputButton(TinyRect rect, Def::ButtonGlyph glyph, bool pressed, bool selected) override;

        void LoadContent() override;

    private:
        /**
 * @brief Updates the pixmap scaling and origin for the current viewport.
 *
 * Computes the zoom factor needed to fit the base 640x480 game area into the
 * current viewport while preserving the original aspect ratio. The remaining
 * horizontal or vertical space is stored as the drawing origin, effectively
 * centering the game area inside the viewport.
 *
 * On Android, tall viewports are normalized to a 4:3 virtual width when the
 * viewport height is greater than 480 pixels.
 *
 * @note Status: IMPLEMENTED
 */
        void UpdateGeometry();

    public:
        void BackgroundCache(const std::string& name) override;

        bool Start() override;

        bool Finish() override;

        /**
 * @brief Draws the full background for the current viewport.
 *
 * First fills the entire viewport with the background texture stretched to the
 * current screen size. Then draws the base 640x480 game background area at the
 * current pixmap origin, so the main game view remains centered according to
 * the current geometry settings.
 *
 * On Android, tall viewports are normalized to a 4:3 virtual width when the
 * viewport height is greater than 480 pixels.
 *
 * @note Status: IMPLEMENTED
 */
        void DrawBackground() override;
        /**
         * @brief Draws one character glyph at the given game-space position.
         *
         * Converts the position to screen-space by applying the current pixmap origin,
         * builds a square destination rectangle scaled from the base 32x32 glyph size,
         * and draws the glyph from icon sheet 6 using the given glyph rank.
         *
         * @param rank Index of the character glyph inside the font/icon sheet.
         * @param pos Top-left position of the character in game-space coordinates.
         * @param size Scale factor applied to the base 32x32 glyph size.
         *
         * @note Status: IMPLEMENTED
         */
        void DrawChar(intcs rank, TinyPoint pos, double size) override;

        void HudIcon(PixmapChannel channel, intcs rank, TinyPoint pos) override;

        void QuickIcon(PixmapChannel channel, intcs rank, TinyPoint pos) override;

        void QuickIcon(PixmapChannel channel, intcs rank, TinyPoint pos, double opacity, double rotation) override;

        bool DrawPart(PixmapChannel channel, TinyPoint dest, TinyRect rect) override;

        bool DrawPart(PixmapChannel channel, TinyPoint dest, TinyRect rect, double zoom) override;

        void DrawIcon(PixmapChannel channel, intcs icon, TinyRect rect, double opacity, bool useHotSpot) override;

        void DrawIcon(PixmapChannel channel, intcs icon, TinyRect rect, double opacity, double rotationDeg,
                      bool useHotSpot) override;

    private:
        Microsoft::Xna::Framework::Rectangle GetSrcRectangle(const Texture2D& bitmap, intcs bitmapGridX,
                                                             intcs bitmapGridY,
                                                             intcs iconWidth, intcs iconHeight, intcs gap, intcs icon);

        Microsoft::Xna::Framework::Rectangle GetDstRectangle(TinyRect rect, intcs iconWidth, intcs iconHeight,
                                                             bool useHotSpot);

        const Texture2D* GetBitmap(PixmapChannel channel);
    };
}
