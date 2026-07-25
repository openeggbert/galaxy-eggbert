#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Game/DecorQuartTable.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <GalaxyEggbert/Game/ObjectIcons.hpp>

#include <cstdint>
#include <initializer_list>
#include <iostream>

namespace
{
    int failures = 0;

    void Check(bool condition, const char* message)
    {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    bool IsOneOf(int value, std::initializer_list<int> values)
    {
        for (const int candidate : values)
        {
            if (value == candidate)
            {
                return true;
            }
        }
        return false;
    }

    GalaxyEggbert::BlockRenderMode ExpectedBlockRenderMode(int icon)
    {
        using GalaxyEggbert::BlockRenderMode;
        if (icon == 0) return BlockRenderMode::Air;
        if (icon >= 378 && icon <= 383) return BlockRenderMode::GroundAnchoredPlate;
        if (IsOneOf(icon, {76, 384, 385})) return BlockRenderMode::InnerPillarBox;
        if (icon == 202) return BlockRenderMode::ThinBar;
        if (IsOneOf(icon, {
                77, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
                121, 122, 123, 124, 125, 138, 199, 264, 265, 266, 267, 268,
                269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280,
                281, 282, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294,
                295, 296, 297, 298, 299, 300, 302, 303, 367, 368, 369, 370,
                371, 372, 398,
            })) return BlockRenderMode::InnerFlatPlate;
        if (IsOneOf(icon, {53, 54, 55, 56, 57, 58, 59, 60, 63, 64}))
            return BlockRenderMode::TripleCrossBillboard;
        if (IsOneOf(icon, {
                2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
                19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 44, 45,
                46, 47, 48, 49, 50, 51, 52, 66, 74, 75, 86, 87, 88, 89, 90,
                107, 108, 109, 126, 127, 128, 129, 130, 131, 132, 133, 134,
                135, 136, 137, 154, 155, 186, 187, 188,
                189, 190, 191, 192, 193, 194, 195, 196, 197, 200, 224, 225,
                226, 227, 228, 232, 245, 250, 251, 252, 253, 254, 255, 256,
                257, 258, 259, 260, 283, 330, 331, 332, 333, 364, 365, 366,
                390, 391, 392, 393, 394, 395, 396, 397, 400,
            })) return BlockRenderMode::DirectionalCube;
        return BlockRenderMode::UniformCube;
    }

    GalaxyEggbert::Game::ObjectTextureSource ExpectedObjectTexture(int raw)
    {
        using GalaxyEggbert::Game::ObjectTextureSource;
        if (raw == 0) return ObjectTextureSource::None;
        if (IsOneOf(raw, {1, 12, 14, 15, 31, 35, 47, 48, 52}))
            return ObjectTextureSource::ObjectM;
        if (IsOneOf(raw, {8, 9, 10, 11, 53, 90, 91, 92, 93, 98, 99, 100}))
            return ObjectTextureSource::Explo;
        if (raw == 200) return ObjectTextureSource::Blupi;
        if (IsOneOf(raw, {201, 202, 203})) return ObjectTextureSource::Blupi1;
        if (raw == 38) return ObjectTextureSource::PhaseDependent;
        return ObjectTextureSource::Element;
    }

    bool ExpectedMobileWorldSupport(int raw)
    {
        return IsOneOf(raw, {
            1, 2, 3, 4, 5, 6, 7, 12, 13, 16, 17, 19, 20, 21, 24, 25, 26, 30,
            32, 33, 40, 44, 46, 47, 49, 50, 51, 54, 55, 96,
        });
    }

    bool ExpectedPatrolMotion(int raw)
    {
        return IsOneOf(raw, {2, 3, 4, 20, 32, 33, 44, 54});
    }
}

int main()
{
    using namespace GalaxyEggbert;
    using namespace GalaxyEggbert::Game;

    const auto& blockRegistry = BlockDefinitionRegistry::Instance();
    for (int icon = 0; icon <= BlockDefinitionRegistry::MaxObjectMIcon; ++icon)
    {
        const auto& definition = blockRegistry.Get(static_cast<std::uint16_t>(icon));
        Check(definition.type == icon, "block registry preserves every icon id");
        Check(blockRegistry.IsSupported(static_cast<std::uint16_t>(icon)),
              "all object-m icon ids are supported");
        if (definition.renderMode != ExpectedBlockRenderMode(icon))
        {
            std::cerr << "render-mode mismatch for block icon " << icon << '\n';
            Check(false, "block render mode matches the pre-registry classification");
        }

        if (icon == 0)
        {
            Check(definition.textureSource == BlockTextureSource::None,
                  "Air has no texture source");
            Check(definition.collisionMask == 0, "Air has no collision mask");
            continue;
        }

        Check(definition.textureSource == BlockTextureSource::ObjectM,
              "every non-Air voxel uses object-m.png");
        Check(definition.textureIcon == icon,
              "block type remains identical to its object-m icon");

        std::uint16_t expectedMask = 0;
        for (int cell = 0; cell < 16; ++cell)
        {
            if (kDecorQuartTable[icon * 16 + cell] != 0)
            {
                expectedMask |= static_cast<std::uint16_t>(1u << cell);
            }
        }
        Check(definition.collisionMask == expectedMask,
              "registry collision mask matches table_decor_quart");
        Check(definition.mobileTransparent ==
                  (expectedMask == 0 && icon != BlockTypes::Lava &&
                   icon != BlockTypes::Crusher),
              "mobile transparent/pass decision preserves its collision exceptions");
        Check(BlockTypes::isMobileTransparent(icon) == definition.mobileTransparent,
              "legacy transparent accessor delegates to the registry");
        Check(BlockTypes::tileAnimBase(static_cast<std::uint16_t>(icon)) ==
                  definition.animation.baseIcon,
              "legacy animation-base accessor delegates to the registry");
        const bool expectedHazard =
            IsOneOf(definition.animation.baseIcon, {
                BlockTypes::Lava, BlockTypes::Spike, BlockTypes::Crusher,
                BlockTypes::Saw, BlockTypes::Blitz,
            });
        Check(BlockTypes::isHazard(static_cast<std::uint16_t>(icon)) ==
                  (expectedHazard || icon == BlockTypes::Drip),
              "legacy hazard accessor preserves animation-family semantics");
    }

    Check(!blockRegistry.IsSupported(441), "icon 441 is outside object-m");
    Check(GetBlockDefinition(BlockTypes::Lava).hazard == BlockHazardKind::Lava,
          "Lava semantic is centralized");
    Check(GetBlockDefinition(BlockTypes::Drip).hazard == BlockHazardKind::Drip,
          "Drip semantic is centralized");
    Check(GetBlockDefinition(BlockTypes::FanLeft).fanDirection ==
              BlockFanDirection::NegativeX,
          "fan direction is centralized");
    Check(GetBlockDefinition(BlockTypes::Door3).doorKeyType == 51,
          "door key is centralized");
    Check(GetBlockDefinition(BlockTypes::WorldSelect12).worldSelectIndex == 12,
          "world selector index is centralized");
    Check(GetBlockDefinition(BlockTypes::ProgressDoor8).progressDoorIndex == 8,
          "progress-door index is centralized");
    Check(ResolveBlockAnimationIcon(BlockTypes::Lava, 0) == 68 &&
              ResolveBlockAnimationIcon(BlockTypes::Lava, 2) == 69 &&
              ResolveBlockAnimationIcon(BlockTypes::Temp, 72) == -1,
          "representative block animation parity");

    const auto& objectRegistry = ObjectDefinitionRegistry::Instance();
    for (int raw = 0; raw <= ObjectDefinitionRegistry::MaxObjectType; ++raw)
    {
        const auto type = static_cast<GalaxyEggbert::Def::ObjectType>(raw);
        const auto& definition = objectRegistry.Get(type);
        Check(static_cast<int>(definition.type) == raw,
              "object registry preserves every ObjectType id");
        Check(objectRegistry.IsSupported(type), "ObjectType0..203 are supported");
        Check(definition.textureSource == ExpectedObjectTexture(raw),
              "object texture source matches the pre-registry classification");
        Check(definition.mobileWorldSupported == ExpectedMobileWorldSupport(raw),
              "mobile-world object support is centralized without parity loss");
        Check(definition.patrolMotion == ExpectedPatrolMotion(raw),
              "patrol-motion classification is centralized without parity loss");

        const auto visual0 = objectRegistry.Resolve(type, 0);
        Check(visual0.icon == GetObjIcon(type, 0),
              "resolved object icon preserves GetObjIcon phase zero");
        Check(visual0.renderMode == definition.renderMode,
              "resolved object render mode comes from its definition");
    }

    Check(!objectRegistry.IsSupported(
              static_cast<GalaxyEggbert::Def::ObjectType>(204)),
          "ObjectType204 is unsupported");
    Check(GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType12).renderMode ==
              ObjectRenderMode::SolidCube,
          "crate is centrally classified as a solid cube");
    Check(GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType2).semanticKind ==
              ObjectSemanticKind::Enemy &&
              GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType49).semanticKind ==
              ObjectSemanticKind::Collectible &&
              GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType200).semanticKind ==
              ObjectSemanticKind::Avatar,
          "stable object semantic categories are centralized");
    Check(GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType6).placementKind ==
              ObjectPlacementKind::Direct,
          "extra-life egg is directly placeable");
    Check(GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType8).placementKind ==
              ObjectPlacementKind::Transient,
          "explosion is runtime-only");
    Check(GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType59).placementKind ==
              ObjectPlacementKind::Reserved,
          "unidentified type remains reserved");

    const auto phase29 =
        ResolveObjectVisual(GalaxyEggbert::Def::ObjectType::ObjectType38, 29);
    const auto phase30 =
        ResolveObjectVisual(GalaxyEggbert::Def::ObjectType::ObjectType38, 30);
    Check(phase29.textureSource == ObjectTextureSource::Blupi1 &&
              phase30.textureSource == ObjectTextureSource::Element,
          "ObjectType38 keeps its phase-dependent texture source");

    const auto overrideVisual =
        ResolveObjectVisual(GalaxyEggbert::Def::ObjectType::ObjectType12, 0,
                            BlockTypes::RockPile);
    Check(overrideVisual.renderMode == ObjectRenderMode::SolidCube &&
              overrideVisual.textureSource == ObjectTextureSource::ObjectM &&
              overrideVisual.icon == BlockTypes::RockPile,
          "MoveObject visual override is resolved centrally");

    Check(GetBirdIcon(true, 2, 0) != GetBirdIcon(false, 2, 0),
          "directional patrol animation remains direction-sensitive");

    if (failures != 0)
    {
        std::cerr << failures << " definition-registry check(s) failed.\n";
        return 1;
    }
    std::cout << "VerifyDefinitionRegistries: all checks passed.\n";
    return 0;
}
