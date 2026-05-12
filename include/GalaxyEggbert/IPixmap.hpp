#pragma once

#include "Def.hpp"
#include "enums/PixmapChannel.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "GalaxyEggbert/TinyPoint.hpp"
#include "GalaxyEggbert/TinyRect.hpp"

namespace GalaxyEggbert
{
    using Microsoft::Xna::Framework::Graphics::Texture2D;

    class IPixmap
    {
    protected:
        virtual ~IPixmap() = default;

    public:
        [[nodiscard]] virtual TinyRect getDrawBoundsProperty() = 0;

        [[nodiscard]] virtual TinyPoint getOriginProperty() const = 0;

        virtual TinyPoint HotSpotToHud(TinyPoint pos) = 0;

        virtual void SetHotSpot(double zoom, double x, double y) = 0;

        virtual void DrawInputButton(TinyRect rect, Def::ButtonGlyph glyph, bool pressed, bool selected) = 0;
        /**
         * @brief Loads all texture resources and creates the sprite batch.
         *
         * Must be called before any drawing method is used.
         *
         * @note Status: PARTIAL
         */
        virtual void LoadContent() = 0;

        virtual void BackgroundCache(const std::string& name) = 0;
        /**
         * @brief Begins a frame by clearing the graphics device.
         *
         * @return Always true.
         *
         * @note Status: PARTIAL
         */
        virtual bool Start() = 0;
        /**
         * @brief Ends a frame.
         *
         * Currently a placeholder because SpriteBatch drawing is begun and ended
         * inside each draw call.
         *
         * @return Always true.
         *
         * @note Status: STUB
         */
        virtual bool Finish() = 0;

        virtual void DrawBackground() = 0;

        virtual void DrawChar(intcs rank, TinyPoint pos, double size) = 0;

        virtual void HudIcon(PixmapChannel channel, intcs rank, TinyPoint pos) = 0;

        virtual void QuickIcon(PixmapChannel channel, intcs rank, TinyPoint pos) = 0;

        virtual void QuickIcon(PixmapChannel channel, intcs rank, TinyPoint pos, double opacity, double rotation) = 0;

        virtual bool DrawPart(PixmapChannel channel, TinyPoint dest, TinyRect rect) = 0;

        virtual bool DrawPart(PixmapChannel channel, TinyPoint dest, TinyRect rect, double zoom) = 0;

        virtual void DrawIcon(PixmapChannel channel, intcs icon, TinyRect rect, double opacity, bool useHotSpot) = 0;

        virtual void DrawIcon(PixmapChannel channel, intcs icon, TinyRect rect, double opacity, double rotationDeg,
                              bool useHotSpot) = 0;
    };
}
