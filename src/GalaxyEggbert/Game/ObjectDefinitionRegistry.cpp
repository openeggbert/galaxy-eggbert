#include "ObjectDefinitionRegistry.hpp"

#include "ObjectIcons.hpp"

#include <initializer_list>

namespace GalaxyEggbert::Game
{
    namespace
    {
        using GalaxyEggbert::Def::ObjectType;

        template<typename Setter>
        void ForTypes(std::array<ObjectDefinition, ObjectDefinitionRegistry::DefinitionCount>& definitions,
                      std::initializer_list<int> types, Setter&& setter)
        {
            for (const int raw : types)
            {
                setter(definitions[static_cast<std::size_t>(raw)]);
            }
        }
    }

    const ObjectDefinitionRegistry& ObjectDefinitionRegistry::Instance() noexcept
    {
        static const ObjectDefinitionRegistry registry;
        return registry;
    }

    ObjectDefinitionRegistry::ObjectDefinitionRegistry() noexcept
    {
        unsupported_.renderMode = ObjectRenderMode::Hidden;
        unsupported_.textureSource = ObjectTextureSource::None;
        unsupported_.placementKind = ObjectPlacementKind::Reserved;
        unsupported_.semanticKind = ObjectSemanticKind::Reserved;

        for (std::size_t raw = 0; raw < definitions_.size(); ++raw)
        {
            definitions_[raw].type = static_cast<ObjectType>(raw);
        }

        auto& nullDefinition = definitions_[0];
        nullDefinition.renderMode = ObjectRenderMode::Hidden;
        nullDefinition.textureSource = ObjectTextureSource::None;
        nullDefinition.placementKind = ObjectPlacementKind::Null;
        nullDefinition.semanticKind = ObjectSemanticKind::Null;

        ForTypes(definitions_, {
            1, 2, 3, 4, 5, 6, 7, 12, 13, 16, 17, 18, 19, 20, 21, 24, 25, 26,
            28, 29, 30, 31, 32, 33, 40, 44, 46, 47, 48, 49, 50, 51, 54, 55,
            96, 97, 200, 201, 202, 203,
        }, [](ObjectDefinition& definition)
        {
            definition.placementKind = ObjectPlacementKind::Direct;
        });

        // Types encountered in the 78 reference mobile-eggbert worlds.
        // This is deliberately separate from editor placeability: the
        // editor also exposes confirmed types not present in those files.
        ForTypes(definitions_, {
            1, 2, 3, 4, 5, 6, 7, 12, 13, 16, 17, 19, 20, 21, 24, 25, 26, 30,
            32, 33, 40, 44, 46, 47, 49, 50, 51, 54, 55, 96,
        }, [](ObjectDefinition& definition)
        {
            definition.mobileWorldSupported = true;
        });

        ForTypes(definitions_, {2, 3, 4, 20, 32, 33, 44, 54},
                 [](ObjectDefinition& definition)
        {
            definition.patrolMotion = true;
        });

        // Declarative runtime capabilities. These are intentionally not a
        // general behavior-dispatch table: they only replace exact membership
        // checks whose members receive identical treatment at their call site.
        ForTypes(definitions_, {
            2, 3, 4, 6, 12, 13, 16, 17, 18, 19, 20, 24, 25, 26, 28, 30,
            32, 33, 34, 40, 44, 46, 52, 54, 96, 97, 200, 201, 202, 203,
        }, [](ObjectDefinition& definition)
        {
            definition.dynamiteDestructible = true;
        });
        ForTypes(definitions_, {2, 3, 4, 16, 17, 20, 96, 97},
                 [](ObjectDefinition& definition)
        {
            definition.genericContactHazard = true;
        });
        ForTypes(definitions_, {17, 20}, [](ObjectDefinition& definition)
        {
            definition.genericContactHazardFeedback =
                GenericContactHazardFeedback::Big;
        });
        definitions_[3].genericContactHazardCrouchImmune = true;
        ForTypes(definitions_, {3, 16, 96, 97}, [](ObjectDefinition& definition)
        {
            definition.balloonPoppableHazard = true;
        });
        ForTypes(definitions_, {5, 6, 7, 21, 25, 26, 29, 30, 31, 40, 49, 50,
                                51, 55}, [](ObjectDefinition& definition)
        {
            definition.standardPickupTouch = true;
        });
        ForTypes(definitions_, {4, 32, 33}, [](ObjectDefinition& definition)
        {
            definition.smallEnemy = true;
        });
        ForTypes(definitions_, {201, 202, 203}, [](ObjectDefinition& definition)
        {
            definition.lethalDecorContactHazard = true;
        });
        ForTypes(definitions_, {7, 21}, [](ObjectDefinition& definition)
        {
            definition.levelExit = true;
        });

        ForTypes(definitions_, {
            8, 9, 10, 11, 14, 15, 22, 23, 27, 34, 35, 36, 37, 38, 39, 41,
            42, 52, 53, 56, 57, 58, 90, 91, 92, 93, 98, 99, 100,
        }, [](ObjectDefinition& definition)
        {
            definition.placementKind = ObjectPlacementKind::Transient;
        });

        ForTypes(definitions_, {1, 47, 48}, [](ObjectDefinition& definition)
        {
            definition.semanticKind = ObjectSemanticKind::Lift;
        });
        definitions_[47].liftConveyorDirection = LiftConveyorDirection::PositiveX;
        definitions_[48].liftConveyorDirection = LiftConveyorDirection::NegativeX;
        ForTypes(definitions_, {2, 3, 4, 16, 17, 18, 20, 32, 33, 44, 54, 96, 97},
                 [](ObjectDefinition& definition)
        {
            definition.semanticKind = ObjectSemanticKind::Enemy;
        });
        ForTypes(definitions_, {5, 6, 7, 21, 49, 50, 51},
                 [](ObjectDefinition& definition)
        {
            definition.semanticKind = ObjectSemanticKind::Collectible;
        });
        ForTypes(definitions_, {13, 19, 24, 25, 26, 28, 29, 30, 31, 40, 46, 55},
                 [](ObjectDefinition& definition)
        {
            definition.semanticKind = ObjectSemanticKind::Pickup;
        });
        definitions_[12].semanticKind = ObjectSemanticKind::StaticObstacle;
        definitions_[23].semanticKind = ObjectSemanticKind::Projectile;
        ForTypes(definitions_, {
            8, 9, 10, 11, 14, 15, 34, 35, 36, 37, 38, 39, 41, 42, 53, 57,
            58, 90, 91, 92, 93, 98, 99, 100,
        }, [](ObjectDefinition& definition)
        {
            definition.semanticKind = ObjectSemanticKind::Effect;
        });
        ForTypes(definitions_, {22, 27, 52, 56}, [](ObjectDefinition& definition)
        {
            definition.semanticKind = ObjectSemanticKind::MovingLevelObject;
        });
        ForTypes(definitions_, {200, 201, 202, 203}, [](ObjectDefinition& definition)
        {
            definition.semanticKind = ObjectSemanticKind::Avatar;
        });

        ForTypes(definitions_, {1, 12, 47, 48}, [](ObjectDefinition& definition)
        {
            definition.renderMode = ObjectRenderMode::SolidCube;
            definition.textureSource = ObjectTextureSource::ObjectM;
        });

        ForTypes(definitions_, {14, 15, 31, 35, 52}, [](ObjectDefinition& definition)
        {
            definition.textureSource = ObjectTextureSource::ObjectM;
        });

        ForTypes(definitions_, {8, 9, 10, 11, 53, 90, 91, 92, 93, 98, 99, 100},
                 [](ObjectDefinition& definition)
        {
            definition.textureSource = ObjectTextureSource::Explo;
        });

        definitions_[200].textureSource = ObjectTextureSource::Blupi;
        ForTypes(definitions_, {201, 202, 203}, [](ObjectDefinition& definition)
        {
            definition.textureSource = ObjectTextureSource::Blupi1;
        });
        definitions_[38].textureSource = ObjectTextureSource::PhaseDependent;
    }

    const ObjectDefinition& ObjectDefinitionRegistry::Get(ObjectType type) const noexcept
    {
        return IsSupported(type)
            ? definitions_[static_cast<std::uint8_t>(type)]
            : unsupported_;
    }

    bool ObjectDefinitionRegistry::IsSupported(ObjectType type) const noexcept
    {
        return static_cast<std::uint16_t>(static_cast<std::uint8_t>(type)) <= MaxObjectType;
    }

    ResolvedObjectVisual ObjectDefinitionRegistry::Resolve(
        ObjectType type, int phase, std::uint16_t visualIconOverride) const noexcept
    {
        const auto& definition = Get(type);
        ResolvedObjectVisual visual;
        visual.definition = &definition;
        visual.renderMode = definition.renderMode;
        visual.textureSource = definition.textureSource;
        visual.icon = visualIconOverride != 0
            ? static_cast<int>(visualIconOverride)
            : ResolveObjectIconFrame(type, phase);

        if (visualIconOverride != 0)
        {
            visual.textureSource = ObjectTextureSource::ObjectM;
        }
        else if (definition.textureSource == ObjectTextureSource::PhaseDependent)
        {
            const int normalizedPhase = ((phase % 90) + 90) % 90;
            visual.textureSource = normalizedPhase < 30
                ? ObjectTextureSource::Blupi1
                : ObjectTextureSource::Element;
        }

        visual.visible = visual.renderMode != ObjectRenderMode::Hidden && visual.icon >= 0;
        return visual;
    }
}
