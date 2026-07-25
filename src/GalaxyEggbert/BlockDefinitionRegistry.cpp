#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>

#include <GalaxyEggbert/BlockTypes.hpp>

#include "Game/DecorQuartTable.hpp"

#include <algorithm>
#include <initializer_list>

namespace GalaxyEggbert
{
    namespace
    {
        using namespace BlockTypes;

        template<std::size_t N>
        void SetAnimation(BlockDefinition& definition, std::uint16_t base,
                          const int (&frames)[N], int divisor)
        {
            static_assert(N <= BlockAnimationDefinition::MaxFrames);
            definition.animation.baseIcon = base;
            definition.animation.frameCount = static_cast<std::uint8_t>(N);
            definition.animation.divisor = static_cast<std::uint8_t>(divisor);
            for (std::size_t i = 0; i < N; ++i)
            {
                definition.animation.frames[i] = static_cast<std::int16_t>(frames[i]);
            }
        }

        void SetRenderMode(std::array<BlockDefinition, BlockDefinitionRegistry::DefinitionCount>& definitions,
                           BlockRenderMode mode, std::initializer_list<int> icons)
        {
            for (const int icon : icons)
            {
                definitions[static_cast<std::size_t>(icon)].renderMode = mode;
            }
        }
    }

    const BlockDefinitionRegistry& BlockDefinitionRegistry::Instance() noexcept
    {
        static const BlockDefinitionRegistry registry;
        return registry;
    }

    BlockDefinitionRegistry::BlockDefinitionRegistry() noexcept
    {
        unsupported_.renderMode = BlockRenderMode::Air;
        unsupported_.textureSource = BlockTextureSource::None;
        unsupported_.mobileTransparent = true;

        for (std::size_t icon = 0; icon < definitions_.size(); ++icon)
        {
            auto& definition = definitions_[icon];
            definition.type = static_cast<std::uint16_t>(icon);
            definition.textureSource = icon == 0 ? BlockTextureSource::None
                                                 : BlockTextureSource::ObjectM;
            definition.textureIcon = static_cast<std::uint16_t>(icon);
            definition.renderMode = icon == 0 ? BlockRenderMode::Air
                                              : BlockRenderMode::UniformCube;
            definition.animation.baseIcon = static_cast<std::uint16_t>(icon);

            if (icon != 0)
            {
                std::uint16_t mask = 0;
                for (std::size_t cell = 0; cell < 16; ++cell)
                {
                    if (Game::kDecorQuartTable[icon * 16 + cell] != 0)
                    {
                        mask |= static_cast<std::uint16_t>(1u << cell);
                    }
                }
                definition.collisionMask = mask;
                definition.mobileTransparent =
                    mask == 0 && icon != BlockTypes::Lava && icon != BlockTypes::Crusher;
            }
        }

        // Central render-mode classification. Per-face mesh construction
        // remains in the focused renderer helpers, but no renderer chooses a
        // mode by maintaining its own parallel icon list.
        SetRenderMode(definitions_, BlockRenderMode::DirectionalCube, {
            2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 44, 45, 46, 47, 48, 49,
            50, 51, 52, 66, 74, 75, 86, 87, 88, 89, 90, 107, 108, 109, 126,
            129, 132, 135, 154, 155, 186, 187, 188, 189, 190, 191, 192, 193,
            194, 195, 196, 197, 200, 224, 225, 226, 227, 228, 232, 245, 250,
            251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 283, 330, 331,
            332, 333, 364, 365, 366, 390, 391, 392, 393, 394, 395, 396, 397,
            400,
        });
        SetRenderMode(definitions_, BlockRenderMode::InnerPillarBox, {76, 384, 385});
        SetRenderMode(definitions_, BlockRenderMode::ThinBar, {202});
        SetRenderMode(definitions_, BlockRenderMode::InnerFlatPlate, {
            77, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
            122, 123, 124, 125, 138, 199, 264, 265, 266, 267, 268, 269, 270,
            271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 285,
            286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298,
            299, 300, 302, 303, 367, 368, 369, 370, 371, 372, 398,
        });
        SetRenderMode(definitions_, BlockRenderMode::TripleCrossBillboard,
                      {53, 54, 55, 56, 57, 58, 59, 60, 63, 64});
        SetRenderMode(definitions_, BlockRenderMode::GroundAnchoredPlate,
                      {BlockTypes::Saw, BlockTypes::SawStopped});

        for (const int icon : {30, 31, 202, 330, 331, 332, 333})
        {
            definitions_[static_cast<std::size_t>(icon)].alphaBlend = true;
        }
        for (const int icon : {107, 108, 109})
        {
            definitions_[static_cast<std::size_t>(icon)].grassTop = true;
        }
        for (const int icon : {330, 331, 332, 333})
        {
            definitions_[static_cast<std::size_t>(icon)].teleporterTip = true;
        }

        constexpr int lavaFrames[8] = {68, 69, 70, 71, 72, 71, 70, 69};
        constexpr int spikeFrames[16] = {
            374, 374, 373, 347, 373, 374, 374, 374,
            373, 347, 347, 373, 374, 374, 374, 374,
        };
        constexpr int crusherFrames[10] = {317, 317, 318, 319, 320, 321, 322, 323, 323, 323};
        constexpr int sawFrames[6] = {378, 379, 380, 381, 382, 383};
        constexpr int water1Frames[6] = {92, 93, 94, 95, 94, 93};
        constexpr int water2Frames[6] = {91, 96, 97, 98, 97, 96};
        constexpr int fanLeftFrames[3] = {126, 127, 128};
        constexpr int fanRightFrames[3] = {129, 130, 131};
        constexpr int fanUpFrames[3] = {132, 133, 134};
        constexpr int fanDownFrames[3] = {135, 136, 137};
        constexpr int tempFrames[20] = {
            328, 328, 327, 327, 326, 326, 325, 325, 324, 324,
            325, 325, 326, 326, 327, 329, 328, 328, -1, -1,
        };
        constexpr int marineFrames[11] = {203, 204, 205, 206, 207, 208, 207, 206, 205, 204, 203};

        SetAnimation(definitions_[Lava], Lava, lavaFrames, 2);
        SetAnimation(definitions_[Spike], Spike, spikeFrames, 4);
        SetAnimation(definitions_[Crusher], Crusher, crusherFrames, 3);
        SetAnimation(definitions_[Saw], Saw, sawFrames, 1);
        SetAnimation(definitions_[SawStopped], Saw, sawFrames, 1);
        SetAnimation(definitions_[Water1], Water1, water1Frames, 3);
        SetAnimation(definitions_[Water2], Water2, water2Frames, 3);
        SetAnimation(definitions_[FanLeft], FanLeft, fanLeftFrames, 3);
        SetAnimation(definitions_[FanRight], FanRight, fanRightFrames, 3);
        SetAnimation(definitions_[FanUp], FanUp, fanUpFrames, 3);
        SetAnimation(definitions_[FanDown], FanDown, fanDownFrames, 3);
        SetAnimation(definitions_[Temp], Temp, tempFrames, 4);
        SetAnimation(definitions_[Marine], Marine, marineFrames, 3);

        // Any non-base frame stored in an imported world resolves through
        // the same base definition as its animation family.
        for (std::uint16_t icon = 1; icon <= MaxObjectMIcon; ++icon)
        {
            std::uint16_t base = icon;
            if (icon >= 68 && icon <= 72) base = Lava;
            else if (icon == 347 || icon == 373 || icon == 374) base = Spike;
            else if (icon >= 317 && icon <= 323) base = Crusher;
            else if (icon >= 378 && icon <= 383) base = Saw;
            else if (icon >= 92 && icon <= 95) base = Water1;
            else if (icon == 91 || (icon >= 96 && icon <= 98)) base = Water2;
            else if (icon >= 126 && icon <= 128) base = FanLeft;
            else if (icon >= 129 && icon <= 131) base = FanRight;
            else if (icon >= 132 && icon <= 134) base = FanUp;
            else if (icon >= 135 && icon <= 137) base = FanDown;
            else if (icon >= 324 && icon <= 329) base = Temp;
            else if (icon >= 203 && icon <= 208) base = Marine;
            if (base != icon)
            {
                definitions_[icon].animation = definitions_[base].animation;
                definitions_[icon].renderMode = definitions_[base].renderMode;
            }
        }

        definitions_[Water1].water = true;
        definitions_[Water2].water = true;
        for (const int icon : {91, 92, 93, 94, 95, 96, 97, 98})
        {
            definitions_[static_cast<std::size_t>(icon)].water = true;
        }

        definitions_[Lava].hazard = BlockHazardKind::Lava;
        for (int icon = 69; icon <= 72; ++icon)
        {
            definitions_[static_cast<std::size_t>(icon)].hazard = BlockHazardKind::Lava;
        }
        definitions_[Spike].hazard = BlockHazardKind::Spike;
        definitions_[347].hazard = BlockHazardKind::Spike;
        definitions_[374].hazard = BlockHazardKind::Spike;
        definitions_[Crusher].hazard = BlockHazardKind::Crusher;
        for (int icon = 318; icon <= 323; ++icon)
        {
            definitions_[static_cast<std::size_t>(icon)].hazard = BlockHazardKind::Crusher;
        }
        definitions_[Saw].hazard = BlockHazardKind::Saw;
        for (int icon = 379; icon <= 383; ++icon)
        {
            definitions_[static_cast<std::size_t>(icon)].hazard = BlockHazardKind::Saw;
        }
        definitions_[Blitz].hazard = BlockHazardKind::Blitz;
        definitions_[Drip].hazard = BlockHazardKind::Drip;

        definitions_[FanLeft].hazard = BlockHazardKind::Fan;
        definitions_[FanLeft].fanDirection = BlockFanDirection::NegativeX;
        definitions_[FanRight].hazard = BlockHazardKind::Fan;
        definitions_[FanRight].fanDirection = BlockFanDirection::PositiveX;
        definitions_[FanUp].hazard = BlockHazardKind::Fan;
        definitions_[FanUp].fanDirection = BlockFanDirection::NegativeZ;
        definitions_[FanDown].hazard = BlockHazardKind::Fan;
        definitions_[FanDown].fanDirection = BlockFanDirection::PositiveZ;

        definitions_[Spring].spring = true;
        definitions_[Temp].temporary = true;
        definitions_[Bridge].bridge = true;
        definitions_[Switch].switchBlock = true;
        definitions_[SwitchOff].switchBlock = true;

        definitions_[Door1].doorKeyType = 49;
        definitions_[Door2].doorKeyType = 50;
        definitions_[Door3].doorKeyType = 51;

        for (int icon = Teleport1; icon <= Teleport4; ++icon)
        {
            definitions_[static_cast<std::size_t>(icon)].teleporterIndex =
                static_cast<std::int8_t>(icon - Teleport1 + 1);
        }
        for (int icon = WorldSelect1; icon <= WorldSelect12; ++icon)
        {
            definitions_[static_cast<std::size_t>(icon)].worldSelectIndex =
                static_cast<std::int8_t>(icon - WorldSelect1 + 1);
        }
        for (int icon = ProgressDoor2; icon <= ProgressDoor8; ++icon)
        {
            definitions_[static_cast<std::size_t>(icon)].progressDoorIndex =
                static_cast<std::int8_t>(icon - ProgressDoor2 + 2);
        }
    }

    const BlockDefinition& BlockDefinitionRegistry::Get(std::uint16_t type) const noexcept
    {
        return IsSupported(type) ? definitions_[type] : unsupported_;
    }

    bool BlockDefinitionRegistry::IsSupported(std::uint16_t type) const noexcept
    {
        return type <= MaxObjectMIcon;
    }

    int BlockDefinitionRegistry::ResolveAnimationIcon(std::uint16_t type, int rawTick) const noexcept
    {
        const auto& animation = Get(type).animation;
        if (!animation.IsAnimated())
        {
            return static_cast<int>(Get(type).textureIcon);
        }
        const int nonNegativeTick = std::max(0, rawTick);
        const int phase = nonNegativeTick / std::max(1, static_cast<int>(animation.divisor));
        return animation.frames[static_cast<std::size_t>(phase % animation.frameCount)];
    }
}
