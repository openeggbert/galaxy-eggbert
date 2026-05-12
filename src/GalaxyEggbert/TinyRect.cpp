#include "GalaxyEggbert/TinyRect.hpp"

namespace GalaxyEggbert
{
    TinyRect::TinyRect(TinyPoint point)
        : Left(point.X), Right(point.X), Top(point.Y), Bottom(point.Y)
    {
    }

    intcs TinyRect::getWidthProperty() const
    {
        return Right - Left;
    }

    intcs TinyRect::getHeightProperty() const
    {
        return Bottom - Top;
    }
}
