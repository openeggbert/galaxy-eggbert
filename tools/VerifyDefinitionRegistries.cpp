#include <set>
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Game/DecorQuartTable.hpp>
#include <GalaxyEggbert/Game/EnemyBlupi.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <GalaxyEggbert/Game/ObjectIcons.hpp>

#include <algorithm>
#include <array>
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
        if (icon >= 378 && icon <= 383) return BlockRenderMode::SawBlade;
        if (IsOneOf(icon, {76, 182, 183, 384, 385})) return BlockRenderMode::InnerPillarBox;
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
        if ((icon >= 158 && icon <= 181) || (icon >= 410 && icon <= 420) || IsOneOf(icon, {184, 309, 310}))
            return BlockRenderMode::InnerFlatPlate; // hub signboards
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
        // The enemy Blupis' channels 11-13 are blupi1.png (OR-0063).
        if (IsOneOf(raw, {32, 33, 201, 202, 203})) return ObjectTextureSource::Blupi1;
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

    bool ExpectedDynamiteDestructible(int raw)
    {
        return IsOneOf(raw, {
            2, 3, 4, 6, 12, 13, 16, 17, 18, 19, 20, 24, 25, 26, 28, 30,
            32, 33, 34, 40, 44, 46, 52, 54, 96, 97, 200, 201, 202, 203,
        });
    }

    bool ExpectedGenericContactHazard(int raw)
    {
        return IsOneOf(raw, {2, 3, 4, 16, 17, 20, 96, 97});
    }

    GalaxyEggbert::Game::GenericContactHazardFeedback
    ExpectedGenericContactHazardFeedback(int raw)
    {
        using GalaxyEggbert::Game::GenericContactHazardFeedback;
        return IsOneOf(raw, {17, 20})
            ? GenericContactHazardFeedback::Big
            : GenericContactHazardFeedback::Small;
    }

    bool ExpectedGenericContactHazardCrouchImmune(int raw)
    {
        return raw == 3;
    }

    GalaxyEggbert::Game::LiftConveyorDirection
    ExpectedLiftConveyorDirection(int raw)
    {
        using GalaxyEggbert::Game::LiftConveyorDirection;
        if (raw == 47) return LiftConveyorDirection::PositiveX;
        if (raw == 48) return LiftConveyorDirection::NegativeX;
        return LiftConveyorDirection::None;
    }

    bool ExpectedBalloonPoppableHazard(int raw)
    {
        return IsOneOf(raw, {3, 16, 96, 97});
    }

    bool ExpectedStandardPickupTouch(int raw)
    {
        return IsOneOf(raw, {5, 6, 7, 21, 25, 26, 29, 30, 31, 40, 49, 50,
                             51, 55});
    }

    bool ExpectedSmallEnemy(int raw)
    {
        return IsOneOf(raw, {4, 32, 33});
    }

    bool ExpectedPersoDecoyAvatarTarget(int raw)
    {
        return IsOneOf(raw, {200, 201, 202, 203});
    }

    bool ExpectedLethalDecorContactHazard(int raw)
    {
        return IsOneOf(raw, {201, 202, 203});
    }

    bool ExpectedLevelExit(int raw)
    {
        return IsOneOf(raw, {7, 21});
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
        // Cross-billboard sprites are mostly alpha-0 texels (87-94 % for 53-60): drawn in the
        // opaque pass they show as black cards.
        if (definition.renderMode == BlockRenderMode::TripleCrossBillboard)
        {
            Check(definition.alphaBlend, "cross-billboard blocks draw in the transparent pass");
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
        if (std::find(GalaxyEggbert::kSolidIn3DDecor.begin(), GalaxyEggbert::kSolidIn3DDecor.end(), icon) !=
            GalaxyEggbert::kSolidIn3DDecor.end())
        {
            expectedMask = 0xFFFF; // OR-0069: solid in 3D, a whole block
        }
        Check(definition.collisionMask == expectedMask,
              "registry collision mask matches the source with explicit solid 3D decor");
        // The original's own decision (its empty mask), kept for OR-0069's solid decor too.
        const bool solidIn3D = std::find(GalaxyEggbert::kSolidIn3DDecor.begin(), GalaxyEggbert::kSolidIn3DDecor.end(),
                                         icon) != GalaxyEggbert::kSolidIn3DDecor.end();
        Check(definition.mobileTransparent ==
                  ((expectedMask == 0 || solidIn3D) && icon != BlockTypes::Lava &&
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
    // A switch turns its saws to 379 (Decor::ActiveSwitch): still (only a stored 378 goes
    // through table_decor_scie) and harmless (Decor::IsScie reads 378).
    Check(GetBlockDefinition(BlockTypes::Saw).animation.frameCount == 6 &&
              GetBlockDefinition(382).animation.baseIcon == BlockTypes::Saw,
          "a saw turns through 378-383");
    Check(GetBlockDefinition(BlockTypes::SawStopped).animation.frameCount == 1 &&
              GetBlockDefinition(BlockTypes::SawStopped).animation.frames[0] == BlockTypes::SawStopped &&
              GetBlockDefinition(BlockTypes::SawStopped).renderMode == BlockRenderMode::SawBlade,
          "a switched-off saw is drawn still, as the same plate");
    Check(!BlockTypes::isHazard(BlockTypes::SawStopped) && BlockTypes::isHazard(BlockTypes::Saw),
          "a switched-off saw is harmless");
    {
        bool still = true;
        bool turns = false;
        for (int tick = 0; tick < 12; ++tick)
        {
            still = still && ResolveBlockAnimationIcon(BlockTypes::SawStopped, tick) == BlockTypes::SawStopped;
            turns = turns || ResolveBlockAnimationIcon(BlockTypes::Saw, tick) != BlockTypes::Saw;
        }
        Check(still && turns, "the terrain draws a switched-off saw still and a saw turning");
    }
    {
        // D51: the lightning is drawn exactly when it kills (WorldRuntime::IsBlitzActiveAtPhase's
        // cycle, Decor::BlitzActif): even ticks of the first half of every 100.
        bool drawnWhenDeadly = true;
        int drawn = 0;
        for (int tick = 0; tick < 300; ++tick)
        {
            const int cycle = tick % 100;
            const bool deadly = cycle % 2 == 0 && cycle < 50;
            const int icon = ResolveBlockAnimationIcon(BlockTypes::Blitz, tick);
            drawnWhenDeadly = drawnWhenDeadly && (deadly ? icon >= 305 && icon <= 307 : icon == -1);
            drawn += icon >= 0 ? 1 : 0;
        }
        Check(drawnWhenDeadly && drawn == 75, "the lightning (305) is drawn only while it is deadly");
        // Decor.cpp:1079-1083 picks independently per drawn cell. A coordinate/strike hash keeps
        // that variety reproducible, with repeats allowed (no bags of three).
        bool perCellTiming = true, deterministic = true, columnsDiffer = true, axesDiffer = true;
        bool varies = false, repeats = false;
        int seen = 0;
        for (int tick = 0; tick < 300; ++tick)
        {
            int columnFrames = 0;
            const bool deadly = tick % 100 < 50 && tick % 2 == 0;
            for (int y = 25; y <= 32; ++y)
            {
                const int icon = ResolveBlockAnimationIconAt(BlockTypes::Blitz, tick, -24, y, 0);
                perCellTiming = perCellTiming && (deadly ? icon >= 305 && icon <= 307 : icon == -1);
                deterministic = deterministic &&
                    icon == ResolveBlockAnimationIconAt(BlockTypes::Blitz, tick, -24, y, 0) &&
                    icon == ResolveBlockAnimationIconAt(BlockTypes::Blitz, tick + 100, -24, y, 0, false);
                if (icon >= 305 && icon <= 307) columnFrames |= 1 << (icon - 305);
                if (deadly && tick >= 2)
                {
                    const int previous = ResolveBlockAnimationIconAt(BlockTypes::Blitz, tick - 2, -24, y, 0);
                    repeats = repeats || icon == previous;
                }
            }
            if (!deadly) continue;
            columnsDiffer = columnsDiffer && (columnFrames & (columnFrames - 1)) != 0;
            const int origin = ResolveBlockAnimationIconAt(BlockTypes::Blitz, tick, 0, 0, 0);
            deterministic = deterministic && origin == ResolveBlockAnimationIcon(BlockTypes::Blitz, tick);
            if (origin >= 305 && origin <= 307) seen |= 1 << (origin - 305);
            varies = varies || origin != ResolveBlockAnimationIconAt(BlockTypes::Blitz, 0, 0, 0, 0);
            // All coordinate axes matter, including signed world-space X/Z at the renderer seam.
            for (int axis = 0; axis < 3; ++axis)
            {
                bool differs = false;
                for (int n = -8; n <= 8; ++n)
                    differs = differs || origin != ResolveBlockAnimationIconAt(BlockTypes::Blitz, tick,
                        axis == 0 ? n : 0, axis == 1 ? n : 0, axis == 2 ? n : 0);
                axesDiffer = axesDiffer && differs;
            }
        }
        Check(perCellTiming, "per-cell lightning is invisible on every non-strike tick");
        Check(deterministic, "lightning frames repeat for the same inputs and after the 100-tick cycle");
        Check(columnsDiffer, "several cells in one lightning column differ on each strike");
        Check(varies && seen == 7, "one lightning cell varies across strikes and shows all three frames");
        Check(axesDiffer && repeats, "lightning hashes all three coordinates and allows consecutive repeats");
    }
    Check(GetBlockDefinition(BlockTypes::FanLeft).fanDirection ==
              BlockFanDirection::NegativeX,
          "fan direction is centralized");
    Check(GetBlockDefinition(BlockTypes::Door3).doorKeyType == 51,
          "door key is centralized");
    // Hub signs as Decor::IsWorld() reads them: 158-165, 166-173 and 174-181 -> 1-8,
    // 309/310 -> 9, 411-415 and 416-420 -> 10-14, 184 -> 199; nothing else is a sign.
    {
        bool signsOk = true;
        for (int icon = 0; icon <= BlockDefinitionRegistry::MaxObjectMIcon; ++icon)
        {
            int expected = -1;
            if (icon >= 158 && icon <= 181) expected = (icon - 158) % 8 + 1;
            if (icon == 309 || icon == 310) expected = 9;
            if (icon >= 411 && icon <= 420) expected = (icon - 411) % 5 + 10;
            if (icon == 184) expected = 199;
            if (GetBlockDefinition(static_cast<std::uint16_t>(icon)).worldSelectIndex != expected)
            {
                std::cerr << "sign index mismatch for icon " << icon << '\n';
                signsOk = false;
            }
        }
        Check(signsOk, "hub sign indices are the original's (Decor::IsWorld)");
    }
    Check(ResolveBlockAnimationIcon(BlockTypes::Lava, 0) == 68 &&
              ResolveBlockAnimationIcon(BlockTypes::Lava, 2) == 69 &&
              ResolveBlockAnimationIcon(BlockTypes::Temp, 72) == -1,
          "representative block animation parity");

    // Each cell its own frame (BD-0065, Decor.cpp:1036-1078): lava one cell to the right is 13 frames
    // on (13 % 8 = 5: icon 71 where the cell beside is on 68), one cell up 7 back; the crusher and
    // the fans are all in step; the second water's pace is the cell's own (3, 4 or 5 ticks a frame).
    {
        const int origin = ResolveBlockAnimationIconAt(BlockTypes::Lava, 0, 0, 0, 0);
        const int right = ResolveBlockAnimationIconAt(BlockTypes::Lava, 0, 1, 0, 0);
        const int lavaFrames[8] = {68, 69, 70, 71, 72, 71, 70, 69};
        int originStep = 0;
        // origin's step: find it from the tick-0 icon of a cell whose offset is known relative to it
        for (int step = 0; step < 8; ++step)
        {
            if (lavaFrames[step] == origin && lavaFrames[(step + 13) % 8] == right)
            {
                originStep = step + 1;
                break;
            }
        }
        Check(originStep != 0, "lava: the cell to the right is 13 frames further on its cycle");
        Check(ResolveBlockAnimationIconAt(BlockTypes::Crusher, 9, 0, 0, 0) ==
                  ResolveBlockAnimationIconAt(BlockTypes::Crusher, 9, 5, 3, 2),
              "the crusher has no per-cell offset (all in step, Decor.cpp:1055)");
        std::set<int> water2Paces;
        for (int x = 0; x < 3; ++x)
        {
            // the first tick at which the cell's frame changes from its tick-0 frame
            const int first = ResolveBlockAnimationIconAt(BlockTypes::Water2, 0, x, 0, 0);
            int tick = 1;
            while (tick < 20 && ResolveBlockAnimationIconAt(BlockTypes::Water2, tick, x, 0, 0) == first) ++tick;
            water2Paces.insert(tick);
        }
        Check(water2Paces.size() >= 2, "the second water's cells keep their own pace (3 + (17 x + 13 row) % 3)");
        // Unfocused, the trap snaps: piege2's {374, 373, 347, 373} at 2 ticks a frame (BD-0067).
        std::set<int> snap;
        for (int tick = 0; tick < 8; ++tick)
        {
            snap.insert(ResolveBlockAnimationIconAt(BlockTypes::Spike, tick, 0, 0, 0, /*blupiFocused=*/false));
        }
        Check(snap == std::set<int>{347, 373, 374} &&
                  ResolveBlockAnimationIconAt(BlockTypes::Spike, 0, 0, 0, 0, false) !=
                      ResolveBlockAnimationIconAt(BlockTypes::Spike, 2, 0, 0, 0, false),
              "an unfocused Blupi: the trap snaps (piege2, a frame every 2 ticks)");
    }

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
        Check(definition.dynamiteDestructible == ExpectedDynamiteDestructible(raw),
              "dynamite destructibility is centralized without parity loss");
        Check(definition.genericContactHazard == ExpectedGenericContactHazard(raw),
              "generic contact hazard membership is centralized without parity loss");
        Check(definition.genericContactHazardFeedback ==
                  ExpectedGenericContactHazardFeedback(raw),
              "generic contact hazard feedback is centralized without parity loss");
        Check(definition.genericContactHazardCrouchImmune ==
                  ExpectedGenericContactHazardCrouchImmune(raw),
              "generic contact hazard crouch immunity is centralized without parity loss");
        Check(definition.balloonPoppableHazard == ExpectedBalloonPoppableHazard(raw),
              "balloon-pop hazard membership is centralized without parity loss");
        Check(definition.standardPickupTouch == ExpectedStandardPickupTouch(raw),
              "standard pickup-touch membership is centralized without parity loss");
        Check(definition.smallEnemy == ExpectedSmallEnemy(raw),
              "small-enemy membership is centralized without parity loss");
        Check((definition.semanticKind == ObjectSemanticKind::Avatar) ==
                  ExpectedPersoDecoyAvatarTarget(raw),
              "Perso-decoy avatar target category is centralized without parity loss");
        Check(definition.lethalDecorContactHazard ==
                  ExpectedLethalDecorContactHazard(raw),
              "lethal-decor contact membership is centralized without parity loss");
        Check(definition.levelExit == ExpectedLevelExit(raw),
              "level-exit membership is centralized without parity loss");
        Check(definition.liftConveyorDirection == ExpectedLiftConveyorDirection(raw),
              "lift conveyor direction is centralized without parity loss");

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
    Check(GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType12).semanticKind ==
              ObjectSemanticKind::StaticObstacle &&
              GetObjectDefinition(GalaxyEggbert::Def::ObjectType::ObjectType1).semanticKind ==
              ObjectSemanticKind::Lift,
          "crate and lift capabilities derive from centralized semantic categories");
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

    // The enemy Blupis (OR-0063): blupi1.png is blupi.png with the hue turned 25 degrees.
    {
        using GalaxyEggbert::Game::RecolorEnemyBlupi;
        const auto recolor = [](std::uint8_t r, std::uint8_t g, std::uint8_t b)
        {
            RecolorEnemyBlupi(r, g, b);
            return std::array<int, 3>{r, g, b};
        };
        // His yellow body (60 degrees) to green (85), the cyan helicopter (180) to blue (205).
        Check(recolor(255, 255, 0) == std::array<int, 3>{149, 255, 0}, "enemy Blupi: yellow turns green");
        Check(recolor(0, 255, 255) == std::array<int, 3>{0, 149, 255}, "enemy Blupi: cyan turns blue");
        Check(recolor(128, 128, 128) == std::array<int, 3>{128, 128, 128} &&
                  recolor(255, 255, 255) == std::array<int, 3>{255, 255, 255},
              "enemy Blupi: greys and white stay");

        using GalaxyEggbert::Game::EnemyBlupiClip;
        using GalaxyEggbert::Game::ResolveEnemyBlupiPose;
        // On the way: his helicopter's or tank's drive, facing where he goes.
        const auto flying = ResolveEnemyBlupiPose(false, 2, 5.0f);
        Check(flying.clip == EnemyBlupiClip::MarchHelico && flying.towardEnd == 1.0f &&
                  ResolveEnemyBlupiPose(true, 4, 5.0f).towardEnd == 0.0f &&
                  ResolveEnemyBlupiPose(true, 4, 5.0f).clip == EnemyBlupiClip::MarchTank,
              "enemy Blupi: drives facing his way");
        // The helicopter's turn (table_blupih_turn2*): turned round by tick 10, the glue drop
        // (HelicoGlu) from 12, its bomb at 21 its 10th frame.
        Check(ResolveEnemyBlupiPose(false, 1, 0.0f).towardEnd < 0.2f &&
                  ResolveEnemyBlupiPose(false, 1, 10.0f).towardEnd == 1.0f &&
                  ResolveEnemyBlupiPose(false, 3, 10.0f).towardEnd == 0.0f,
              "enemy Blupi: the helicopter turns round in 10 ticks");
        const auto drop = ResolveEnemyBlupiPose(false, 1, 21.0f);
        Check(ResolveEnemyBlupiPose(false, 1, 11.0f).clip == EnemyBlupiClip::MarchHelico &&
                  drop.clip == EnemyBlupiClip::HelicoGlu && drop.clipTicks == 9.0f,
              "enemy Blupi: the helicopter drops its bomb in HelicoGlu");
        // The tank (table_blupit_turn2*): a shot the way he came (3), the turn (6-17), a shot the
        // new way (21).
        const auto firstShot = ResolveEnemyBlupiPose(true, 1, 3.0f);
        const auto secondShot = ResolveEnemyBlupiPose(true, 1, 21.0f);
        Check(firstShot.clip == EnemyBlupiClip::FireTank && firstShot.clipTicks == 1.0f &&
                  firstShot.towardEnd == 0.0f && secondShot.clip == EnemyBlupiClip::FireTank &&
                  secondShot.towardEnd == 1.0f && ResolveEnemyBlupiPose(true, 1, 11.5f).towardEnd == 0.5f &&
                  ResolveEnemyBlupiPose(true, 1, 11.5f).clip == EnemyBlupiClip::MarchTank,
              "enemy Blupi: the tank fires, turns, fires");
    }

    for (int icon=1; icon<=GalaxyEggbert::BlockDefinitionRegistry::MaxObjectMIcon; ++icon)
    {
        const auto& d=GalaxyEggbert::GetBlockDefinition(icon);
        Check(d.secretPassage==IsOneOf(icon,{22,73,85,86,339}),"xray secret families are classified in the block registry");
        if (d.secretPassage) Check(d.collisionMask==0 && d.decor,"secret passage stays passable decor");
    }
    if (failures != 0)
    {
        std::cerr << failures << " definition-registry check(s) failed.\n";
        return 1;
    }
    std::cout << "VerifyDefinitionRegistries: all checks passed.\n";
    return 0;
}
