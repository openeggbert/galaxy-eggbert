#pragma once

#include "SharpRuntime/Prop.hpp"
#include "GalaxyEggbert/IPixmap.hpp"
#include "GalaxyEggbert/TinyPoint.hpp"

namespace GalaxyEggbert
{
    class Slider
    {
    public:
        Slider(TinyPoint topLeftCorner, double value);

        TinyPoint topLeftCorner;
        double value;

        [[nodiscard]] TinyPoint getTopLeftCornerProperty() const;
        void setTopLeftCornerProperty(TinyPoint point);
        DDATA(double, Value)

    private:
        [[nodiscard]] intcs getPosLeftProperty() const;
        [[nodiscard]] intcs getPosRightProperty() const;

        /*
                NeoSdk::Property<byte> SelectedGamer;
                SelectedGamer( [this]() { return data[2]; } , [this](byte value) {data[2] = value; }),
        */

    public:
        void Draw(IPixmap& pixmap);

        bool Move(TinyPoint pos);
    };
}
