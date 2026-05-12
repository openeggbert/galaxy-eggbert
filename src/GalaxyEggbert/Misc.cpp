#include "GalaxyEggbert/Misc.hpp"

#include "System/Math.hpp"

namespace GalaxyEggbert
{
    Microsoft::Xna::Framework::Rectangle Misc::RotateAdjust(const Microsoft::Xna::Framework::Rectangle& rect,
                                                            const double angle)
    {
        TinyPoint center{};
        center.X = rect.Width / 2;
        center.Y = rect.Height / 2;
        TinyPoint originalCenter = center;
        TinyPoint rotatedCenter = RotatePointRad(angle, originalCenter);
        int offsetX = rotatedCenter.X - originalCenter.X;
        int offsetY = rotatedCenter.Y - originalCenter.Y;
        return {
            rect.getLeftProperty() - offsetX,
            rect.getTopProperty() - offsetY,
            rect.Width,
            rect.Height
        };
    }

    TinyPoint Misc::RotatePointRad(double angle, const TinyPoint& p)
    {
        return RotatePointRad(TinyPoint{}, angle, p);
    }

    TinyPoint Misc::RotatePointRad(const TinyPoint& center, double angle, const TinyPoint& point)
    {
        TinyPoint relativePoint{};
        TinyPoint rotatedPoint{};

        relativePoint.X = point.X - center.X;
        relativePoint.Y = point.Y - center.Y;

        double sinAngle = System::Math::Sin(angle);
        double cosAngle = System::Math::Cos(angle);

        rotatedPoint.X = static_cast<int>(
            static_cast<double>(relativePoint.X) * cosAngle -
            static_cast<double>(relativePoint.Y) * sinAngle);

        rotatedPoint.Y = static_cast<int>(
            static_cast<double>(relativePoint.X) * sinAngle +
            static_cast<double>(relativePoint.Y) * cosAngle);

        rotatedPoint.X += center.X;
        rotatedPoint.Y += center.Y;

        return rotatedPoint;
    }

    double Misc::DegToRad(double angle)
    {
        return angle * System::Math::PI / 180.0;
    }

    intcs Misc::Approach(intcs actual, const intcs final, const intcs step)
    {
        if (actual < final)
        {
            actual = System::Math::Min(actual + step, final);
        }
        else if (actual > final)
        {
            actual = System::Math::Max(actual - step, final);
        }
        return actual;
    }

    intcs Misc::Speed(const double speed, const intcs max)
    {
        if (speed > 0.0)
        {
            return System::Math::Max(static_cast<intcs>(speed * static_cast<double>(max)), 1);
        }
        if (speed < 0.0)
        {
            return System::Math::Min(static_cast<intcs>(speed * static_cast<double>(max)), -1);
        }
        return 0;
    }

    TinyRect Misc::Inflate(const TinyRect& rect, const intcs value)
    {
        TinyRect result{};
        result.Left = rect.Left - value;
        result.Right = rect.Right + value;
        result.Top = rect.Top - value;
        result.Bottom = rect.Bottom + value;
        return result;
    }

    bool Misc::IsInside(const TinyRect& rect, const TinyPoint& p)
    {
        return p.X >= rect.Left && p.X <= rect.Right && p.Y >= rect.Top && p.Y <= rect.Bottom;
    }

    bool Misc::IntersectRect(TinyRect& dst, const TinyRect& src1, const TinyRect& src2)
    {
        dst = TinyRect{};
        dst.Left = System::Math::Max(src1.Left, src2.Left);
        dst.Right = System::Math::Min(src1.Right, src2.Right);
        dst.Top = System::Math::Max(src1.Top, src2.Top);
        dst.Bottom = System::Math::Min(src1.Bottom, src2.Bottom);
        return !IsRectEmpty(dst);
    }

    bool Misc::UnionRect(TinyRect& dst, const TinyRect& src1, const TinyRect& src2)
    {
        dst = TinyRect();
        dst.Left = System::Math::Min(src1.Left, src2.Left);
        dst.Right = System::Math::Max(src1.Right, src2.Right);
        dst.Top = System::Math::Min(src1.Top, src2.Top);
        dst.Bottom = System::Math::Max(src1.Bottom, src2.Bottom);
        return !IsRectEmpty(dst);
    }

    bool Misc::IsRectEmpty(const TinyRect& rect)
    {
        return rect.getWidthProperty() <= 0 || rect.getHeightProperty() <= 0;
    }
}
