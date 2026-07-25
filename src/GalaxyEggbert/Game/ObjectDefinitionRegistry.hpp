#pragma once

#include <GalaxyEggbert/Def/ObjectType.hpp>

#include <array>
#include <cstdint>

namespace GalaxyEggbert::Game
{
    enum class ObjectRenderMode : std::uint8_t
    {
        Hidden,
        Billboard,
        SolidCube,
    };

    enum class ObjectTextureSource : std::uint8_t
    {
        None,
        Element,
        ObjectM,
        Explo,
        Blupi,
        Blupi1,
        PhaseDependent,
    };

    enum class ObjectPlacementKind : std::uint8_t
    {
        Null,
        Direct,
        Transient,
        Reserved,
    };

    enum class ObjectSemanticKind : std::uint8_t
    {
        Null,
        Lift,
        Enemy,
        Collectible,
        Pickup,
        StaticObstacle,
        Projectile,
        Effect,
        MovingLevelObject,
        Avatar,
        Reserved,
    };

    enum class ObjectVerticalPlacement : std::uint8_t
    {
        CellCenter,
    };

    struct ObjectDefinition final
    {
        GalaxyEggbert::Def::ObjectType type =
            GalaxyEggbert::Def::ObjectType::ObjectType0;
        ObjectRenderMode renderMode = ObjectRenderMode::Billboard;
        ObjectTextureSource textureSource = ObjectTextureSource::Element;
        ObjectPlacementKind placementKind = ObjectPlacementKind::Reserved;
        ObjectSemanticKind semanticKind = ObjectSemanticKind::Reserved;
        ObjectVerticalPlacement verticalPlacement =
            ObjectVerticalPlacement::CellCenter;
        bool mobileWorldSupported = false;
        bool patrolMotion = false;
    };

    struct ResolvedObjectVisual final
    {
        const ObjectDefinition* definition = nullptr;
        ObjectRenderMode renderMode = ObjectRenderMode::Hidden;
        ObjectTextureSource textureSource = ObjectTextureSource::None;
        int icon = -1;
        bool visible = false;
    };

    class ObjectDefinitionRegistry final
    {
    public:
        static constexpr std::uint16_t MaxObjectType = 203;
        static constexpr std::size_t DefinitionCount =
            static_cast<std::size_t>(MaxObjectType) + 1;

        [[nodiscard]] static const ObjectDefinitionRegistry& Instance() noexcept;
        [[nodiscard]] const ObjectDefinition& Get(
            GalaxyEggbert::Def::ObjectType type) const noexcept;
        [[nodiscard]] bool IsSupported(
            GalaxyEggbert::Def::ObjectType type) const noexcept;
        [[nodiscard]] ResolvedObjectVisual Resolve(
            GalaxyEggbert::Def::ObjectType type, int phase,
            std::uint16_t visualIconOverride = 0) const noexcept;

    private:
        ObjectDefinitionRegistry() noexcept;

        std::array<ObjectDefinition, DefinitionCount> definitions_{};
        ObjectDefinition unsupported_{};
    };

    [[nodiscard]] inline const ObjectDefinition& GetObjectDefinition(
        GalaxyEggbert::Def::ObjectType type) noexcept
    {
        return ObjectDefinitionRegistry::Instance().Get(type);
    }

    [[nodiscard]] inline bool IsSupportedObjectType(
        GalaxyEggbert::Def::ObjectType type) noexcept
    {
        return ObjectDefinitionRegistry::Instance().IsSupported(type);
    }

    [[nodiscard]] inline ResolvedObjectVisual ResolveObjectVisual(
        GalaxyEggbert::Def::ObjectType type, int phase,
        std::uint16_t visualIconOverride = 0) noexcept
    {
        return ObjectDefinitionRegistry::Instance().Resolve(
            type, phase, visualIconOverride);
    }

    [[nodiscard]] inline float ResolveObjectVisualCenterY(
        GalaxyEggbert::Def::ObjectType type, float objectCenterY) noexcept
    {
        // Kept as an explicit registry-backed policy even though every
        // current type uses CellCenter. New vertical policies gain one
        // authoritative dispatch point instead of renderer-side offsets.
        switch (GetObjectDefinition(type).verticalPlacement)
        {
            case ObjectVerticalPlacement::CellCenter:
            default:
                return objectCenterY;
        }
    }
}
