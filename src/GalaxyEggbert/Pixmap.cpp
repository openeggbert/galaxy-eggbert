//
// Created by robertvokac on 5/24/25.
//

#include "GalaxyEggbert/Pixmap.hpp"

#include "CNA/Logger.hpp"
#include "CNA/Platform.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "GalaxyEggbert/Decor.hpp"
#include "GalaxyEggbert/Def.hpp"

#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "System/Math.hpp"
#include "GalaxyEggbert/Config.hpp"
#include "GalaxyEggbert/Misc.hpp"
#include "GalaxyEggbert/Tables.hpp"
#include "GalaxyEggbert/Text.hpp"

namespace GalaxyEggbert
{
    TinyRect Pixmap::getDrawBoundsProperty()
    {
        TinyRect result{};
        double screenWidth = graphics.getGraphicsDeviceProperty()->getViewportProperty().getWidthProperty();
        double screenHeight = graphics.getGraphicsDeviceProperty()->getViewportProperty().getHeightProperty();
        if (CNA::getCurrentPlatform() == CNA::Platform::Android && screenHeight > 480)
        {
            screenWidth = screenHeight * (640.0 / 480.0);
        }
        if (screenWidth != 0.0 && screenHeight != 0.0)
        {
            double drawWidth;
            double drawHeight;
            if (screenWidth / screenHeight < 1.3333333333333333)
            {
                drawWidth = 640.0;
                drawHeight = 640.0 * (screenHeight / screenWidth);
            }
            else
            {
                drawWidth = 480.0 * (screenWidth / screenHeight);
                drawHeight = 480.0;
            }
            result.Left = 0;
            result.Right = static_cast<intcs>(drawWidth);
            result.Top = 0;
            result.Bottom = static_cast<intcs>(drawHeight);
        }
        return result;
    }

    TinyPoint Pixmap::getOriginProperty() const
    {
        return {static_cast<intcs>(originX), static_cast<intcs>(originY)};
    }

    Pixmap::Pixmap(IGame1* game1, Microsoft::Xna::Framework::Graphics::GraphicsDeviceManager& graphics) :
        game1(game1),
        graphics(graphics), spriteBatch(nullptr), bitmapText(Texture2D()), bitmapButton(Texture2D()),
        bitmapJauge(Texture2D()), bitmapBlupi(Texture2D()), bitmapBlupi1(Texture2D()),
        bitmapObject(Texture2D()),
        bitmapElement(Texture2D()),
        bitmapExplo(Texture2D()), bitmapPad(Texture2D()),
        bitmapSpeedyBlupi(Texture2D()),
        bitmapBlupiYoupie(Texture2D()),
        bitmapGear(Texture2D()),
        bitmapBackground(Texture2D()),
        origin(Microsoft::Xna::Framework::Vector2(0.0f, 0.0f))
    {
        effect = Microsoft::Xna::Framework::Graphics::SpriteEffects::None;
    }

    TinyPoint Pixmap::HotSpotToHud(TinyPoint pos)
    {
        if (hotSpotZoom == 0.0)
        {
            return pos;
        }
        return {
            static_cast<intcs>(static_cast<double>(pos.X - static_cast<intcs>(hotSpotX)) / hotSpotZoom) + static_cast<
                intcs>(hotSpotX) - static_cast<intcs>(originX),
            static_cast<intcs>(static_cast<double>(pos.Y - static_cast<intcs>(hotSpotY)) / hotSpotZoom) + static_cast<
                intcs>(hotSpotY) - static_cast<intcs>(originY)
        };
    }

    void Pixmap::SetHotSpot(const double zoom, const double x, const double y)
    {
        hotSpotZoom = zoom;
        hotSpotX = x;
        hotSpotY = y;
    }

    void Pixmap::DrawInputButton(TinyRect rect, Def::ButtonGlyph glyph, bool pressed, bool selected)
    {
        switch (glyph)
        {
        case Def::ButtonGlyph::InitGamerA:
            DrawIcon(PixmapChannel::Pad, selected ? 16 : 4, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::InitGamerB:
            DrawIcon(PixmapChannel::Pad, selected ? 17 : 5, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::InitGamerC:
            DrawIcon(PixmapChannel::Pad, selected ? 18 : 6, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::InitSetup:
        case Def::ButtonGlyph::PauseSetup:
            DrawIcon(PixmapChannel::Pad, 19, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::InitPlay:
            DrawIcon(PixmapChannel::Pad, 7, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::PauseMenu:
        case Def::ButtonGlyph::ResumeMenu:
            DrawIcon(PixmapChannel::Pad, 11, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::PauseBack:
            DrawIcon(PixmapChannel::Pad, 8, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::PauseRestart:
            DrawIcon(PixmapChannel::Pad, 9, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::PauseContinue:
        case Def::ButtonGlyph::ResumeContinue:
            DrawIcon(PixmapChannel::Pad, 10, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::WinLostReturn:
            DrawIcon(PixmapChannel::Pad, 3, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::InitBuy:
        case Def::ButtonGlyph::TrialBuy:
            DrawIcon(PixmapChannel::Pad, 22, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::InitRanking:
            DrawIcon(PixmapChannel::Pad, 12, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::TrialCancel:
        case Def::ButtonGlyph::RankingContinue:
            DrawIcon(PixmapChannel::Pad, 8, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::SetupSounds:
        case Def::ButtonGlyph::SetupJump:
        case Def::ButtonGlyph::SetupZoom:
        case Def::ButtonGlyph::SetupAccel:
            DrawIcon(PixmapChannel::Pad, selected ? 13 : 21, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::SetupReset:
            DrawIcon(PixmapChannel::Pad, 20, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::SetupReturn:
            DrawIcon(PixmapChannel::Pad, 8, rect, pressed ? 0.8 : 1.0, false);
            break;
        case Def::ButtonGlyph::PlayJump:
            DrawIcon(PixmapChannel::Pad, 2, rect, pressed ? 0.6 : 1.0, false);
            break;
        case Def::ButtonGlyph::PlayAction:
            DrawIcon(PixmapChannel::Pad, 12, rect, pressed ? 0.6 : 1.0, false);
            break;
        case Def::ButtonGlyph::PlayDown:
            DrawIcon(PixmapChannel::Pad, 23, rect, pressed ? 0.6 : 1.0, false);
            break;
        case Def::ButtonGlyph::PlayPause:
            DrawIcon(PixmapChannel::Pad, 3, rect, pressed ? 0.6 : 1.0, false);
            break;
        case Def::ButtonGlyph::Cheat1:
        case Def::ButtonGlyph::Cheat2:
        case Def::ButtonGlyph::Cheat3:
        case Def::ButtonGlyph::Cheat4:
        case Def::ButtonGlyph::Cheat5:
        case Def::ButtonGlyph::Cheat6:
        case Def::ButtonGlyph::Cheat7:
        case Def::ButtonGlyph::Cheat8:
        case Def::ButtonGlyph::Cheat9:
            {
                DrawIcon(PixmapChannel::Pad, 0, rect, pressed ? 0.6 : 1.0, false);
                TinyPoint tinyPoint{
                    rect.Left + rect.getWidthProperty() / 2 - static_cast<intcs>(originX),
                    rect.Top + 28
                };
                Text::DrawTextCenter(*this, tinyPoint, Decor::GetCheatTinyText(glyph), 1.0);
                break;
            }
        case Def::ButtonGlyph::Cheat11:
        case Def::ButtonGlyph::Cheat12:
        case Def::ButtonGlyph::Cheat21:
        case Def::ButtonGlyph::Cheat22:
        case Def::ButtonGlyph::Cheat31:
        case Def::ButtonGlyph::Cheat32:
            break;
        }
    }

    void Pixmap::LoadContent()
    {
        CNA::Logger::Info("SpeedyBlupi: Pixmap::LoadContent entered");
        CNA::Logger::Info("SpeedyBlupi: asset root = " + game1->getContentProperty().getRootDirectoryProperty());
        spriteBatch = std::make_unique<Microsoft::Xna::Framework::Graphics::SpriteBatch>(
            game1->getGraphicsDeviceProperty());
        CNA::Logger::Info("SpeedyBlupi: loading icons/text");
        bitmapText = game1->getContentProperty().Load<Texture2D>("icons/text");
        bitmapButton = game1->getContentProperty().Load<Texture2D>("icons/button");
        bitmapJauge = game1->getContentProperty().Load<Texture2D>("icons/jauge");
        bitmapBlupi = game1->getContentProperty().Load<Texture2D>("icons/blupi");
        bitmapBlupi1 = game1->getContentProperty().Load<Texture2D>("icons/blupi1");
        bitmapObject = game1->getContentProperty().Load<Texture2D>("icons/object-m");
        bitmapElement = game1->getContentProperty().Load<Texture2D>("icons/element");
        bitmapExplo = game1->getContentProperty().Load<Texture2D>("icons/explo");
        bitmapPad = game1->getContentProperty().Load<Texture2D>("icons/pad");
        bitmapSpeedyBlupi = game1->getContentProperty().Load<Texture2D>("backgrounds/speedyblupi");
        bitmapBlupiYoupie = game1->getContentProperty().Load<Texture2D>("backgrounds/blupiyoupie");
        bitmapGear = game1->getContentProperty().Load<Texture2D>("backgrounds/gear");
        CNA::Logger::Info("SpeedyBlupi: Pixmap::LoadContent done");
        UpdateGeometry();
    }

    void Pixmap::UpdateGeometry()
    {
        double screenWidth = graphics.getGraphicsDeviceProperty()->getViewportProperty().getWidthProperty();
        double screenHeight = graphics.getGraphicsDeviceProperty()->getViewportProperty().getHeightProperty();
        if (CNA::getCurrentPlatform() == CNA::Platform::Android && screenHeight > 480)
        {
            screenWidth = screenHeight * (640.0 / 480.0);
        }
        double widthScale = screenWidth / 640.0;
        double heightScale = screenHeight / 480.0;
        zoom = System::Math::Min(widthScale, heightScale);
        originX = (screenWidth - 640.0 * zoom) / 2.0;
        originY = (screenHeight - 480.0 * zoom) / 2.0;
    }

    void Pixmap::BackgroundCache(const string& name)
    {
        CNA::Logger::Info("SpeedyBlupi: BackgroundCache loading backgrounds/" + name);
        bitmapBackground = game1->getContentProperty().Load<Texture2D>("backgrounds/" + name);
        CNA::Logger::Info("SpeedyBlupi: BackgroundCache done backgrounds/" + name);
    }

    bool Pixmap::Start()
    {
        graphics.getGraphicsDeviceProperty()->Clear(Microsoft::Xna::Framework::CornflowerBlue);
        return true;
    }

    bool Pixmap::Finish()
    {
        return true;
    }

    void Pixmap::DrawBackground()
    {
        if (!spriteBatch)
        {
            CNA::Logger::Error("Pixmap::DrawBackground called before LoadContent()");
            return;
        }
        double screenWidth = graphics.getGraphicsDeviceProperty()->getViewportProperty().getWidthProperty();
        double screenHeight = graphics.getGraphicsDeviceProperty()->getViewportProperty().getHeightProperty();
        if (CNA::getCurrentPlatform() == CNA::Platform::Android && screenHeight > 480)
        {
            screenWidth = screenHeight * (640.0 / 480.0);
        }
        const Texture2D* bitmap_ptr = GetBitmap(PixmapChannel::Background);
        if (bitmap_ptr == nullptr)
        {
            CNA::Logger::Error("GetBitmap returned nullptr for channel=3");
            return;
        }
        const Texture2D& bitmap = *bitmap_ptr;
        Microsoft::Xna::Framework::Rectangle srcRectangle = GetSrcRectangle(bitmap, 10, 10, 10, 10, 0, 0);
        Microsoft::Xna::Framework::Rectangle destinationRectangle = Microsoft::Xna::Framework::Rectangle(
            0, 0, static_cast<intcs>(screenWidth), static_cast<intcs>(screenHeight));
        spriteBatch->Begin(Microsoft::Xna::Framework::Graphics::SpriteSortMode::BackToFront,
                           Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        spriteBatch->Draw(bitmap, destinationRectangle, srcRectangle, Microsoft::Xna::Framework::White);
        spriteBatch->End();
        TinyPoint dest{
            static_cast<intcs>(originX),
            static_cast<intcs>(originY)
        };
        TinyRect rect{};
        rect.Left = 0;
        rect.Top = 0;
        rect.Right = 640;
        rect.Bottom = 480;
        DrawPart(PixmapChannel::Background, dest, rect);
    }

    void Pixmap::DrawChar(intcs rank, TinyPoint pos, double size)
    {
        pos.X = static_cast<intcs>(static_cast<double>(pos.X) + originX);
        pos.Y = static_cast<intcs>(static_cast<double>(pos.Y) + originY);
        TinyRect tinyRect{};
        tinyRect.Left = pos.X;
        tinyRect.Top = pos.Y;
        tinyRect.Right = pos.X + static_cast<intcs>(32.0 * size);
        tinyRect.Bottom = pos.Y + static_cast<intcs>(32.0 * size);
        DrawIcon(PixmapChannel::Text, rank, tinyRect, 1.0, false);
    }

    void Pixmap::HudIcon(PixmapChannel channel, intcs rank, TinyPoint pos)
    {
        pos.X = static_cast<intcs>(static_cast<double>(pos.X) + originX);
        pos.Y = static_cast<intcs>(static_cast<double>(pos.Y) + originY);
        TinyRect tinyRect{pos};
        DrawIcon(channel, rank, tinyRect, 1.0, false);
    }

    void Pixmap::QuickIcon(PixmapChannel channel, intcs rank, TinyPoint pos)
    {
        TinyRect tinyRect{pos};
        DrawIcon(channel, rank, tinyRect, 1.0, true);
    }

    void Pixmap::QuickIcon(PixmapChannel channel, intcs rank, TinyPoint pos, double opacity, double rotation)
    {
        TinyRect tinyRect{pos};
        DrawIcon(channel, rank, tinyRect, opacity, rotation, true);
    }

    bool Pixmap::DrawPart(PixmapChannel channel, TinyPoint dest, TinyRect rect)
    {
        return DrawPart(channel, dest, rect, 1.0);
    }

    bool Pixmap::DrawPart(PixmapChannel channel, TinyPoint dest, TinyRect rect, double zoom)
    {
        if (!spriteBatch)
        {
            CNA::Logger::Error("Pixmap::DrawPart called before LoadContent()");
            return false;
        }
        const Texture2D* bitmap = GetBitmap(channel);
        if (bitmap == nullptr)
        {
            return false;
        }
        if (channel == PixmapChannel::Jauge)
        {
            CNA::Logger::Debug(
                "Pixmap::DrawPart ch5 BEFORE origin: dest=(" + std::to_string(dest.X) + "," + std::to_string(dest.Y) +
                "), srcRect=(" + std::to_string(rect.Left) + "," + std::to_string(rect.Top) + "," +
                std::to_string(rect.Right) + "," + std::to_string(rect.Bottom) + "), zoom=" + std::to_string(zoom));

            dest.X = static_cast<intcs>(static_cast<double>(dest.X) + originX);
            dest.Y = static_cast<intcs>(static_cast<double>(dest.Y) + originY);
            CNA::Logger::Debug(
                "Pixmap::DrawPart ch5 AFTER origin: dest=(" + std::to_string(dest.X) + "," + std::to_string(dest.Y) +
                "), origin=(" + std::to_string(originX) + "," + std::to_string(originY) + ")");
        }
        using Microsoft::Xna::Framework::Rectangle;
        Rectangle value = Rectangle(
            rect.Left, rect.Top, rect.getWidthProperty(), rect.getHeightProperty());
        Rectangle destinationRectangle = Rectangle(
            dest.X,
            dest.Y,
            static_cast<intcs>(static_cast<double>(rect.getWidthProperty()) * zoom),
            static_cast<intcs>(static_cast<double>(rect.getHeightProperty()) * zoom)
        );
        spriteBatch->Begin(Microsoft::Xna::Framework::Graphics::SpriteSortMode::BackToFront,
                           Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
        spriteBatch->Draw(*bitmap, destinationRectangle, value, Microsoft::Xna::Framework::White);
        spriteBatch->End();
        return true;
    }

    void Pixmap::DrawIcon(PixmapChannel channel, intcs icon, TinyRect rect, double opacity, bool useHotSpot)
    {
        if (!spriteBatch)
        {
            CNA::Logger::Error("Pixmap::DrawIcon called before LoadContent()");
            return;
        }
        DrawIcon(channel, icon, rect, opacity, 0.0, useHotSpot);
    }

    void Pixmap::DrawIcon(PixmapChannel channel, intcs icon, TinyRect rect, double opacity, double rotationDeg, bool useHotSpot)
    {
        if (!spriteBatch)
        {
            CNA::Logger::Error("Pixmap::DrawIcon called before LoadContent()");
            return;
        }
        if (icon == -1)
        {
            return;
        }
        using Microsoft::Xna::Framework::Input::Touch::TouchPanel;
        if (Config::TOUCH_BUTTONS_SHOWN_ONLY_IF_TOUCHSCREEN_IS_AVAILABLE && CNA::getCurrentPlatform() !=
            CNA::Platform::Android && channel == PixmapChannel::Pad && !TouchPanel::GetCapabilities().getIsConnectedProperty())
        {
            static intcs padGameplayIconNumbers[] = {0, 1, 2, 3, 30, 12, 23};
            for (intcs iconNumber : padGameplayIconNumbers)
            {
                if (iconNumber == icon)
                {
                    if (iconNumber == 1 && rect.Left > 100) { continue; }
                    // Touch display is not connected and the icon is a gameplay icon. Nothing to do.
                    return;
                }
            }
        }
        const Texture2D* bitmap = GetBitmap(channel);
        if (bitmap == nullptr)
        {
            return;
        }
        intcs bitmapGridX;
        intcs bitmapGridY;
        intcs iconWidth;
        intcs iconHeight;
        intcs gap;
        switch (channel)
        {
        case PixmapChannel::Blupi:
        case PixmapChannel::Blupi1_11:
        case PixmapChannel::Blupi1_12:
        case PixmapChannel::Blupi1_13:
            bitmapGridX = 60;
            bitmapGridY = 60;
            iconWidth = 60;
            iconHeight = 60;
            gap = 0;
            break;
        case PixmapChannel::Object:
            bitmapGridX = 64;
            bitmapGridY = 64;
            iconWidth = 64;
            iconHeight = 64;
            gap = 1;
            break;
        case PixmapChannel::Element:
            bitmapGridX = 60;
            bitmapGridY = 60;
            iconWidth = 60;
            iconHeight = 60;
            gap = 0;
            break;
        case PixmapChannel::Explosion:
            bitmapGridX = 144;
            bitmapGridY = 144;
            iconHeight = Tables::table_explo_size[icon];
            iconWidth = System::Math::Max(iconHeight, 128);
            gap = 0;
            break;
        case PixmapChannel::Text:
            bitmapGridX = 32;
            bitmapGridY = 32;
            iconWidth = 32;
            iconHeight = 32;
            gap = 0;
            break;
        case PixmapChannel::Button:
            bitmapGridX = 40;
            bitmapGridY = 40;
            iconWidth = 40;
            iconHeight = 40;
            gap = 0;
            break;
        case PixmapChannel::Pad:
            bitmapGridX = 140;
            bitmapGridY = 140;
            iconWidth = 140;
            iconHeight = 140;
            gap = 0;
            break;
        case PixmapChannel::SpeedyBlupiBackground:
            bitmapGridX = 640;
            bitmapGridY = 160;
            iconWidth = 640;
            iconHeight = 160;
            gap = 0;
            break;
        case PixmapChannel::BlupiYoupieBackground:
            bitmapGridX = 410;
            bitmapGridY = 380;
            iconWidth = 410;
            iconHeight = 380;
            gap = 0;
            break;
        case PixmapChannel::GearBackground:
            bitmapGridX = 226;
            bitmapGridY = 226;
            iconWidth = 226;
            iconHeight = 226;
            gap = 0;
            break;
        default:
            bitmapGridX = 0;
            bitmapGridY = 0;
            iconWidth = 0;
            iconHeight = 0;
            gap = 0;
            break;
        }
        if (bitmapGridX != 0)
        {
            using Microsoft::Xna::Framework::Rectangle;
            Rectangle srcRectangle = GetSrcRectangle(
                *bitmap, bitmapGridX, bitmapGridY, iconWidth, iconHeight, gap, icon);
            Rectangle rectangle = GetDstRectangle(rect, iconWidth, iconHeight, useHotSpot);
            float rotationRad = 0.0f;
            if (rotationDeg != 0.0)
            {
                rotationRad = static_cast<float>(Misc::DegToRad(rotationDeg));
                rectangle = Misc::RotateAdjust(rectangle, rotationRad);
            }
            spriteBatch->Begin(Microsoft::Xna::Framework::Graphics::SpriteSortMode::BackToFront,
                               Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
            spriteBatch->Draw(*bitmap, rectangle, srcRectangle,
                              Microsoft::Xna::Framework::Color::FromNonPremultiplied(
                                  255, 255, 255, static_cast<intcs>(255.0 * opacity)), rotationRad, origin, effect,
                              0.0f);
            spriteBatch->End();
        }
    }

    Microsoft::Xna::Framework::Rectangle Pixmap::GetSrcRectangle(const Texture2D& bitmap, intcs bitmapGridX,
                                                                 intcs bitmapGridY,
                                                                 intcs iconWidth, intcs iconHeight, intcs gap,
                                                                 intcs icon)
    {
        intcs width = bitmap.getBoundsProperty().Width;
        // warning: variable height is not used
        intcs height = bitmap.getBoundsProperty().Height;
        intcs column = icon % (width / bitmapGridX);
        intcs row = icon / (width / bitmapGridX);
        bitmapGridX += gap;
        bitmapGridY += gap;
        return Microsoft::Xna::Framework::Rectangle(gap + column * bitmapGridX, gap + row * bitmapGridY, iconWidth,
                                                    iconHeight);
    }

    Microsoft::Xna::Framework::Rectangle Pixmap::GetDstRectangle(TinyRect rect, intcs iconWidth, intcs iconHeight,
                                                                 bool useHotSpot)
    {
        intcs finalWidth = ((rect.getWidthProperty() == 0) ? iconWidth : rect.getWidthProperty());
        intcs finalHeight = ((rect.getHeightProperty() == 0) ? iconHeight : rect.getHeightProperty());
        intcs scaledLeftX = (intcs)((double)rect.Left * zoom);
        intcs scaledTopY = (intcs)((double)rect.Top * zoom);
        intcs scaledRightX = (intcs)((double)scaledLeftX + (double)finalWidth * zoom);
        intcs scaledBottomY = (intcs)((double)scaledTopY + (double)finalHeight * zoom);
        if (useHotSpot && hotSpotZoom > 1.0)
        {
            scaledLeftX -= (intcs)hotSpotX;
            scaledTopY -= (intcs)hotSpotY;
            scaledRightX -= (intcs)hotSpotX;
            scaledBottomY -= (intcs)hotSpotY;
            scaledLeftX = (intcs)((double)scaledLeftX * hotSpotZoom);
            scaledTopY = (intcs)((double)scaledTopY * hotSpotZoom);
            scaledRightX = (intcs)((double)scaledRightX * hotSpotZoom);
            scaledBottomY = (intcs)((double)scaledBottomY * hotSpotZoom);
            scaledLeftX += (intcs)hotSpotX;
            scaledTopY += (intcs)hotSpotY;
            scaledRightX += (intcs)hotSpotX;
            scaledBottomY += (intcs)hotSpotY;
        }
        return Microsoft::Xna::Framework::Rectangle(scaledLeftX, scaledTopY, scaledRightX - scaledLeftX,
                                                    scaledBottomY - scaledTopY);
    }

    const Texture2D* Pixmap::GetBitmap(PixmapChannel channel)
    {
        switch (channel)
        {
        case PixmapChannel::Blupi:
            return &bitmapBlupi;
        case PixmapChannel::Blupi1_11:
        case PixmapChannel::Blupi1_12:
        case PixmapChannel::Blupi1_13:
            return &bitmapBlupi1;
        case PixmapChannel::Object:
            return &bitmapObject;
        case PixmapChannel::Element:
            return &bitmapElement;
        case PixmapChannel::Explosion:
            return &bitmapExplo;
        case PixmapChannel::Text:
            return &bitmapText;
        case PixmapChannel::Button:
            return &bitmapButton;
        case PixmapChannel::Jauge:
            return &bitmapJauge;
        case PixmapChannel::Pad:
            return &bitmapPad;
        case PixmapChannel::SpeedyBlupiBackground:
            return &bitmapSpeedyBlupi;
        case PixmapChannel::BlupiYoupieBackground:
            return &bitmapBlupiYoupie;
        case PixmapChannel::GearBackground:
            return &bitmapGear;
        case PixmapChannel::Background:
            return &bitmapBackground;
        default:
            return nullptr;
        }
    }
}
