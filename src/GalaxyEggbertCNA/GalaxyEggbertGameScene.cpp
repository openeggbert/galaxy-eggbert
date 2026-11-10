// GalaxyEggbertGame's SceneFrame (SceneFrame.hpp): what the 3D world draws this frame, read from the
// simulation -- which objects show, by which icon or model in which pose, where and turned how. The
// WorldRenderer draws it (WorldRenderer.hpp); nothing here touches the device's state.

#include "GalaxyEggbertGame.hpp"
#include "GameInternal.hpp"
#include "NetGame.hpp"

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Editor/PlayerStartColor.hpp>
#include <GalaxyEggbert/Game/BarRows.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/BoltMesh.hpp>
#include <GalaxyEggbert/Game/DirectionalCubeTiles.hpp>
#include <GalaxyEggbert/Game/EffectParticles.hpp>
#include <GalaxyEggbert/Game/SceneShaders.hpp>
#include <GalaxyEggbert/Game/SceneEffectsLogic.hpp>
#include <GalaxyEggbert/Game/TerrainLighting.hpp>
#include <GalaxyEggbert/Game/EnemyBlupi.hpp>
#include <GalaxyEggbert/Game/GlueStrands.hpp>
#include <GalaxyEggbert/Game/LiftVisual.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <GalaxyEggbert/Game/ObjectVerticalPlacement.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>
#include <GalaxyEggbert/Game/WaterSurface.hpp>

#include <Microsoft/Xna/Framework/Color.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::CNA::detail;

    namespace
    {
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        using Microsoft::Xna::Framework::Graphics::SkinnedModelEXT;
        using Microsoft::Xna::Framework::Graphics::Texture2D;

        constexpr float kPi = 3.14159265358979f;

        CnaEngine::Graphics::UvRect Uv(const ObjectIconUv& uv)
        {
            return CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1};
        }

        SceneQuad Quad(const Vector3& center, const Vector2& size, const Vector3& right, const Vector3& up,
                       const CnaEngine::Graphics::UvRect& uv, int sourceObject = -1, int sourceIcon = -1)
        {
            SceneQuad quad;
            quad.center = center;
            quad.size = size;
            quad.right = right;
            quad.up = up;
            quad.uv = uv;
            quad.sourceObject = sourceObject;
            quad.sourceIcon = sourceIcon;
            return quad;
        }

        // Each part's texture: `face` on the face part, else `other(part texture)` (nullptr keeps it).
        template <typename Other>
        std::vector<Texture2D*> PartTextures(const SkinnedModelEXT& model, Texture2D* face, const std::string& facePart,
                                             Other&& other)
        {
            std::vector<Texture2D*> textures;
            textures.reserve(model.Parts.size());
            for (const auto& part : model.Parts)
            {
                textures.push_back(face != nullptr && part.Name == facePart ? face : other(part.Texture));
            }
            return textures;
        }

        std::vector<Texture2D*> PartTextures(const SkinnedModelEXT& model, Texture2D* face, const std::string& facePart)
        {
            if (face == nullptr)
            {
                return {};
            }
            return PartTextures(model, face, facePart, [](Texture2D*) -> Texture2D* { return nullptr; });
        }
    }

    void GalaxyEggbertGame::BuildSceneFrame(SceneFrame& frame, bool& soulDrawnIn3D)
    {
        frame.Clear();
        frame.world = true;
        sceneEditTarget_ = phase_ == GalaxyEggbert::Def::GamePhase::Editor && !worldEditor_.IsBrowsing()
                               ? worldEditor_.CurrentEditTarget(worldRuntime_.GetWorld())
                               : std::nullopt;
        frame.camera.view = camera_.View();
        frame.camera.projection = camera_.Projection();
        frame.camera.position = camera_.Position();
        frame.camera.target = cameraTarget_;
        frame.camera.fieldOfView = camera_.FieldOfView();
        frame.camera.aspectRatio = camera_.AspectRatio();

        if (terrainRenderer_ && terrainEffect_)
        {
            frame.terrain = true;
            frame.reliefTarget = cameraTarget_;
            frame.reliefRadius = terrainReliefRadius_;
            frame.reliefBudget = terrainReliefWarm_ ? 1 : -1;
            frame.reportReliefTiming = !terrainReliefWarm_ && std::getenv("GALAXY_TERRAIN_RELIEF_TIMING") != nullptr;
            terrainReliefWarm_ = true;

            if (objectCubeEffect_)
            {
                AppendSceneCubes(frame);
            }
            // Blupi drawn opaque now, translucent (the drink's Hide, a ghost) over the finished scene.
            const bool translucentBlupi = blupi_.GetDrawOpacity(worldRuntime_.GetAnimPhase()) < 1.0f;
            if (!translucentBlupi)
            {
                AppendSceneBlupi(frame.opaqueModels);
            }
            if (blupiEffect_ && blupiModelIsReal_ && !blupiVehicles_.empty())
            {
                AppendSceneVehicles(frame);
            }
            if (blupiEffect_ && !objectModels_.empty())
            {
                AppendSceneObjectModels(frame, soulDrawnIn3D);
            }
            AppendSceneDecals(frame);
            if (translucentBlupi)
            {
                AppendSceneBlupi(frame.translucentModels);
            }
            // The other players of a network game, in their colours.
            AppendSceneNetPlayers(frame.opaqueModels, frame.translucentModels);
            AppendSceneEditorStarts(frame.editorStarts);
        }
        AppendSceneBillboards(frame);
        AppendSceneEffects(frame);
        ApplySpriteGi(frame);
    }

    void GalaxyEggbertGame::AppendSceneEffects(SceneFrame& frame)
    {
        // The 3D effects (the Setup's "3D effects"): presentation only, nothing of the game changes.
        auto& effects = frame.effects;
        effects.level = frame.world ? EffectsLevel() : 0;
        effects.enabled = effects.level >= 1;
        effects.occlusion = effects.surfaces = effects.smoothing = effects.level >= 2;
        effects.shadowMap = effects.pointLights = effects.glow = effects.level >= 3;
        effects.ultra = effects.level >= 4;
        effects.steadyShadows = goldenCaptureMode_ || goldenTraceMode_ || autoplay_ != nullptr;
        effects.capture = effects.steadyShadows;
        effects.water = waterBodies_;
        if (!effects.enabled)
        {
            effectParticles_.Clear();
            return;
        }
        // The distance haze in the colour of the world's own sky (its background picture's middle).
        effects.fogColor = backgroundHaze_;
        effects.fogStart = 20.0f;
        effects.fogEnd = 140.0f;
        effects.seconds = static_cast<float>(worldRuntime_.GetAnimPhase()) / 60.0f;

        const auto& world = worldRuntime_.GetWorld();
        const auto& objects = worldRuntime_.GetMobileObjects();
        const auto addShadow = [&](float x, float feetY, float z, float radius, float darkness, bool model)
        {
            if (const auto shadow = PlaceShadow(world, x, feetY, z, radius, darkness, model, WorldRuntime::kWorldCenterX,
                                                WorldRuntime::kWorldCenterZ))
            {
                effects.shadows.push_back(*shadow);
            }
        };
        // Blupi's, when he is seen.
        if (phase_ != GalaxyEggbert::Def::GamePhase::Editor && cameraMode_ == CameraMode::ThirdPersonModel &&
            !(blupiModelIsReal_ && blupi_.IsAnimIconInvisible()))
        {
            addShadow(blupi_.GetX(), blupi_.GetY() - 0.5f, blupi_.GetZ(), 0.34f, 0.55f,
                      blupiModelLoaded_ && !BlupiDrawnAsDeathSprite());
        }
        // Every object standing or flying in the world: pickups, enemies, vehicles (not the effects, nor
        // the lifts and crates, solid themselves).
        for (const auto& obj : objects)
        {
            if (!obj.active)
            {
                continue;
            }
            const auto visual = ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon);
            if (!visual.visible || visual.renderMode == ObjectRenderMode::SolidCube ||
                GetObjectDefinition(obj.type).semanticKind == ObjectSemanticKind::Effect)
            {
                continue;
            }
            // Drawn as a 3D model, it throws its real shadow when the shadow map is drawn.
            ObjectTextureSource sheet{};
            int icon = 0;
            const bool model = (ObjectSprite(obj, sheet, icon) && objectModelTable_.Find(sheet, icon) != nullptr) ||
                               ParkedVehicleModel(obj.type).model != nullptr || EnemyBlupiModelDrawn(obj) ||
                               ConveyorLiftModel(obj) != nullptr;
            addShadow(obj.currentX, obj.currentY - 0.5f, obj.currentZ, 0.32f, 0.45f, model);
        }

        // What lights what is near it: explosions and the lightning first, then the nearest lava and
        // the electrodes (SceneShaders.hpp's point lights; at most kMaxLights, the nearest first;
        // SceneEffectsLogic.hpp). High only.
        if (effects.pointLights)
        {
            const float t = effects.seconds;
            std::vector<ScenePointLight> strong;
            std::vector<ScenePointLight> weak;
            for (const auto& obj : objects)
            {
                ParticleKind kind{};
                if (!obj.active || !EffectParticleKind(obj.type, kind) || kind != ParticleKind::Fire ||
                    !ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon).visible)
                {
                    continue;
                }
                // A blast of several objects (a crate wall) is one light.
                const Vector3 at(obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ);
                if (NearAnyLight(strong, at, 3.0f))
                {
                    continue;
                }
                strong.push_back(ScenePointLight{at, Vector3(1.0f, 0.62f, 0.28f) * 0.85f, 4.5f});
            }
            if (terrainRenderer_)
            {
                if (terrainRenderer_->BlitzShown())
                {
                    for (const auto& cell : terrainRenderer_->BlitzCells())
                    {
                        strong.push_back(ScenePointLight{cell, Vector3(0.6f, 0.75f, 1.0f) * 1.8f, 6.0f});
                    }
                }
                // The lava: its nearest cells, a light every few blocks, flickering.
                const auto lava = PickLavaLightCells(terrainRenderer_->LavaCells(), cameraTarget_);
                for (std::size_t i = 0; i < lava.size(); ++i)
                {
                    const float flicker = 0.85f + 0.15f * std::sin(t * 7.3f + static_cast<float>(i + 1) * 1.7f);
                    weak.push_back(ScenePointLight{lava[i] + Vector3(0.0f, 0.45f, 0.0f),
                                                   Vector3(1.0f, 0.42f, 0.1f) * (0.9f * flicker), 3.5f});
                }
            }
            for (const auto& cell : electrodeCells_)
            {
                weak.push_back(ScenePointLight{Vector3(cell.x, cell.y, cell.z), Vector3(0.55f, 0.75f, 1.0f) * 0.9f, 2.5f});
            }
            effects.lights = SelectSceneLights(cameraTarget_, std::move(strong), std::move(weak),
                                               static_cast<std::size_t>(SceneShaders::kMaxLights));
        }

        // The explosions, splashes and sparks: their sprites become particles (EffectParticles.hpp).
        effectParticles_.Advance(worldRuntime_.GetAnimPhase());
        for (auto& batch : frame.billboards)
        {
            auto& quads = batch.quads;
            quads.erase(std::remove_if(quads.begin(), quads.end(),
                                       [&](const SceneQuad& quad)
                                       {
                                           if (quad.sourceObject < 0 ||
                                               quad.sourceObject >= static_cast<int>(objects.size()))
                                           {
                                               return false;
                                           }
                                           ParticleKind kind{};
                                           if (!EffectParticleKind(objects[static_cast<std::size_t>(quad.sourceObject)].type,
                                                                   kind))
                                           {
                                               return false;
                                           }
                                           effectParticles_.Feed(quad.sourceObject, kind, batch.sheet, quad.uv, quad.center);
                                           return ParticlesReplaceSprite(kind);
                                       }),
                        quads.end());
        }
        AppendSceneMoments();
        const auto invView = Matrix::Invert(camera_.View());
        // Between two steps (the Setup's frame rate above 60): back along their way by what is left of
        // the step, as the camera and the objects are drawn part way.
        effectParticles_.Append(effects.particles, invView.getRightProperty(), invView.getUpProperty(), particleLag_);
    }

    void GalaxyEggbertGame::ApplySpriteGi(SceneFrame& frame)
    {
        const auto* settings = frame.effects.ultra ? TerrainGiWanted() : nullptr;
        if (settings == nullptr || !terrainRenderer_)
        {
            return;
        }
        if (spriteGiTerrain_ != terrainRenderer_.get())
        {
            spriteGiCache_.clear(); // another terrain (a mission loaded, the terrain rebuilt)
            spriteGiTerrain_ = terrainRenderer_.get();
        }
        const auto& world = worldRuntime_.GetWorld();
        const Microsoft::Xna::Framework::Vector3 up(0.0f, 1.0f, 0.0f);
        const auto at = [&](const Microsoft::Xna::Framework::Vector3& p)
        {
            // The cell it stands in (blocks are centred on whole numbers; WorldRuntime's centre on X and Z).
            const int x = static_cast<int>(std::lround(p.X)) + WorldRuntime::kWorldCenterX;
            const int y = static_cast<int>(std::lround(p.Y));
            const int z = static_cast<int>(std::lround(p.Z)) + WorldRuntime::kWorldCenterZ;
            const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint16_t>(x)) << 32u) |
                                      (static_cast<std::uint64_t>(static_cast<std::uint16_t>(y)) << 16u) |
                                      static_cast<std::uint64_t>(static_cast<std::uint16_t>(z));
            auto found = spriteGiCache_.find(key);
            if (found == spriteGiCache_.end())
            {
                found = spriteGiCache_.emplace(key, FaceIndirectLight(world, tileCoverage_, x, y, z, up, *settings)).first;
            }
            return found->second;
        };
        for (auto* batches : {&frame.billboards, &frame.effects.particles})
        {
            for (auto& batch : *batches)
            {
                for (auto& quad : batch.quads)
                {
                    quad.gi = at(quad.center);
                }
            }
        }
        for (auto& quad : frame.marine)
        {
            quad.gi = at(quad.center);
        }
    }

    void GalaxyEggbertGame::AppendSceneMoments()
    {
        // Blupi's and the world's moments as particles (presentation only, read from the state the game
        // already has): dust where he lands and runs, a splash as he goes into water, bubbles from him
        // under it, stars as he teleports, sparks rising from the lava. Pieces of the original sprites:
        // the pollution puff (element 179-186), the plouf splash (object-m 99-102), the blup bubbles
        // (object-m 103-106), the stars (element 152-156).
        auto& particles = effectParticles_;
        const int stepped = particles.Stepped();
        const int tick = particles.Tick();
        const auto element = [](int icon) { return Uv(GetElementIconUv(icon)); };
        const bool playing = phase_ == GalaxyEggbert::Def::GamePhase::Play;
        const bool seen = playing && cameraMode_ == CameraMode::ThirdPersonModel && !blupi_.IsAnimIconInvisible();
        const bool ground = blupi_.IsOnGround();
        const bool swimming = blupi_.IsNage();
        const bool teleporting = blupi_.IsTeleporting();
        const Vector3 feet(blupi_.GetX(), blupi_.GetY() - 0.5f, blupi_.GetZ());
        if (seen && stepped > 0)
        {
            // Landing after a jump or a fall (not a step), and running on foot.
            if (ground && !momentWasGround_ && momentAirTicks_ >= 12 && !swimming)
            {
                particles.Burst(ParticleKind::Dust, 1, SceneSheet::Element, element(181), feet, 10,
                                Microsoft::Xna::Framework::Color(214, 196, 168, 255));
            }
            const auto anim = blupi_.GetAnimState();
            if (ground && !swimming && anim == BlupiController::AnimState::March)
            {
                if (const int puffs = particles.Crossed(9); puffs > 0)
                {
                    particles.Burst(ParticleKind::Dust, 2, SceneSheet::Element, element(179 + (tick / 9) % 4), feet,
                                    puffs * 2, Microsoft::Xna::Framework::Color(214, 196, 168, 200));
                }
            }
            // Into the water: a splash at its top.
            if (swimming && !momentWasSwimming_)
            {
                particles.Burst(ParticleKind::Splash, 3, SceneSheet::ObjectM, tileAtlas_.GetTileUv(101),
                                feet + Vector3(0.0f, 0.4f, 0.0f), 14);
            }
            // Under it: a bubble now and then.
            const auto& world = worldRuntime_.GetWorld();
            const Vector3 head = feet + Vector3(0.0f, 0.8f, 0.0f);
            if (swimming && GalaxyEggbert::Game::WaterViewImmersion(world, head.X + WorldRuntime::kWorldCenterX, head.Y,
                                                                    head.Z + WorldRuntime::kWorldCenterZ) > 0.9f)
            {
                if (const int bubbles = particles.Crossed(14); bubbles > 0)
                {
                    particles.Burst(ParticleKind::Bubble, 4, SceneSheet::ObjectM, tileAtlas_.GetTileUv(103 + (tick / 14) % 4),
                                    head, bubbles);
                }
            }
            // Teleporting: stars bursting out as he goes and as he comes.
            if (teleporting != momentWasTeleporting_)
            {
                particles.Burst(ParticleKind::Flash, 5, SceneSheet::Element, element(152 + tick % 5),
                                feet + Vector3(0.0f, 0.5f, 0.0f), 24, Microsoft::Xna::Framework::Color(255, 250, 220, 255));
            }
        }
        if (stepped > 0)
        {
            momentAirTicks_ = ground ? 0 : momentAirTicks_ + stepped;
            momentWasGround_ = ground;
            momentWasSwimming_ = swimming;
            momentWasTeleporting_ = teleporting;
        }

        // The lava's sparks: from its cells nearest the camera's target, a few a second.
        if (terrainRenderer_ && stepped > 0)
        {
            if (const int sparks = particles.Crossed(4); sparks > 0)
            {
                auto lava = terrainRenderer_->LavaCells();
                const auto& target = cameraTarget_;
                const auto far = [&](const Vector3& p) {
                    const float dx = p.X - target.X, dz = p.Z - target.Z, dy = p.Y - target.Y;
                    return dx * dx + dy * dy + dz * dz;
                };
                std::sort(lava.begin(), lava.end(), [&](const Vector3& a, const Vector3& b) { return far(a) < far(b); });
                const std::size_t near = std::min<std::size_t>(lava.size(), 24);
                for (int k = 0; k < sparks && near > 0; ++k)
                {
                    // A cell among the nearest, picked by the tick (the same every run).
                    const std::size_t pick = static_cast<std::size_t>((tick / 4 + k) * 7919) % near;
                    if (far(lava[pick]) > 16.0f * 16.0f)
                    {
                        continue;
                    }
                    particles.Burst(ParticleKind::Ember, 6 + static_cast<int>(pick), SceneSheet::Element,
                                    element(152 + (tick / 4 + k) % 5), lava[pick] + Vector3(0.0f, 0.45f, 0.0f), 1,
                                    Microsoft::Xna::Framework::Color(255, 150, 40, 255));
                }
            }
        }
    }

    bool GalaxyEggbertGame::SceneEditorSelected(int kind, float rawX, float rawY, float rawZ) const
    {
        const auto& target = sceneEditTarget_;
        return target && static_cast<int>(target->kind) == kind && std::lround(rawX) == target->x &&
               std::lround(rawY) == target->y && std::lround(rawZ) == target->z;
    }

    void GalaxyEggbertGame::AppendSceneCubes(SceneFrame& frame)
    {
        // The platform lifts and crates (UniformCube objects) on the terrain's sheet, each on its
        // per-instance phase, so the chenille icon formulas of types 47/48 animate.
        for (const auto& obj : worldRuntime_.GetMobileObjects())
        {
            const auto visual = ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon);
            if (!obj.active || !visual.visible || visual.renderMode != ObjectRenderMode::SolidCube)
            {
                continue;
            }
            if (ConveyorLiftModel(obj) != nullptr)
            {
                continue; // drawn as Decor/chenille.rds in the model pass
            }
            const Vector3 center(obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ);
            const auto tileUv = tileAtlas_.GetTileUv(visual.icon);
            const auto addBox = [&](const Vector3& boxCenter, const Vector3& size,
                                    const CnaEngine::Graphics::BoxFaces& faces) {
                frame.cubes.push_back(SceneCube{boxCenter, size, faces});
            };
            const auto allFaces = [](const CnaEngine::Graphics::UvRect& uv) {
                CnaEngine::Graphics::BoxFaces faces;
                faces.fill(CnaEngine::Graphics::BoxFaceStyle{true, uv});
                return faces;
            };
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType1 &&
                ChooseLiftDrawPath(visual.icon, false) == LiftDrawPath::BarAndStem)
            {
                // The lift tile 29's bar and stem, not a cube of its mostly transparent tile.
                const float du = tileUv.u1 - tileUv.u0;
                const float dv = tileUv.v1 - tileUv.v0;
                for (const auto& part : LiftVisualParts())
                {
                    addBox(center + Vector3(0.0f, part.centerYOffset, 0.0f), Vector3(part.width, part.height, part.width),
                           allFaces(CnaEngine::Graphics::UvRect{tileUv.u0 + part.u0 * du, tileUv.v0 + part.v0 * dv,
                                                                tileUv.u0 + part.u1 * du, tileUv.v0 + part.v1 * dv}));
                }
                continue;
            }
            // A lift wearing any other tile (and any other cube object) is that tile's cube moving: a
            // directional tile with the faces the terrain gives it.
            const auto& definition = GalaxyEggbert::GetBlockDefinition(static_cast<std::uint16_t>(visual.icon));
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType1 &&
                definition.renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube)
            {
                CnaEngine::Graphics::BoxFaces faces;
                ConfigureDirectionalCubeFaces(visual.icon, tileUv, tileAtlas_.GetTileUv(107), faces);
                addBox(center, Vector3(1.0f, 1.0f, 1.0f), faces);
                continue;
            }
            addBox(center, Vector3(1.0f, 1.0f, 1.0f), allFaces(tileUv));
        }
        // The crates blown away (InteractionSystem::ByeByePiece, BD-0049): each its cube, spinning
        // about the view axis where it flies.
        for (const auto& piece : interaction_.ByeByePieces())
        {
            if (piece.helicopter || piece.fanIcon >= 0)
            {
                continue;
            }
            const auto visual = ResolveObjectVisual(piece.look.type, static_cast<int>(piece.look.phase),
                                                    piece.look.visualIcon);
            if (!visual.visible || visual.renderMode != ObjectRenderMode::SolidCube)
            {
                continue;
            }
            SceneCubePiece cube;
            cube.world = Matrix::CreateRotationZ(-piece.rotationDegrees * 3.14159265f / 180.0f) *
                         Matrix::CreateTranslation(piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z);
            cube.uv = tileAtlas_.GetTileUv(visual.icon);
            frame.cubePieces.push_back(cube);
        }
    }

    void GalaxyEggbertGame::AppendSceneBlupi(std::vector<SceneModel>& models)
    {
        // His real model (or the placeholder) in third person only. Hidden during the death-lock's
        // life-loss Voyage, on the frame tables' invisible frames, and drawn as his sprite where the
        // original's death is a sprite. A frame drawn before the first Play tick has no clip yet.
        if (phase_ == GalaxyEggbert::Def::GamePhase::Editor || cameraMode_ != CameraMode::ThirdPersonModel ||
            !blupiModelLoaded_ || !blupiEffect_ || blupiModel_->Clips.count(blupiActiveClipName_) == 0 ||
            (blupiModelIsReal_ && blupi_.IsAnimIconInvisible()) || BlupiDrawnAsDeathSprite())
        {
            return;
        }
        constexpr float kCharredShade = 0.07f; // the sprites' soot black
        // Charred (Clear1, Clear3: CharredBlupi.hpp), shrunk toward his feet as the frame crumbles and
        // spread out as the heap of ash. Glued (BlupiGlueModel()): flattened and spread under the glue
        // model, the dark green puddle of the sprites (168-171).
        auto charred = BlupiCharredShape();
        const bool gluePuddle = BlupiGlueModel() != nullptr;
        if (gluePuddle)
        {
            charred = GalaxyEggbert::Game::CharredShape{true, 0.06f, 1.5f, false};
        }
        // The drink's Hide (Decor.cpp:828-838, OR-0016): his own clips at 30 % opacity, flashing to
        // full every other pair of ticks once the gauge is down to 25; not depth-written.
        const float alpha = blupi_.GetDrawOpacity(worldRuntime_.GetAnimPhase());
        SceneModel body;
        body.model = blupiModel_.get();
        body.clip = blupiActiveClipName_;
        body.seconds = blupiClipTimeSeconds_;
        body.world = BlupiModelWorld(charred);
        body.alpha = alpha;
        body.translucent = alpha < 1.0f;
        if (charred.charred)
        {
            // Burnt black: soot, no sheen; glued, the puddle's dark green.
            body.tinted = true;
            body.tint = gluePuddle ? Vector3(0.20f, 0.32f, 0.03f) : Vector3(kCharredShade, kCharredShade, kCharredShade);
        }
        body.partTextures = PartTextures(*blupiModel_, BlupiFaceTexture(), blupiFacePart_);
        if (const int team = NetTeam(); team != 0 && blupiModelIsReal_)
        {
            // His team's colour in a network game (SB2's coloured Blupi channels, decor.cpp:211-224).
            Texture2D* face = BlupiFaceTexture();
            body.partTextures.clear();
            for (const auto& part : blupiModel_->Parts)
                body.partTextures.push_back(TeamTexture(team, face && part.Name == blupiFacePart_ ? face : part.Texture));
        }
        models.push_back(std::move(body));

        // The vehicle he rides in this clip (helicopter, jeep, tank, overcraft, skateboard): its own
        // model, built from the same Ray Dream frames in the same space, so it takes his transform and
        // clip time.
        for (const auto& [vehicleName, vehicle] : blupiVehicles_)
        {
            if (vehicle->Clips.find(blupiActiveClipName_) == vehicle->Clips.end())
            {
                continue;
            }
            SceneModel ride;
            ride.model = vehicle.get();
            ride.clip = blupiActiveClipName_;
            ride.seconds = blupiClipTimeSeconds_;
            ride.world = models.back().world;
            ride.alpha = alpha;
            ride.translucent = alpha < 1.0f;
            models.push_back(std::move(ride));
        }

        // The Hide's fading ghosts behind him.
        for (const auto& ghost : hideGhosts_)
        {
            if (blupiModel_->Clips.count(ghost.clip) == 0)
            {
                continue;
            }
            SceneModel echo;
            echo.model = blupiModel_.get();
            echo.clip = ghost.clip;
            echo.seconds = ghost.clipSeconds;
            echo.world = ghost.world;
            echo.alpha = std::max(0.0f, (20.0f - ghost.ageTicks) * 0.3f / 20.0f);
            echo.translucent = true;
            echo.partTextures = PartTextures(*blupiModel_, ghost.face, blupiFacePart_);
            models.push_back(std::move(echo));
        }
    }

    void GalaxyEggbertGame::AppendSceneVehicles(SceneFrame& frame)
    {
        // Parked vehicles: a helicopter, jeep, skateboard, tank or overcraft pickup is its real model
        // in the first frame of its standing clip instead of its element.png billboard, in both camera
        // modes, standing on its cell's floor, facing +X as the sprites face right.
        using TargetKind = GalaxyEggbert::Editor::WorldEditor::EditTarget::Kind;
        constexpr float kHalfPi = 1.57079632679f;
        constexpr float kSkateLift = 0.45f; // the upright board hangs this far above the floor, tuned by eye
        auto& models = frame.opaqueModels;
        frame.vehiclePassStart = static_cast<int>(models.size());
        for (const auto& obj : worldRuntime_.GetMobileObjects())
        {
            const auto parked = ParkedVehicleModel(obj.type);
            if (parked.model == nullptr || !obj.active ||
                !ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon).visible)
            {
                continue;
            }
            // The parked skateboard of the original (table_skate, element icons 129-135) hangs upright,
            // its long axis up, a little above the floor, and turns about that axis, 30 degrees an
            // icon (Decor.cpp:8339; OR-0083).
            float skateSpin = 0.0f;
            float skateLift = 0.0f;
            float skateStand = 0.0f;
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType24)
            {
                const int skateIcon = GalaxyEggbert::Game::GetObjIcon(obj.type, static_cast<int>(obj.phase));
                skateSpin = static_cast<float>(skateIcon - 132) * (30.0f * 3.14159265f / 180.0f);
                skateStand = kHalfPi;
                skateLift = kSkateLift;
            }
            SceneModel model;
            model.model = parked.model;
            model.clip = parked.clip;
            model.seconds = 0.0;
            model.loop = false;
            // Facing +X, turned in the editor a quarter at a time (OR-0071).
            model.world = Matrix::CreateScale(kRealModelScale) * Matrix::CreateRotationX(skateStand) *
                          Matrix::CreateRotationY(skateSpin) *
                          Matrix::CreateRotationY(kHalfPi * static_cast<float>(
                                                               1 + CellObjectQuarterTurns(obj.posStartX, obj.posStartY,
                                                                                          obj.posStartZ))) *
                          Matrix::CreateTranslation(obj.currentX, obj.currentY - 0.5f + skateLift, obj.currentZ);
            model.editorSelected = SceneEditorSelected(static_cast<int>(TargetKind::Object),
                                                       obj.posStartX + WorldRuntime::kWorldCenterX, obj.posStartY,
                                                       obj.posStartZ + WorldRuntime::kWorldCenterZ);
            models.push_back(std::move(model));
        }
        AppendSceneEnemyBlupis(models);
        // The lost helicopter flying away (ByeByeHelico, BD-0049): its parked model, turned about its
        // middle as the original's picture spins.
        const auto helicopter = ParkedVehicleModel(GalaxyEggbert::Def::ObjectType::ObjectType13);
        for (const auto& piece : interaction_.ByeByePieces())
        {
            if (!piece.helicopter || helicopter.model == nullptr)
            {
                continue;
            }
            SceneModel model;
            model.model = helicopter.model;
            model.clip = helicopter.clip;
            model.seconds = 0.0;
            model.loop = false;
            model.world = Matrix::CreateScale(kRealModelScale) * Matrix::CreateRotationY(kHalfPi) *
                          Matrix::CreateTranslation(0.0f, -0.5f, 0.0f) *
                          Matrix::CreateRotationZ(-piece.rotationDegrees * 3.14159265f / 180.0f) *
                          Matrix::CreateTranslation(piece.x, piece.y, piece.z);
            models.push_back(std::move(model));
        }
    }

    void GalaxyEggbertGame::AppendSceneEnemyBlupis(std::vector<SceneModel>& models)
    {
        // OR-0063 (EnemyBlupi.hpp): his model and his helicopter or tank in the clip and frame the
        // original's icon shows, the textures turned green and blue as blupi1.png's, facing along the
        // patrol. A turn swings his front past the camera, as the sprites turn through their viewer.
        const auto cameraPosition = Matrix::Invert(camera_.View()).getTranslationProperty();
        const auto recolored = [&](Texture2D* texture) -> Texture2D*
        {
            const auto found = enemyBlupiTextures_.find(texture);
            return found != enemyBlupiTextures_.end() ? found->second.get() : texture;
        };
        for (const auto& obj : worldRuntime_.GetMobileObjects())
        {
            if (!obj.active || !EnemyBlupiModelDrawn(obj) ||
                !ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon).visible)
            {
                continue;
            }
            const bool tank = obj.type == GalaxyEggbert::Def::ObjectType::ObjectType33;
            const auto pose = GalaxyEggbert::Game::ResolveEnemyBlupiPose(tank, obj.patrolStep, obj.patrolTime);
            const std::string clip = GalaxyEggbert::Game::EnemyBlupiClipName(pose.clip);
            if (blupiModel_->Clips.count(clip) == 0)
            {
                continue;
            }
            // Facing posStart (yaw0) and posEnd (yaw0 + pi); a model's +Z turned by a yaw a points along
            // (sin a, cos a). Standing still (posStart = posEnd) he faces +X, the sprites' right.
            const float dx = obj.posEndX - obj.posStartX;
            const float dz = obj.posEndZ - obj.posStartZ;
            const bool still = dx == 0.0f && dz == 0.0f;
            const float yawStart = still ? -kPi * 0.5f : std::atan2(-dx, -dz);
            const float towardEnd = still ? 1.0f : pose.towardEnd;
            // The side the turn swings through: the one facing the camera half way.
            const float toCameraX = cameraPosition.X - obj.currentX;
            const float toCameraZ = cameraPosition.Z - obj.currentZ;
            const float midLeft = yawStart + kPi * 0.5f;
            const float side = std::sin(midLeft) * toCameraX + std::cos(midLeft) * toCameraZ >= 0.0f ? 1.0f : -1.0f;
            const float yaw = yawStart + side * kPi * towardEnd;
            const float seconds = pose.clipTicks / 20.0f;
            const auto world = Matrix::CreateScale(kRealModelScale) * Matrix::CreateRotationY(yaw) *
                               Matrix::CreateTranslation(obj.currentX, obj.currentY - 0.5f, obj.currentZ);
            Texture2D* face = nullptr;
            if (const auto faces = blupiClipFaces_.find(clip); faces != blupiClipFaces_.end() && !faces->second.empty())
            {
                const auto frame = static_cast<std::size_t>(seconds * blupiFaceRate_) % faces->second.size();
                const int index = faces->second[frame];
                if (index >= 0 && index < static_cast<int>(blupiFaces_.size()))
                {
                    face = recolored(&blupiFaces_[static_cast<std::size_t>(index)]);
                }
            }
            SceneModel body;
            body.model = blupiModel_.get();
            body.clip = clip;
            body.seconds = seconds;
            body.world = world;
            body.partTextures = PartTextures(*blupiModel_, face, blupiFacePart_, recolored);
            models.push_back(std::move(body));
            for (const auto& [vehicleName, vehicle] : blupiVehicles_)
            {
                if (vehicle->Clips.count(clip) == 0)
                {
                    continue;
                }
                SceneModel ride;
                ride.model = vehicle.get();
                ride.clip = clip;
                ride.seconds = seconds;
                ride.world = world;
                ride.partTextures = PartTextures(*vehicle, nullptr, blupiFacePart_, recolored);
                models.push_back(std::move(ride));
            }
        }
    }

    void GalaxyEggbertGame::AppendSceneObjectModels(SceneFrame& frame, bool& soulDrawnIn3D)
    {
        // Objects with a converted 3D model (objectModelTable_, plan.md Phase 7B): drawn where their
        // billboard would be (skipped in the billboard passes), posed at their icon's frame, turned by
        // its yaw and flipped as the icon is (left-right or upside down, about the sprite's centre
        // line: kFlipShift). A model is in blocks with its sprite's view toward +Z, as the parked
        // vehicles are. BigDecor cells with a model stand on their cell's floor at the original's
        // size: an explo.png icon is 64, 128 or 144 game pixels (icons.txt), drawn 1:1 in the original
        // (Pixmap.cpp), where the billboard is 1 block.
        using TargetKind = GalaxyEggbert::Editor::WorldEditor::EditTarget::Kind;
        constexpr float kDegrees = 3.14159265358979f / 180.0f;
        frame.objectModelPass = true;
        auto& models = frame.objectModels;
        // The signs drawn this frame whose painted face is a decal (drawn after the models), the
        // electrodes and dynamite with painted sparks.
        auto& decalCells = signDecalCells_;
        decalCells.clear();
        electrodeCells_.clear();
        dynamiteSparkCells_.clear();
        bool tintModelCell = false;
        const auto drawModel = [&](const ObjectModelPose& pose, float x, float y, float z, float turnRadians = 0.0f,
                                   float rollRadians = 0.0f, float depthScale = 1.0f, bool boothFrontOnly = false,
                                   float topScale = 1.0f) -> SceneModel&
        {
            const auto index = static_cast<std::size_t>(pose.model);
            SceneModel model;
            model.model = objectModels_[index].get();
            model.clip = "Frames";
            model.seconds = pose.seconds;
            model.loop = false;
            // Models out of view are not drawn (objectModelRadii_): each one's sphere, its reach widened
            // by its depth scale and sprite offset, a block's margin for its animation.
            if (modelCulling_ && index < objectModelRadii_.size())
            {
                model.cullRadius = objectModelRadii_[index] * std::max(1.0f, std::fabs(depthScale)) +
                                   static_cast<float>(std::abs(pose.offsetX) + std::abs(pose.offsetY)) / 64.0f + 1.0f;
                model.cullCenter = Vector3(x, y, z);
            }
            // Mirroring turns the triangles' winding around (a negative depth scale too).
            model.cull = (pose.mirror != 0) != (depthScale < 0.0f) ? SceneCull::Clockwise : SceneCull::CounterClockwise;
            model.world =
                Matrix::CreateRotationY(pose.yawDegrees * kDegrees) *
                Matrix::CreateScale(pose.mirror == 1 ? -1.0f : 1.0f, pose.mirror == 2 ? -1.0f : 1.0f, 1.0f) *
                Matrix::CreateTranslation((pose.mirror == 1 ? -kFlipShift : 0.0f) + static_cast<float>(pose.offsetX) / 64.0f,
                                          (pose.mirror == 2 ? kFlipShift : 0.0f) - static_cast<float>(pose.offsetY) / 64.0f,
                                          0.0f) *
                Matrix::CreateScale(1.0f, 1.0f, depthScale) *
                // Scaled about the cell's top (the drip's smallest drops, DripShrink).
                Matrix::CreateTranslation(0.0f, -0.5f, 0.0f) * Matrix::CreateScale(topScale) *
                Matrix::CreateTranslation(0.0f, 0.5f, 0.0f) * Matrix::CreateRotationZ(rollRadians) *
                Matrix::CreateRotationY(turnRadians) * Matrix::CreateTranslation(x, y, z);
            model.sawSeconds = pose.seconds;
            model.boothFrontOnly = boothFrontOnly;
            model.editorSelected = tintModelCell;
            model.counted = true;
            if (index < objectPartMaterials_.size() && !objectPartMaterials_[index].empty())
            {
                model.partMaterials = &objectPartMaterials_[index];
            }
            models.push_back(std::move(model));
            return models.back();
        };
        // The clay bridge: its row of lumps across the whole cell (BlockModels.hpp).
        const auto drawBridgeRows = [&](const ObjectModelPose& pose, float x, float y, float z)
        {
            for (int row = 0; row < GalaxyEggbert::Game::kBridgeRows; ++row)
            {
                const auto placement = GalaxyEggbert::Game::BridgeRowPlacement(row);
                drawModel(pose, x, y, z + placement.dz, placement.yaw);
            }
        };
        for (const auto& obj : worldRuntime_.GetMobileObjects())
        {
            tintModelCell = SceneEditorSelected(static_cast<int>(TargetKind::Object),
                                                obj.posStartX + WorldRuntime::kWorldCenterX, obj.posStartY,
                                                obj.posStartZ + WorldRuntime::kWorldCenterZ);
            ObjectTextureSource sheet{};
            int icon = 0;
            const ObjectModelPose* pose = ObjectSprite(obj, sheet, icon) ? objectModelTable_.Find(sheet, icon) : nullptr;
            if (pose != nullptr && sheet == ObjectTextureSource::ObjectM && GalaxyEggbert::Game::IsClayBridgeIcon(icon))
            {
                // ObjectType52 writes every visible frame to the grid, including 367-372. Its terrain
                // model now owns the drawing (also for the static gallery).
                if (!terrainRenderer_)
                    drawBridgeRows(*pose, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ);
            }
            else if (const ObjectModelPose* conveyor = ConveyorLiftModel(obj))
            {
                // The conveyor lift (47/48): its tread on the frame table_chenille(i) shows, stretched
                // across the cell as the terrain conveyor (ModelDepthScale); the plain lift (1) its round
                // platform on a funnel and stem, as modelled.
                const bool plainLift = obj.type == GalaxyEggbert::Def::ObjectType::ObjectType1;
                const float depth = plainLift ? 1.0f : GalaxyEggbert::Game::ModelDepthScale(311);
                drawModel(*conveyor, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ,
                          0.0f, 0.0f, depth);
                if (!plainLift)
                {
                    // The belt on both sides (BlockModels.hpp ModelMirroredInDepth).
                    drawModel(*conveyor, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY),
                              obj.currentZ, 0.0f, 0.0f, -depth);
                }
            }
            else if (pose != nullptr)
            {
                // A sprite only faces left or right, along X, by its travel along X. One patrolling
                // mainly along Z is turned from that facing to its real one.
                const float dx = obj.posEndX - obj.posStartX;
                const float dz = obj.posEndZ - obj.posStartZ;
                float turn = 0.0f;
                if (std::fabs(dz) > std::fabs(dx))
                {
                    const float spriteFacing = obj.posStartX > obj.posEndX ? 3.14159265f : 0.0f;
                    turn = std::atan2(-dz, dx) - spriteFacing;
                }
                // A still object turned in the editor (OR-0071: its cell's quarter turns).
                if (dx == 0.0f && dz == 0.0f && obj.posStartY == obj.posEndY)
                {
                    turn += 1.57079632f *
                            static_cast<float>(CellObjectQuarterTurns(obj.posStartX, obj.posStartY, obj.posStartZ));
                }
                drawModel(*pose, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ, turn);
                if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType56 && dynamiteSparks_.count(icon) != 0)
                {
                    // Centre of the terminal ring in the shipped dynamite_rd_6e6e00 mesh at Frames=0
                    // (model space). Anchor the star to the fuse, so orbiting the camera or turning the
                    // object cannot detach it.
                    const auto tip = Vector3::Transform(Vector3(0.22986f, -0.38538f, 0.51735f),
                                                        Matrix::CreateRotationY(turn));
                    dynamiteSparkCells_.push_back({static_cast<std::uint16_t>(icon), obj.currentX + tip.X,
                                                   ResolveObjectVisualCenterY(obj.type, obj.currentY) + tip.Y,
                                                   obj.currentZ + tip.Z, 0.0f});
                }
            }
        }
        // The pieces flying away with a model (BD-0049): enemies, the invert potion, a fan, spinning
        // about the view axis as the original's picture does.
        tintModelCell = false;
        for (const auto& piece : interaction_.ByeByePieces())
        {
            const float spin = -piece.rotationDegrees * kDegrees;
            if (piece.helicopter)
            {
                continue; // the parked vehicles' pass
            }
            if (piece.fanIcon >= 0)
            {
                if (const ObjectModelPose* fan =
                        objectModelTable_.Find(ObjectTextureSource::ObjectM, GalaxyEggbert::Game::kFanModelIcon))
                {
                    drawModel(*fan, piece.x, piece.y, piece.z, 0.0f,
                              GalaxyEggbert::Game::FanModelRoll(static_cast<std::uint16_t>(piece.fanIcon)) + spin);
                }
                continue;
            }
            ObjectTextureSource sheet{};
            int icon = 0;
            if (ObjectSprite(piece.look, sheet, icon))
            {
                if (const ObjectModelPose* pose = objectModelTable_.Find(sheet, icon))
                {
                    drawModel(*pose, piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z, 0.0f, spin);
                }
            }
        }
        // The Clear2 death's soul (OR-0047): element 230-241 as Blupi/blupi-ange.rd4, the turn its
        // icons show, rising 300 px from where he died (Decor::BlupiDead, VoyageDraw), half transparent
        // as the sheet's sprite (alpha 127).
        if (interaction_.VoyageActive() && interaction_.VoyageIsSoul())
        {
            if (const ObjectModelPose* pose =
                    objectModelTable_.Find(ObjectTextureSource::Element, interaction_.VoyageIconId()))
            {
                constexpr float kSoulRise = 300.0f / 64.0f;
                auto& soul = drawModel(*pose, interaction_.VoyageWorldAnchorX(),
                                       interaction_.VoyageWorldAnchorY() + interaction_.VoyageProgress() * kSoulRise,
                                       interaction_.VoyageWorldAnchorZ());
                soul.alpha = 0.5f;
                soul.translucent = true;
                soulDrawnIn3D = true;
            }
        }
        // The glue death (element 168-171; Q-006): Blupi/glu.rd4's struggling pose, its yellow made the
        // sprites' green at conversion (texrecolour), turned the way he faces, its icon's centre 30 px
        // over his feet; his own model lies flattened under it as the puddle (BlupiGlueModel()).
        if (const ObjectModelPose* glued = BlupiGlueModel())
        {
            drawModel(*glued, blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f, blupi_.GetZ(), kPi - blupi_.GetYaw());
            glueStrandsPending_ = true;
        }
        // The terrain's model cells: fans, saws, animated tiles, pillars, lattices, bars, signs,
        // teleporters, the clay bridge.
        if (terrainRenderer_)
        {
            for (const auto& cell : terrainRenderer_->ModelCells())
            {
                tintModelCell = SceneEditorSelected(static_cast<int>(TargetKind::Block),
                                                    cell.x + WorldRuntime::kWorldCenterX, cell.y,
                                                    cell.z + WorldRuntime::kWorldCenterZ);
                const bool saw = std::find(GalaxyEggbert::Game::kSawModelBases.begin(),
                                           GalaxyEggbert::Game::kSawModelBases.end(),
                                           cell.base) != GalaxyEggbert::Game::kSawModelBases.end();
                if (saw)
                {
                    // Independent blade in its own cell; openSide carries its axis.
                    const int icon = GalaxyEggbert::ResolveBlockAnimationIconAt(
                        cell.base, worldRuntime_.GetAnimPhase(), static_cast<int>(std::lround(cell.x)),
                        static_cast<int>(std::lround(cell.y)), static_cast<int>(std::lround(cell.z)));
                    if (icon >= 0)
                    {
                        if (const ObjectModelPose* pose =
                                objectModelTable_.Find(ObjectTextureSource::ObjectM, static_cast<std::uint16_t>(icon)))
                        {
                            drawModel(*pose, cell.x, cell.y + GalaxyEggbert::Game::kSawModelYOffset, cell.z,
                                      GalaxyEggbert::Game::FrontModelYaw(cell.openSide) +
                                          1.57079632f * static_cast<float>(CellObjectQuarterTurns(cell.x, cell.y, cell.z)));
                        }
                    }
                    continue;
                }
                if (std::find(GalaxyEggbert::Game::kAnimatedModelBases.begin(),
                              GalaxyEggbert::Game::kAnimatedModelBases.end(),
                              cell.base) != GalaxyEggbert::Game::kAnimatedModelBases.end())
                {
                    // An animated tile as its scene's model, on the tile's frame (gone: none).
                    const int icon = GalaxyEggbert::ResolveBlockAnimationIconAt(
                        cell.base, worldRuntime_.GetAnimPhase(), static_cast<int>(std::lround(cell.x)),
                        static_cast<int>(std::lround(cell.y)), static_cast<int>(std::lround(cell.z)));
                    if (icon >= 0)
                    {
                        const auto shrink = GalaxyEggbert::Game::DripShrink(icon);
                        if (const ObjectModelPose* pose =
                                objectModelTable_.Find(ObjectTextureSource::ObjectM, shrink.modelIcon))
                        {
                            drawModel(*pose, cell.x, cell.y, cell.z, 0.0f, 0.0f, 1.0f, false, shrink.scale);
                        }
                    }
                    continue;
                }
                if (std::find(GalaxyEggbert::Game::kFanModelBases.begin(), GalaxyEggbert::Game::kFanModelBases.end(),
                              cell.base) == GalaxyEggbert::Game::kFanModelBases.end())
                {
                    // A still icon drawn as its model (a pillar), or a lattice of its frame. The spring
                    // bounces through its frames (BlockModels.hpp SpringFrameIcon).
                    const std::uint16_t shown =
                        cell.base == GalaxyEggbert::BlockTypes::Spring
                            ? GalaxyEggbert::Game::SpringFrameIcon(
                                  worldRuntime_.GetAnimPhase(),
                                  static_cast<int>(std::lround(cell.x)) + WorldRuntime::kWorldCenterX,
                                  static_cast<int>(std::lround(cell.z)) + WorldRuntime::kWorldCenterZ)
                            : cell.base;
                    if (const ObjectModelPose* still = objectModelTable_.Find(ObjectTextureSource::ObjectM, shown))
                    {
                        if (std::find(GalaxyEggbert::Game::kLatticeModelIcons.begin(),
                                      GalaxyEggbert::Game::kLatticeModelIcons.end(),
                                      cell.base) != GalaxyEggbert::Game::kLatticeModelIcons.end())
                        {
                            for (const auto& side : GalaxyEggbert::Game::kLatticeSides)
                            {
                                if ((cell.sameSides & side.bit) == 0)
                                {
                                    drawModel(*still, cell.x + side.dx, cell.y, cell.z + side.dz, side.yaw);
                                }
                            }
                        }
                        else if (std::find(GalaxyEggbert::Game::kRowModelIcons.begin(),
                                           GalaxyEggbert::Game::kRowModelIcons.end(),
                                           cell.base) != GalaxyEggbert::Game::kRowModelIcons.end())
                        {
                            // A bar is two pipes along its row, one under each of Blupi's hands
                            // (BarRows.hpp); the model runs along X, turned where the row runs along Z.
                            const bool alongZ =
                                GalaxyEggbert::Game::ClassifyBarRow((cell.sameSides & 3) != 0, (cell.sameSides & 12) != 0) ==
                                GalaxyEggbert::Game::BarRow::AlongZ;
                            for (const float side : {-1.0f, 1.0f})
                            {
                                const float offset = side * GalaxyEggbert::Game::kBarPipeOffset;
                                drawModel(*still, cell.x + (alongZ ? offset : 0.0f), cell.y,
                                          cell.z + (alongZ ? 0.0f : offset), alongZ ? 1.57079632f : 0.0f);
                            }
                        }
                        else if (std::find(GalaxyEggbert::Game::kFrontModelIcons.begin(),
                                           GalaxyEggbert::Game::kFrontModelIcons.end(),
                                           cell.base) != GalaxyEggbert::Game::kFrontModelIcons.end() ||
                                 GalaxyEggbert::Game::IsSignModelIcon(cell.base))
                        {
                            // Turned in the editor a quarter at a time from the side it faces by itself
                            // (OR-0071), its turns kept as an object's (metadata type 5): the plate-rotation
                            // bit the hub generators set on signs facing X must not turn the model too.
                            const float frontYaw =
                                GalaxyEggbert::Game::FrontModelYaw(cell.openSide) +
                                1.57079632f * static_cast<float>(CellObjectQuarterTurns(cell.x, cell.y, cell.z));
                            drawModel(*still, cell.x, cell.y, cell.z, frontYaw);
                            if (GalaxyEggbert::Game::IsTeleporterIcon(cell.base))
                            {
                                // The letter plate and lights on its other three sides too
                                // (BlockModels.hpp IsTeleporterIcon, OR-0056).
                                for (int quarter = 1; quarter < 4; ++quarter)
                                {
                                    const float yaw = frontYaw + 1.57079632f * static_cast<float>(quarter);
                                    const float out =
                                        GalaxyEggbert::Game::kBoothSideOffsets[static_cast<std::size_t>(quarter)];
                                    drawModel(*still, cell.x + out * std::sin(yaw), cell.y, cell.z + out * std::cos(yaw),
                                              yaw, 0.0f, 1.0f, /*boothFrontOnly=*/true);
                                }
                            }
                            if (signDecals_.count(cell.base) != 0)
                            {
                                decalCells.push_back({cell.base, cell.x, cell.y, cell.z, frontYaw});
                            }
                        }
                        else if (GalaxyEggbert::Game::IsClayBridgeIcon(cell.base))
                        {
                            drawBridgeRows(*still, cell.x, cell.y, cell.z);
                        }
                        else
                        {
                            if (GalaxyEggbert::Game::IsElectrodeIcon(cell.base) && electrodeSparks_.count(cell.base) != 0)
                            {
                                electrodeCells_.push_back({cell.base, cell.x, cell.y, cell.z, 0.0f});
                            }
                            drawModel(*still, cell.x, cell.y, cell.z, 0.0f, 0.0f,
                                      GalaxyEggbert::Game::ModelDepthScale(cell.base));
                            if (GalaxyEggbert::Game::ModelMirroredInDepth(cell.base))
                            {
                                drawModel(*still, cell.x, cell.y, cell.z, 0.0f, 0.0f,
                                          -GalaxyEggbert::Game::ModelDepthScale(cell.base));
                            }
                        }
                    }
                    continue;
                }
                const int frameIcon =
                    GalaxyEggbert::ResolveBlockAnimationIcon(cell.base, worldRuntime_.GetAnimPhase()) - static_cast<int>(cell.base);
                const ObjectModelPose* pose = objectModelTable_.Find(
                    ObjectTextureSource::ObjectM, GalaxyEggbert::Game::kFanModelIcon + std::max(0, frameIcon));
                if (pose != nullptr)
                {
                    // A turned side fan blows along Z (M3D-0005): -X turned to -Z is -90 degrees.
                    drawModel(*pose, cell.x, cell.y, cell.z, cell.openSide == 0 ? -1.57079632f : 0.0f,
                              GalaxyEggbert::Game::FanModelRoll(cell.base));
                }
            }
        }
        for (const auto& cell : bigDecorCells_)
        {
            tintModelCell = SceneEditorSelected(static_cast<int>(TargetKind::BigDecor),
                                                cell.worldX + WorldRuntime::kWorldCenterX, cell.worldY,
                                                cell.worldZ + WorldRuntime::kWorldCenterZ);
            const ObjectModelPose* pose = objectModelTable_.Find(ObjectTextureSource::Explo, cell.icon);
            if (pose != nullptr)
            {
                const float halfBlocks = static_cast<float>(pose->iconPixels > 0 ? pose->iconPixels : 128) / 128.0f;
                drawModel(*pose, cell.worldX, cell.worldY - 0.5f + halfBlocks, cell.worldZ,
                          1.57079632f * static_cast<float>(CellObjectQuarterTurns(cell.worldX, cell.worldY, cell.worldZ)));
            }
        }
    }

    void GalaxyEggbertGame::AppendSceneDecals(SceneFrame& frame)
    {
        // The signs' painted faces (MR-0006): each decal on the sign's front and, mirrored so it reads
        // the right way round, on its back, a hair off the faces.
        if (!signDecals_.empty() && terrainRenderer_)
        {
            frame.signDecalPass = true;
            for (const auto& cell : signDecalCells_)
            {
                const auto found = signDecals_.find(cell.icon);
                if (found == signDecals_.end())
                {
                    continue;
                }
                const SignDecal& decal = found->second;
                const float c = std::cos(cell.yaw), s = std::sin(cell.yaw);
                // Model (x, y, z) to the world: CreateRotationY(yaw), then the cell.
                const auto toWorld = [&](float x, float y, float z) {
                    return Vector3(cell.x + x * c + z * s, cell.y + y, cell.z - x * s + z * c);
                };
                const float midX = (decal.left + decal.right) * 0.5f;
                const float midY = (decal.top + decal.bottom) * 0.5f;
                const Vector2 size(decal.right - decal.left, decal.top - decal.bottom);
                constexpr float kLift = 0.004f;
                const Vector3 right(c, 0.0f, -s);
                const Vector3 up(0.0f, 1.0f, 0.0f);
                SceneDecal sceneDecal;
                sceneDecal.texture = decal.texture.get();
                sceneDecal.quads.push_back(Quad(toWorld(midX, midY, decal.depth + kLift), size, right, up,
                                                CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f}));
                sceneDecal.quads.push_back(Quad(toWorld(-midX, midY, -decal.depth - kLift), size, Vector3(-c, 0.0f, s),
                                                up, CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f}));
                frame.signDecals.push_back(std::move(sceneDecal));
            }
        }

        // The glue death's strands (owner 2026-11-04, Q-006): the painted strands from his hands to the
        // puddle and its peaks, as thin tubes in the glue model's plane (GlueStrands.hpp), in their green
        // from element.png, turned and placed as the glue model is.
        if (glueStrandsPending_ && objectEffect_)
        {
            glueStrandsPending_ = false;
            const int glueFrame = std::clamp(blupi_.GetRawAnimIcon() - 168, 0, 3);
            const auto iconUv = GetElementIconUv(168 + glueFrame);
            const auto& texel = GalaxyEggbert::Game::kGlueTexel[static_cast<std::size_t>(glueFrame)];
            const Vector2 uv(iconUv.U0 + texel[0] * (iconUv.U1 - iconUv.U0), iconUv.V0 + texel[1] * (iconUv.V1 - iconUv.V0));
            const float turn = kPi - blupi_.GetYaw();
            const float c = std::cos(turn), s = std::sin(turn);
            const Vector3 origin(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f, blupi_.GetZ());
            auto mesh = MakeTexturedMeshBuilder();
            GalaxyEggbert::Game::AppendPolylineTubes(
                mesh, GalaxyEggbert::Game::kGluePoints.data(), GalaxyEggbert::Game::kGluePolylines.data(),
                GalaxyEggbert::Game::kGlueFrames[static_cast<std::size_t>(glueFrame)], uv, Vector3(0.0f, 1.0f, 0.0f),
                [&](const Vector3& p) { return Vector3(origin.X + p.X * c + p.Z * s, origin.Y + p.Y, origin.Z - p.X * s + p.Z * c); });
            frame.glueStrands.shown = true;
            frame.glueStrands.mesh = mesh.Mesh();
        }

        // Painted sparks facing the camera over their models (Q-005).
        if (!electrodeCells_.empty() || !dynamiteSparkCells_.empty())
        {
            const auto invView = Matrix::Invert(camera_.View());
            const auto right = invView.getRightProperty();
            const auto up = invView.getUpProperty();
            const auto addSparks = [&](std::vector<SceneDecal>& out, const auto& cells, const auto& textures,
                                       const Vector2& size, const CnaEngine::Graphics::UvRect& uv)
            {
                for (const auto& cell : cells)
                {
                    SceneDecal spark;
                    spark.texture = textures.at(cell.icon).get();
                    spark.quads.push_back(Quad(Vector3(cell.x, cell.y, cell.z), size, right, up, uv));
                    out.push_back(std::move(spark));
                }
            };
            addSparks(frame.electrodeSparks, electrodeCells_, electrodeSparks_, Vector2(1.0f, 1.0f),
                      CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
            // The three stars occupy [38,55) x [44,60) in their 60-pixel cells. Crop the clear margin
            // so only the star rotates about the 3D fuse tip; preserve the sprite's scale (64 original
            // pixels per world block).
            addSparks(frame.dynamiteSparks, dynamiteSparkCells_, dynamiteSparks_, Vector2(17.0f / 64.0f, 16.0f / 64.0f),
                      CnaEngine::Graphics::UvRect{38.0f / 60.0f, 44.0f / 60.0f, 55.0f / 60.0f, 1.0f});
        }
    }

    Texture2D* GalaxyEggbertGame::TeamTexture(int team, Texture2D* source)
    {
        // The editor's player starts and a network game's teams: the original yellow material in
        // the colour (RecolorPlayerStartPixel), a copy made once per texture.
        if (!source || team <= 0 || team > 3) return source;
        auto& textures = editorStartTextures_[static_cast<std::size_t>(team - 1)];
        if (const auto found = textures.find(source); found != textures.end()) return found->second.get();
        using Microsoft::Xna::Framework::Color;
        const int width = source->getWidthProperty(), height = source->getHeightProperty();
        std::vector<Color> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
        source->GetData(pixels.data(), static_cast<int>(pixels.size()));
        const auto color = static_cast<PlayerStartColor>(team);
        for (auto& pixel : pixels)
        {
            std::uint8_t r = pixel.getRProperty(), g = pixel.getGProperty(), b = pixel.getBProperty();
            GalaxyEggbert::Editor::RecolorPlayerStartPixel(color, r, g, b);
            pixel = Color(r, g, b, pixel.getAProperty());
        }
        auto texture = std::make_unique<Texture2D>(getGraphicsDeviceProperty(), width, height);
        texture->SetData(pixels.data(), static_cast<int>(pixels.size()));
        auto* result = texture.get();
        textures.emplace(source, std::move(texture));
        return result;
    }

    void GalaxyEggbertGame::AppendSceneNetPlayers(std::vector<SceneModel>& opaque, std::vector<SceneModel>& translucent)
    {
        // A network game's other Blupis (the original draws them only, decblock.cpp:1252-1282): each
        // as his own machine draws him (Pose: his model's matrix, clip and time), moved between his
        // last two poses, in his team's colour, with the vehicle he rides.
        if (!NetMatchActive() || !blupiModelLoaded_ || !blupiEffect_ || !blupiModel_ ||
            phase_ == GalaxyEggbert::Def::GamePhase::Editor)
            return;
        const auto& n = *net_;
        for (const auto& peer : n.peers)
        {
            const auto& p = peer.pose;
            if (!peer.present || !peer.hasPose || (p.flags & GalaxyEggbert::Net::kPoseHidden) != 0 ||
                p.clip >= n.clipNames.size())
                continue;
            const std::string& clip = n.clipNames[p.clip];
            if (blupiModel_->Clips.count(clip) == 0) continue;
            const auto& q = peer.previous;
            float t = std::clamp(peer.age / std::max(peer.interval, 0.02f), 0.0f, 1.0f);
            const float jump = std::fabs(p.world[9] - q.world[9]) + std::fabs(p.world[10] - q.world[10]) +
                               std::fabs(p.world[11] - q.world[11]);
            if (jump > 3.0f) t = 1.0f; // a new life, a teleport: there at once
            const auto lerp = [t](float a, float b) { return a + (b - a) * t; };
            SceneModel body;
            body.model = blupiModel_.get();
            body.clip = clip;
            body.seconds = static_cast<double>(p.clipSeconds) + std::min(peer.age, 0.25f);
            body.world = Matrix(p.world[0], p.world[1], p.world[2], 0.0f, p.world[3], p.world[4], p.world[5], 0.0f,
                                p.world[6], p.world[7], p.world[8], 0.0f, lerp(q.world[9], p.world[9]),
                                lerp(q.world[10], p.world[10]), lerp(q.world[11], p.world[11]), 1.0f);
            body.alpha = static_cast<float>(p.alpha) / 255.0f;
            body.translucent = body.alpha < 1.0f;
            if ((p.flags & (GalaxyEggbert::Net::kPoseCharred | GalaxyEggbert::Net::kPoseGlued)) != 0)
            {
                body.tinted = true;
                body.tint = (p.flags & GalaxyEggbert::Net::kPoseGlued) != 0 ? Vector3(0.20f, 0.32f, 0.03f)
                                                                            : Vector3(0.07f, 0.07f, 0.07f);
            }
            Texture2D* face = BlupiFaceFor(clip, body.seconds);
            for (const auto& part : blupiModel_->Parts)
                body.partTextures.push_back(TeamTexture(peer.team, face && part.Name == blupiFacePart_ ? face : part.Texture));
            auto& bucket = body.translucent ? translucent : opaque;
            const Matrix world = body.world;
            const float alpha = body.alpha;
            const double seconds = body.seconds;
            bucket.push_back(std::move(body));
            for (const auto& [vehicleName, vehicle] : blupiVehicles_)
            {
                if (vehicle->Clips.find(clip) == vehicle->Clips.end()) continue;
                SceneModel ride;
                ride.model = vehicle.get();
                ride.clip = clip;
                ride.seconds = seconds;
                ride.world = world;
                ride.alpha = alpha;
                ride.translucent = alpha < 1.0f;
                bucket.push_back(std::move(ride));
            }
        }
    }

    void GalaxyEggbertGame::AppendSceneEditorStarts(std::vector<SceneModel>& models)
    {
        // The editor's player starts: Blupi standing, in the start's colour, the selected colour last.
        if (phase_ != GalaxyEggbert::Def::GamePhase::Editor || worldEditor_.IsBrowsing() || !blupiModelIsReal_ ||
            !blupiModel_ || !blupiEffect_ || blupiModel_->Clips.count("Stop") == 0)
            return;
        auto& device = getGraphicsDeviceProperty();
        using Microsoft::Xna::Framework::Color;
        auto starts = CollectPlayerStarts(worldRuntime_.GetWorld());
        if (starts.empty()) return;
        const auto target = worldEditor_.CurrentEditTarget(worldRuntime_.GetWorld());
        if (target && target->kind == GalaxyEggbert::Editor::WorldEditor::EditTarget::Kind::PlayerStart)
            std::stable_partition(starts.begin(), starts.end(),
                                  [&](const auto& start) { return start.color != target->color; });
        Texture2D* face = nullptr;
        if (const auto faces = blupiClipFaces_.find("Stop"); faces != blupiClipFaces_.end() && !faces->second.empty())
        {
            const int index = faces->second.front();
            if (index >= 0 && index < static_cast<int>(blupiFaces_.size())) face = &blupiFaces_[index];
        }
        for (const auto& start : starts)
        {
            const auto coloredTexture = [&](Texture2D* source) -> Texture2D* {
                return TeamTexture(static_cast<int>(start.color), source);
            };
            SceneModel model;
            model.model = blupiModel_.get();
            model.clip = "Stop";
            model.seconds = 0.0;
            model.loop = false;
            model.world = Matrix::CreateScale(kRealModelScale) * Matrix::CreateRotationY(3.14159265f - PlayerStartYaw(start)) *
                          Matrix::CreateTranslation(float(start.x) - WorldRuntime::kWorldCenterX, float(start.y) - .5f,
                                                    float(start.z) - WorldRuntime::kWorldCenterZ);
            model.partTextures = PartTextures(*blupiModel_, nullptr, blupiFacePart_,
                                              [&](Texture2D*) -> Texture2D* { return nullptr; });
            for (std::size_t i = 0; i < blupiModel_->Parts.size(); ++i)
            {
                const auto& part = blupiModel_->Parts[i];
                model.partTextures[i] = coloredTexture(face && part.Name == blupiFacePart_ ? face : part.Texture);
            }
            // The model itself stays opaque; selection is the same red overlay as other objects.
            model.editorSelected = target && target->kind == GalaxyEggbert::Editor::WorldEditor::EditTarget::Kind::PlayerStart &&
                                   target->color == start.color;
            models.push_back(std::move(model));
        }
    }

    void GalaxyEggbertGame::AppendSceneBillboards(SceneFrame& frame)
    {
        const auto invView = Matrix::Invert(camera_.View());
        const auto cameraRight = invView.getRightProperty();
        const auto cameraUp = invView.getUpProperty();
        const auto& objects = worldRuntime_.GetMobileObjects();
        constexpr float kObjectSize = 1.0f;
        const Vector2 objectSize(kObjectSize, kObjectSize);

        // element.png: the MoveObjects shown as billboards (ObjectDefinitionRegistry's Element source),
        // each on its per-instance phase; the pieces flying away without a model; his death sprite,
        // aura and smoke; then the drink's bubbles.
        if (objectEffect_ &&
            (!objects.empty() || BlupiDrawnAsDeathSprite() || BlupiCharredShape().smokes ||
             BlupiAuraShown() != GalaxyEggbert::Def::SecretPower::None))
        {
            SceneBillboardBatch batch;
            batch.sheet = SceneSheet::Element;
            for (std::size_t index = 0; index < objects.size(); ++index)
            {
                const auto& obj = objects[index];
                // Lifts and crates are cubes; object-m.png-, explo.png- and blupi.png-sourced types
                // have their own batches. ResolveObjectVisual() is phase-aware because ObjectType38
                // spends part of its cycle on element.png.
                const int objPhase = static_cast<int>(obj.phase);
                const auto visual = ResolveObjectVisual(obj.type, objPhase, obj.visualIcon);
                if (!obj.active || !visual.visible || visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::Element || ParkedVehicleModel(obj.type).model != nullptr)
                {
                    continue;
                }
                // The 7 patrol enemies' per-direction/turn-transition icon tables (ENEMY-013...).
                const bool patrolGoesLeftFromStart = obj.posStartX > obj.posEndX;
                const int patrolTimeTicks = static_cast<int>(obj.patrolTime);
                int icon;
                if (!TryGetPatrolIcon(obj.type, patrolGoesLeftFromStart, obj.patrolStep, patrolTimeTicks, icon))
                {
                    icon = visual.icon;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Element, icon) != nullptr)
                {
                    continue; // drawn as its 3D model
                }
                batch.quads.push_back(Quad(Vector3(obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                                           objectSize, cameraRight, cameraUp, Uv(GetElementIconUv(icon)),
                                           static_cast<int>(index), icon));
            }
            // A piece flying away without a model (BD-0049): its sprite, turned as the original's.
            for (const auto& piece : interaction_.ByeByePieces())
            {
                ObjectTextureSource sheet{};
                int icon = 0;
                if (piece.helicopter || piece.fanIcon >= 0 || !ObjectSprite(piece.look, sheet, icon) ||
                    sheet != ObjectTextureSource::Element || objectModelTable_.Find(sheet, icon) != nullptr)
                {
                    continue;
                }
                const float angle = piece.rotationDegrees * 3.14159265f / 180.0f;
                const auto right = cameraRight * std::cos(angle) - cameraUp * std::sin(angle);
                const auto up = cameraRight * std::sin(angle) + cameraUp * std::cos(angle);
                batch.quads.push_back(Quad(Vector3(piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z),
                                           objectSize, right, up, Uv(GetElementIconUv(icon))));
            }
            // His own death sprite (BlupiDrawnAsDeathSprite()): the 60 px icon, its centre 30 px over his
            // feet (half a cell under GetY()), as the original draws it.
            if (BlupiDrawnAsDeathSprite())
            {
                batch.quads.push_back(Quad(Vector3(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f, blupi_.GetZ()),
                                           Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight, cameraUp,
                                           Uv(GetElementIconUv(blupi_.GetDisplayAnimIcon()))));
            }
            // The shield's orb (table_shield_blupi, 2 px higher) and sparkles (table_shieldloop), the
            // power's stars (table_magicloop), a frame every 2 ticks, on his sprite's place, a little
            // behind him from the camera: the original draws them before him (Decor.cpp:782-806).
            if (const auto aura = BlupiAuraShown();
                aura == GalaxyEggbert::Def::SecretPower::Shield || aura == GalaxyEggbert::Def::SecretPower::Power)
            {
                static constexpr int kShieldBlupi[16] = {144, 145, 146, 147, 148, 149, 150, 151,
                                                         266, 267, 268, 269, 270, 271, 272, 273};
                static constexpr int kShieldLoop[5] = {274, 275, 276, 277, 278};
                static constexpr int kMagicLoop[5] = {152, 153, 154, 155, 156};
                const int auraFrame = worldRuntime_.GetAnimPhase() / 2;
                const auto behind = invView.getForwardProperty() * 0.35f;
                const Vector3 centre(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f, blupi_.GetZ());
                const auto add = [&](int icon, float raise)
                {
                    batch.quads.push_back(Quad(centre + behind + Vector3(0.0f, raise, 0.0f), Vector2(60.0f / 64.0f, 60.0f / 64.0f),
                                               cameraRight, cameraUp, Uv(GetElementIconUv(icon))));
                };
                if (aura == GalaxyEggbert::Def::SecretPower::Shield)
                {
                    add(kShieldBlupi[auraFrame % 16], 2.0f / 64.0f);
                    add(kShieldLoop[auraFrame % 5], 0.0f);
                }
                else
                {
                    add(kMagicLoop[auraFrame % 5], 0.0f);
                }
            }
            // The smoke over his heap of ash (46/47, CharredBlupi.hpp): the icon's rows above the heap,
            // its bottom where the sprite's would be over the model's feet.
            if (const auto charred = BlupiCharredShape(); charred.smokes)
            {
                const auto uv = GetElementIconUv(blupi_.GetRawAnimIcon());
                constexpr float kRows = static_cast<float>(GalaxyEggbert::Game::kCharredSmokeRows);
                const float feetY = blupi_.GetY() - 0.5f;
                const float bottom = feetY + GalaxyEggbert::Game::kCharredSmokeBottomPixels / 64.0f;
                batch.quads.push_back(Quad(Vector3(blupi_.GetX(), bottom + kRows / 128.0f, blupi_.GetZ()),
                                           Vector2(60.0f / 64.0f, kRows / 64.0f), cameraRight, cameraUp,
                                           CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V0 + (uv.V1 - uv.V0) * kRows / 60.0f}));
            }
            frame.billboards.push_back(std::move(batch));

            // The drink's bubbles (Decor.cpp:882-895): over each bottle three bubbles (element
            // 274 + n % 5) rise 3 px a tick for 50 ticks, a third apart (table_drinkoffset 0, 7, 22),
            // fading from whole to nothing. The fade in five steps: one batch and alpha each.
            static constexpr int kDrinkOffsets[3] = {0, 7, 22};
            constexpr int kFadeSteps = 5;
            std::array<SceneBillboardBatch, kFadeSteps> fades;
            for (int k = 0; k < kFadeSteps; ++k)
            {
                fades[static_cast<std::size_t>(k)].sheet = SceneSheet::Element;
                // (50 - n) / 50 at the step's middle.
                fades[static_cast<std::size_t>(k)].alpha =
                    1.0f - (static_cast<float>(k) + 0.5f) / static_cast<float>(kFadeSteps);
            }
            for (const auto& obj : objects)
            {
                if (!obj.active || obj.type != GalaxyEggbert::Def::ObjectType::ObjectType30)
                {
                    continue;
                }
                const float centreY = ResolveObjectVisualCenterY(obj.type, obj.currentY);
                for (const int offset : kDrinkOffsets)
                {
                    const int n = (worldRuntime_.GetAnimPhase() + offset) % 50;
                    const int step = std::min(kFadeSteps - 1, n * kFadeSteps / 50);
                    fades[static_cast<std::size_t>(step)].quads.push_back(
                        Quad(Vector3(obj.currentX, centreY + static_cast<float>(n) * 3.0f / 64.0f, obj.currentZ) +
                                 cameraRight * (2.0f / 64.0f),
                             Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight, cameraUp, Uv(GetElementIconUv(274 + n % 5))));
                }
            }
            for (auto& fade : fades)
            {
                if (!fade.quads.empty())
                {
                    frame.billboards.push_back(std::move(fade));
                }
            }
        }

        // object-m.png: the MoveObjects shown from the terrain's sheet (tileAtlas_ UVs).
        if (objectMPngEffect_ && !objects.empty())
        {
            SceneBillboardBatch batch;
            batch.sheet = SceneSheet::ObjectM;
            for (std::size_t index = 0; index < objects.size(); ++index)
            {
                const auto& obj = objects[index];
                const auto visual = ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                if (!obj.active || !visual.visible || visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::ObjectM)
                {
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::ObjectM, visual.icon) != nullptr)
                {
                    continue; // drawn as its 3D model
                }
                batch.quads.push_back(Quad(Vector3(obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                                           objectSize, cameraRight, cameraUp, tileAtlas_.GetTileUv(visual.icon),
                                           static_cast<int>(index), visual.icon));
            }
            frame.billboards.push_back(std::move(batch));
        }

        // explo.png: the explosion channel's MoveObjects, and the Cloud's sparks round him.
        if (exploEffect_ && (!objects.empty() || BlupiAuraShown() == GalaxyEggbert::Def::SecretPower::Cloud))
        {
            SceneBillboardBatch batch;
            batch.sheet = SceneSheet::Explo;
            // The Cloud's three sparks round him (explo.png 48 + (t + k) % 6, 128 px centred on his
            // sprite: pos - 34, Decor.cpp:807-818), a little behind him as the original draws them first.
            if (BlupiAuraShown() == GalaxyEggbert::Def::SecretPower::Cloud)
            {
                const auto behind = invView.getForwardProperty() * 0.35f;
                const Vector3 centre(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f, blupi_.GetZ());
                for (int k = 0; k < 3; ++k)
                {
                    batch.quads.push_back(Quad(centre + behind, Vector2(128.0f / 64.0f, 128.0f / 64.0f), cameraRight, cameraUp,
                                               Uv(GetExploIconUv(48 + (worldRuntime_.GetAnimPhase() + k) % 6))));
                    batch.quads.back().glow = kShiningSpriteGlow;
                }
            }
            for (std::size_t index = 0; index < objects.size(); ++index)
            {
                const auto& obj = objects[index];
                const auto visual = ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                if (!obj.active || visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::Explo)
                {
                    continue;
                }
                // The -1 "invisible frame" sentinel (VISUAL-009, table_sploutch2/3's leading delay
                // ticks): nothing this tick, as the original's blank frame.
                if (!visual.visible)
                {
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Explo, visual.icon) != nullptr)
                {
                    continue; // drawn as its 3D model
                }
                batch.quads.push_back(Quad(Vector3(obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                                           objectSize, cameraRight, cameraUp, Uv(GetExploIconUv(visual.icon)),
                                           static_cast<int>(index), visual.icon));
                // Ultra: an explosion's fire and the sparks and stars shine (SceneQuad::glow); splashes,
                // smoke and the rest are lit.
                ParticleKind kind{};
                if (EffectParticleKind(obj.type, kind) && (kind == ParticleKind::Fire || kind == ParticleKind::Sparkle))
                {
                    batch.quads.back().glow = kShiningSpriteGlow;
                }
            }
            frame.billboards.push_back(std::move(batch));
        }

        // blupi.png (ObjectType200) and blupi1.png (201-203): two batches, one texture each.
        if (blupiObjectEffect_ && blupi1ObjectEffect_ && !objects.empty())
        {
            SceneBillboardBatch blupiBatch;
            blupiBatch.sheet = SceneSheet::Blupi;
            SceneBillboardBatch blupi1Batch;
            blupi1Batch.sheet = SceneSheet::Blupi1;
            for (std::size_t index = 0; index < objects.size(); ++index)
            {
                const auto& obj = objects[index];
                const auto visual = ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                if (!obj.active || !visual.visible || visual.renderMode != ObjectRenderMode::Billboard ||
                    (visual.textureSource != ObjectTextureSource::Blupi && visual.textureSource != ObjectTextureSource::Blupi1))
                {
                    continue;
                }
                // The enemy Blupis' patrol tables (ObjectSprite), unless drawn as his model.
                ObjectTextureSource sheet{};
                int icon = visual.icon;
                if (EnemyBlupiModelDrawn(obj) || !ObjectSprite(obj, sheet, icon) ||
                    objectModelTable_.Find(visual.textureSource, icon) != nullptr)
                {
                    continue; // drawn as its 3D model
                }
                auto& batch = visual.textureSource == ObjectTextureSource::Blupi1 ? blupi1Batch : blupiBatch;
                batch.quads.push_back(Quad(Vector3(obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                                           objectSize, cameraRight, cameraUp, Uv(GetBlupiIconUv(icon)),
                                           static_cast<int>(index), icon));
            }
            frame.billboards.push_back(std::move(blupiBatch));
            frame.billboards.push_back(std::move(blupi1Batch));
        }

        // The big decor (BigDecor: cells) on explo.png, the source's CHEXPLO layer; cells from a mobile
        // BigDecor: grid or sparse 3D .vwr BigDecorRecord metadata.
        if (bigDecorEffect_ && !bigDecorCells_.empty())
        {
            SceneBillboardBatch batch;
            batch.sheet = SceneSheet::BigDecor;
            constexpr float kBigDecorSize = 1.0f;
            for (const auto& cell : bigDecorCells_)
            {
                if (cell.icon == GalaxyEggbert::BlockTypes::Marine)
                {
                    // BigDecor icon 203 is not an explo.png icon: the original draws the object
                    // channel's marine animation there (Decor.cpp: `num2 = table_marine[m_time /
                    // ScaleDiv(3) % 11]; channel = Object`), the Marine block's animation.
                    float u = 0.0f, v = 0.0f, us = 0.0f, vs = 0.0f;
                    GalaxyEggbert::BlockTypes::tileUV(
                        GalaxyEggbert::ResolveBlockAnimationIcon(GalaxyEggbert::BlockTypes::Marine, worldRuntime_.GetAnimPhase()),
                        u, v, us, vs);
                    frame.marine.push_back(Quad(Vector3(cell.worldX, cell.worldY, cell.worldZ),
                                                Vector2(kBigDecorSize, kBigDecorSize), cameraRight, cameraUp,
                                                CnaEngine::Graphics::UvRect{u, v, u + us, v + vs}));
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Explo, cell.icon) != nullptr)
                {
                    continue; // drawn as its 3D model
                }
                batch.quads.push_back(Quad(Vector3(cell.worldX, cell.worldY, cell.worldZ), Vector2(kBigDecorSize, kBigDecorSize),
                                           cameraRight, cameraUp, Uv(GetBigDecorIconUv(static_cast<int>(cell.icon)))));
            }
            frame.billboards.push_back(std::move(batch));
        }
    }
}
