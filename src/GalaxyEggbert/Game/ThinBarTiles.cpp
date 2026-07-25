#include "ThinBarTiles.hpp"
#include "SwatchUv.hpp"

namespace GalaxyEggbert::Game
{
    namespace
    {
        using Easy3D::CubeFace;
    }

    bool TryGetThinBarFaces(int icon, const Easy3D::UvRect& tileUv,
                             Easy3D::DirectionalCubeFace (&outFaces)[6])
    {
        if (icon == 202)
        {
            // 4 long sides (top/bottom/front/back, i.e. every face except
            // the 2 end caps along the bar's own X-axis length) carry the
            // real texture; the 2 end caps (PosX/NegX) are a flat blue
            // fallback sampled from the icon's own texture.
            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = tileUv;
            }
            outFaces[static_cast<int>(CubeFace::PosX)].Uv = SwatchUv(tileUv, kMidSwatchV);
            outFaces[static_cast<int>(CubeFace::NegX)].Uv = SwatchUv(tileUv, kMidSwatchV);
            return true;
        }

        return false;
    }
}
