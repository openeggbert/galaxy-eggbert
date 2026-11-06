#pragma once

#include <array>
#include <cstdint>

namespace GalaxyEggbert
{
    enum class BlockTextureSource : std::uint8_t
    {
        None,
        ObjectM,
    };

    enum class BlockRenderMode : std::uint8_t
    {
        Air,
        UniformCube,
        DirectionalCube,
        InnerPillarBox,
        ThinBar,
        InnerFlatPlate,
        TripleCrossBillboard,
        GroundAnchoredPlate,
    };

    enum class BlockHazardKind : std::uint8_t
    {
        None,
        Lava,
        Spike,
        Crusher,
        Saw,
        Blitz,
        Drip,
        Fan,
    };

    enum class BlockFanDirection : std::uint8_t
    {
        None,
        NegativeX,
        PositiveX,
        NegativeZ,
        PositiveZ,
    };

    struct BlockAnimationDefinition final
    {
        static constexpr std::size_t MaxFrames = 100; // the lightning's whole cycle (Decor::BlitzActif)

        std::uint16_t baseIcon = 0;
        std::array<std::int16_t, MaxFrames> frames{};
        std::uint8_t frameCount = 0;
        std::uint8_t divisor = 1;

        [[nodiscard]] constexpr bool IsAnimated() const noexcept
        {
            return frameCount != 0;
        }
    };

    // One authoritative runtime definition for a voxel block type. The
    // structure deliberately contains no CNA or cna-engine types, so world,
    // gameplay, editor, renderer and verification code can share it.
    struct BlockDefinition final
    {
        std::uint16_t type = 0;
        BlockTextureSource textureSource = BlockTextureSource::None;
        std::uint16_t textureIcon = 0;
        BlockRenderMode renderMode = BlockRenderMode::Air;
        BlockAnimationDefinition animation{};
        std::uint16_t collisionMask = 0;

        bool alphaBlend = false;
        bool mobileTransparent = true;
        bool water = false;
        bool spring = false;
        bool temporary = false;
        bool bridge = false;
        bool switchBlock = false;
        bool grassTop = false;
        bool teleporterTip = false;
        // Decor Blupi passes through, as the original's empty quarter-cell mask makes it (D41):
        // no collision, no hazard, and none of the roles the game gives a tile under or in front
        // of him (water, springs, bridges, doors, teleporters, fans, wind, bars, switches, the saw
        // plates, the Temp saucers). Neither a floor nor a ceiling to him.
        bool decor = false;

        BlockHazardKind hazard = BlockHazardKind::None;
        BlockFanDirection fanDirection = BlockFanDirection::None;
        std::int8_t doorKeyType = -1;
        std::int8_t teleporterIndex = -1;
        std::int16_t worldSelectIndex = -1; // Decor::IsWorld(): 1-14, 199 for the return sign
    };

    // Decor the original's Blupi walks in front of in 2D (an empty quarter mask) that stands in his
    // way in 3D, a whole block each (OR-0069, owner 2026-11-04): the columns (76, the toys' striped
    // posts 235/236), the girder (201), the machine pieces (49, 50, 51), the golden support (77),
    // the trestle (199), the posts (200), the slime column (300), the balustrade (398). Not the
    // original's secret passages, walls one walks through by design (the editor's "secret"
    // families: the tech-blocks 22/73, the machine pieces 85/86; the cheese 339), nor the cobwebs,
    // bolts, knobs, lumps, the drip, the arches and the signs.
    inline constexpr std::array<std::uint16_t, 12> kSolidIn3DDecor{
        49, 50, 51, 76, 77, 199, 200, 201, 235, 236, 300, 398};

    // The BigDecor (explo.png pictures standing in the rooms as their models) that stand in his way
    // in 3D (Q-014, owner 2026-11-05, MR-0007/MR-0008): the palms 16-19, the trees 20-22, the plants
    // 23-25, the houses 26-27, the machines 28-31, the toys 36-47, the palace pieces 87-88 and the Blupi
    // statue 89 -- as their models' shapes (WorldRuntime::SolidBigDecorBoxes); without a measured shape
    // a column of kSolidBigDecorHeight cells over the anchor (the 128 px picture: 2 blocks). The
    // slime 66-69 and 203 stay passable.
    inline constexpr std::array<std::uint16_t, 31> kSolidBigDecorIcons{
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 87, 88, 89};
    inline constexpr int kSolidBigDecorHeight = 2;
    [[nodiscard]] inline constexpr bool IsSolidBigDecorIcon(std::uint16_t icon) noexcept
    {
        for (const auto solid : kSolidBigDecorIcons)
        {
            if (solid == icon)
            {
                return true;
            }
        }
        return false;
    }

    class BlockDefinitionRegistry final
    {
    public:
        static constexpr std::uint16_t MaxObjectMIcon = 440;
        static constexpr std::size_t DefinitionCount =
            static_cast<std::size_t>(MaxObjectMIcon) + 1;

        [[nodiscard]] static const BlockDefinitionRegistry& Instance() noexcept;
        [[nodiscard]] const BlockDefinition& Get(std::uint16_t type) const noexcept;
        [[nodiscard]] bool IsSupported(std::uint16_t type) const noexcept;
        [[nodiscard]] int ResolveAnimationIcon(std::uint16_t type, int rawTick) const noexcept;
        // The frame of the tile in cell (x, y, z) (world cells, y up): lava, the spike trap, the
        // drip, the saw and the first water start each cell 13 x + 7 row frames on, the second water
        // 11 x + 7 row on and at its own pace (a divisor of 3 + (17 x + 13 row) % 3), as the
        // original draws them (Decor.cpp:1069-1078, 1032-1043, 1055-1061; row = its j, down);
        // depth adds 11 z (or 13 z) -- Galaxy's own, so cells side by side in depth differ too
        // (BD-0065). Every other tile as ResolveAnimationIcon.
        // The spike trap snaps (table_decor_piege2, 4 frames at 2 ticks) while Blupi is not focused --
        // dying, taking a pickup, teleporting... (Decor.cpp:1032-1037; BD-0067).
        [[nodiscard]] int ResolveAnimationIconAt(std::uint16_t type, int rawTick, int x, int y, int z,
                                                 bool blupiFocused = true) const noexcept;

    private:
        BlockDefinitionRegistry() noexcept;

        std::array<BlockDefinition, DefinitionCount> definitions_{};
        BlockDefinition unsupported_{};
    };

    [[nodiscard]] inline const BlockDefinition& GetBlockDefinition(std::uint16_t type) noexcept
    {
        return BlockDefinitionRegistry::Instance().Get(type);
    }

    [[nodiscard]] inline bool IsSupportedBlockType(std::uint16_t type) noexcept
    {
        return BlockDefinitionRegistry::Instance().IsSupported(type);
    }

    [[nodiscard]] inline int ResolveBlockAnimationIcon(std::uint16_t type, int rawTick) noexcept
    {
        return BlockDefinitionRegistry::Instance().ResolveAnimationIcon(type, rawTick);
    }

    [[nodiscard]] inline int ResolveBlockAnimationIconAt(std::uint16_t type, int rawTick, int x, int y, int z,
                                                         bool blupiFocused = true) noexcept
    {
        return BlockDefinitionRegistry::Instance().ResolveAnimationIconAt(type, rawTick, x, y, z, blupiFocused);
    }
}
