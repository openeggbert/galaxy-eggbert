#include "GalaxyEggbert/TinyPoint.hpp"

#include "GalaxyEggbert/Jauge.hpp"

#include "CNA/Logger.hpp"

namespace GalaxyEggbert
{
    double Jauge::getZoomProperty() const { return m_zoom; }
    void Jauge::setZoomProperty(double v) { m_zoom = v; }

    Jauge::Jauge() : m_pixmap(nullptr), m_sound(nullptr), m_mode(JaugeMode::Empty),
                     m_bHide(true),
                     m_bMinimizeRedraw(false),
                     m_bRedraw(false),
                     m_zoom(1.0),
                     m_level(0)
    {
    }

    bool Jauge::Create(IPixmap* pixmap, ISound* sound, TinyPoint pos, JaugeMode mode, bool bMinimizeRedraw)
    {
        m_pixmap = pixmap;
        m_sound = sound;
        m_mode = mode;
        m_bMinimizeRedraw = bMinimizeRedraw;
        m_bHide = true;
        m_pos = pos;
        m_dim.X = 124;
        m_dim.Y = 22;
        m_level = 0;
        m_bRedraw = true;
        return true;
    }

    void Jauge::Draw()
    {
        if (m_pixmap == nullptr)
        {
            return;
        }
        TinyRect rect{};
        if (m_bMinimizeRedraw && !m_bRedraw)
        {
            return;
        }
        m_bRedraw = false;
        if (!m_bHide)
        {
            int filledWidth = m_level * 114 / 100;
            CNA::Logger::Debug(
                "Jauge::Draw: level=" + std::to_string(m_level) +
                ", mode=" + std::to_string(ToRaw(m_mode)) +
                ", filledWidth=" + std::to_string(filledWidth) +
                ", zoom=" + std::to_string(m_zoom) +
                ", pos=(" + std::to_string(m_pos.X) + "," + std::to_string(m_pos.Y) + ")");

            rect.Left = 0;
            rect.Right = 124;
            rect.Top = 0;
            rect.Bottom = 22;
            m_pixmap->DrawPart(PixmapChannel::Jauge, m_pos, rect, m_zoom);
            if (filledWidth > 0)
            {
                rect.Left = 0;
                rect.Right = 6 + filledWidth;
                rect.Top = 22 * ToRaw(m_mode);
                rect.Bottom = 22 * (ToRaw(m_mode) + 1);
                CNA::Logger::Debug(
                    "Jauge::Draw fill rect: L=" + std::to_string(rect.Left) +
                    " T=" + std::to_string(rect.Top) +
                    " R=" + std::to_string(rect.Right) +
                    " B=" + std::to_string(rect.Bottom));
                m_pixmap->DrawPart(PixmapChannel::Jauge, m_pos, rect, m_zoom);
            }
        }
    }

    void Jauge::Redraw()
    {
        m_bRedraw = true;
    }

    int Jauge::GetLevel()
    {
        return m_level;
    }

    void Jauge::SetLevel(int level)
    {
        if (level < 0)
        {
            level = 0;
        }
        if (level > 100)
        {
            level = 100;
        }
        if (m_level != level)
        {
            m_bRedraw = true;
        }
        m_level = level;
    }

    JaugeMode Jauge::GetMode()
    {
        return m_mode;
    }

    void Jauge::SetMode(JaugeMode mode)
    {
        if (m_mode != mode)
        {
            m_bRedraw = true;
        }
        m_mode = mode;
    }

    bool Jauge::GetHide()
    {
        return m_bHide;
    }

    void Jauge::SetHide(bool bHide)
    {
        if (m_bHide != bHide)
        {
            m_bRedraw = true;
        }
        m_bHide = bHide;
    }

    TinyPoint Jauge::GetPos() const
    {
        return m_pos;
    }

    void Jauge::SetRedraw()
    {
        m_bRedraw = true;
    }
}
