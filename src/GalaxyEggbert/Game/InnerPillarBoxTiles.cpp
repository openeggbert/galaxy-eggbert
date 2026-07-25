#include "InnerPillarBoxTiles.hpp"
#include "SwatchUv.hpp"

namespace GalaxyEggbert::Game
{
    namespace
    {
        using Easy3D::CubeFace;
    }

    bool TryGetInnerPillarBoxFaces(int icon, const Easy3D::UvRect& tileUv,
                                   Easy3D::DirectionalCubeFace (&outFaces)[6])
    {
        if (icon == 76)
        {
            // "vnější krychle bloku je celá (všech 6 stran) průhledná;
            // uvnitř bloku je menší kvádr (sloup), který má texturu na
            // svých 4 bočních stranách" -- top/bottom of the inner box are
            // unstated; treated as open to match the "outer cube fully
            // transparent" theme (a totem-pole-like look from directly
            // above/below), same convention as icon 200's unstated faces.
            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = tileUv;
            }
            outFaces[static_cast<int>(CubeFace::PosY)].Visible = false;
            outFaces[static_cast<int>(CubeFace::NegY)].Visible = false;
            return true;
        }

        if (icon == 384 || icon == 385)
        {
            // BlockTypes::Switch/SwitchOff: "uvnitř kvádr s texturou na
            // jedné straně, zbylé strany kvádru plná barva (modrý odstín)"
            // -- 1 side textured, other 5 (including top/bottom of the
            // inner box) flat fallback color. No confirmed facing cue in
            // either the questionnaire text or the crop (a symmetric
            // wall-panel icon) -- defaults to PosZ, same convention as
            // DirectionalCubeTiles's symmetric-icon default.
            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = SwatchUv(tileUv, kMidSwatchV);
            }
            outFaces[static_cast<int>(CubeFace::PosY)].Uv = SwatchUv(tileUv, kTopSwatchV);
            outFaces[static_cast<int>(CubeFace::NegY)].Uv = SwatchUv(tileUv, kBottomSwatchV);
            outFaces[static_cast<int>(CubeFace::PosZ)].Uv = tileUv;
            return true;
        }

        return false;
    }
}
