#pragma once

// The Ray Dream procedural materials of the object models (tools/rd4_to_gltf/procedural_materials.py
// writes avatars3d/objects/<model>/materials.txt): the converter flattened each part's mix shader
// (colours blended by wood, marble or spots) to one colour and left its bump out; the sprites show the
// patterns. The 3D effects' model shader (SceneShaders.hpp, level high) draws them again from these:
// the part's colours mixed by its patterns in the model's own space, and its bump.
//
//     material <part suffix rd_rrggbb>
//     colour <r> <g> <b>
//     layer <marble|wood|spots> <r> <g> <b> <scale> <spacing> <perturbation> <axis> <blend>
//     bump <spots|grain> <amplitude> <scale> <threshold>

#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <array>
#include <istream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace GalaxyEggbert::Game
{
    enum class MaterialPattern : int
    {
        None = 0,
        Marble = 1,
        Wood = 2,
        Spots = 3,
    };

    enum class MaterialBump : int
    {
        None = 0,
        Spots = 1,
        Grain = 2,
    };

    struct MaterialLayer
    {
        MaterialPattern pattern = MaterialPattern::None;
        Microsoft::Xna::Framework::Vector3 colour;
        float scale = 3.0f;        // Ray Dream units: the pattern's noise size
        float spacing = 4.0f;      // the veins' or rings' spacing
        float perturbation = 1.0f; // how much the noise bends them
        int axis = 0;              // 0 x, 1 y, 2 z (model space)
        float blend = 0.5f;        // 0 hard edges .. 1 soft
    };

    struct ProceduralMaterial
    {
        bool hasColour = false;
        Microsoft::Xna::Framework::Vector3 colour;
        std::array<MaterialLayer, 2> layers{};
        int layerCount = 0;
        MaterialBump bump = MaterialBump::None;
        float bumpAmplitude = 0.0f;
        float bumpScale = 1.0f;
        float bumpThreshold = 0.5f;
    };

    using ProceduralMaterials = std::unordered_map<std::string, ProceduralMaterial>;

    // Reads a materials.txt; unknown or malformed lines are skipped.
    [[nodiscard]] inline ProceduralMaterials ReadProceduralMaterials(std::istream& in)
    {
        ProceduralMaterials materials;
        ProceduralMaterial* current = nullptr;
        for (std::string line; std::getline(in, line);)
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }
            std::istringstream words(line);
            std::string kind;
            words >> kind;
            if (kind == "material")
            {
                std::string name;
                if (words >> name)
                {
                    current = &materials[name];
                }
                continue;
            }
            if (current == nullptr)
            {
                continue;
            }
            if (kind == "colour")
            {
                float r = 0, g = 0, b = 0;
                if (words >> r >> g >> b)
                {
                    current->hasColour = true;
                    current->colour = Microsoft::Xna::Framework::Vector3(r, g, b);
                }
            }
            else if (kind == "layer" && current->layerCount < static_cast<int>(current->layers.size()))
            {
                std::string pattern;
                MaterialLayer layer;
                float r = 0, g = 0, b = 0;
                if (words >> pattern >> r >> g >> b >> layer.scale >> layer.spacing >> layer.perturbation >> layer.axis >>
                    layer.blend)
                {
                    layer.pattern = pattern == "marble" ? MaterialPattern::Marble
                                    : pattern == "wood" ? MaterialPattern::Wood
                                    : pattern == "spots" ? MaterialPattern::Spots
                                                         : MaterialPattern::None;
                    layer.colour = Microsoft::Xna::Framework::Vector3(r, g, b);
                    if (layer.pattern != MaterialPattern::None)
                    {
                        current->layers[static_cast<std::size_t>(current->layerCount++)] = layer;
                    }
                }
            }
            else if (kind == "bump")
            {
                std::string pattern;
                if (words >> pattern >> current->bumpAmplitude >> current->bumpScale >> current->bumpThreshold)
                {
                    current->bump = pattern == "spots" ? MaterialBump::Spots
                                    : pattern == "grain" ? MaterialBump::Grain
                                                         : MaterialBump::None;
                }
            }
        }
        return materials;
    }

    // A model part's material: the part is named "<object>_rd_rrggbb" (its flat colour), the material
    // by that suffix; nullptr when it has none.
    [[nodiscard]] inline const ProceduralMaterial* FindPartMaterial(const ProceduralMaterials& materials,
                                                                    std::string_view partName)
    {
        const auto at = partName.rfind("rd_");
        if (at == std::string_view::npos)
        {
            return nullptr;
        }
        const auto found = materials.find(std::string(partName.substr(at)));
        return found != materials.end() ? &found->second : nullptr;
    }
}
