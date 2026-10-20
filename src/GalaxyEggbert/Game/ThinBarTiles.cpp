#include "ThinBarTiles.hpp"
#include "SwatchUv.hpp"

#include <cstdlib>

namespace GalaxyEggbert::Game
{
    namespace
    {
        using CnaEngine::Graphics::BoxFace;
    }

    void ConfigureThinBarFaces(int icon, const CnaEngine::Graphics::UvRect& tileUv,
                               CnaEngine::Graphics::BoxFaces& outFaces)
    {
        if (icon == 202)
        {
            // 4 long sides (top/bottom/front/back, i.e. every face except
            // the 2 end caps along the bar's own X-axis length) carry the
            // real texture; the 2 end caps (PosX/NegX) are a flat blue
            // fallback sampled from the icon's own texture.
            for (auto& face : outFaces)
            {
                face.visible = true;
                face.uv = tileUv;
            }
            outFaces[static_cast<int>(BoxFace::PositiveX)].uv = SwatchUv(tileUv, kMidSwatchV);
            outFaces[static_cast<int>(BoxFace::NegativeX)].uv = SwatchUv(tileUv, kMidSwatchV);
            return;
        }

        std::abort();
    }
}
