#pragma once

#include <GalaxyEggbert/PlayerStart.hpp>
#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::Editor
{
    // Body colours measured from the established Content/icons/button.png icons96-98,
    // normalized to full brightness. Recolour only the original yellow material (including
    // the face background); eyes, eyelashes, footwear and other materials retain their colour.
    inline void RecolorPlayerStartPixel(PlayerStartColor color, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b)
    {
        if (color==PlayerStartColor::Yellow || std::abs(int(r)-int(g))>std::max(r,g)*.12f ||
            b>=std::min(r,g)*.5f) return;
        constexpr std::array<std::array<int,3>,4> body{{{255,255,0},{255,165,23},{23,199,255},{23,255,31}}};
        const int c=static_cast<int>(color);
        if (c<1 || c>3) return;
        const float chroma=std::max(r,g)-b;
        const int base=b;
        r=std::uint8_t(std::lround(base+chroma*body[c][0]/255.f));
        g=std::uint8_t(std::lround(base+chroma*body[c][1]/255.f));
        b=std::uint8_t(std::lround(base+chroma*body[c][2]/255.f));
    }
}
