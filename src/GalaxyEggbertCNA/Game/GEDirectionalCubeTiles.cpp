#include "GEDirectionalCubeTiles.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::CNA
{
    bool TryGetDirectionalCubeFaces(int icon, const Easy3D::UvRect& tileUv,
                                    Easy3D::DirectionalCubeFace (&outFaces)[6])
    {
        using Easy3D::CubeFace;

        if (icon == GalaxyEggbert::BlockTypes::Platform)
        {
            // Icon 200 ("Platform"): confirmed by round-1 Q&A
            // (questionnaire-unidentified-tiles.md) and re-confirmed in
            // 02-tiles.md's icon 200 entry -- a passable grate/grid, textured
            // on all 4 side faces, top and bottom genuinely open (a real
            // hole, not a fallback color -- see BlockTypes.hpp's Platform
            // comment).
            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = tileUv;
            }
            outFaces[static_cast<int>(CubeFace::PosY)].Visible = false;
            outFaces[static_cast<int>(CubeFace::NegY)].Visible = false;
            return true;
        }

        return false;
    }
}
