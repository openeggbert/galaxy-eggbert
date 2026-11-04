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
