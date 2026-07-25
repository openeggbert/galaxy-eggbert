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
        static constexpr std::size_t MaxFrames = 20;

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
    // structure deliberately contains no CNA or Easy3D types, so world,
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

        BlockHazardKind hazard = BlockHazardKind::None;
        BlockFanDirection fanDirection = BlockFanDirection::None;
        std::int8_t doorKeyType = -1;
        std::int8_t teleporterIndex = -1;
        std::int8_t worldSelectIndex = -1;
        std::int8_t progressDoorIndex = -1;
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
}
