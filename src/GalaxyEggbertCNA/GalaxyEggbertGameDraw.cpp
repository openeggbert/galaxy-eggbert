// GalaxyEggbertGame's frame: the 3D world (DrawWorld), the HUD, menus and editor over it
// (DrawOverlay) and the frame captures (DrawFrameCaptures). Split out of GalaxyEggbertGame.cpp.
#include "GalaxyEggbertGame.hpp"

#include <GalaxyEggbert/Game/WaterSurface.hpp>
#include <GalaxyEggbert/Game/CheatConfig.hpp>
#include <GalaxyEggbert/Game/DebugOverlay.hpp>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include <GalaxyEggbert/Game/GamepadInput.hpp>
#include <GalaxyEggbert/Game/BarRows.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/BoltMesh.hpp>
#include <GalaxyEggbert/Game/EnemyBlupi.hpp>
#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>
#include <GalaxyEggbert/Game/GlueStrands.hpp>

#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <GalaxyEggbert/Editor/CustomWorldStorage.hpp>
#include <GalaxyEggbert/Game/BrickEdges.hpp>
#include <GalaxyEggbert/Editor/PlatformFiles.hpp>
#include <GalaxyEggbert/Editor/PlayerStartColor.hpp>
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include "GalaxyEggbert/BlockTypes.hpp"
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include "GalaxyEggbert/Worlds/Block.hpp"

#include <GalaxyEggbert/HubDoors.hpp>
#include <GalaxyEggbert/Game/LiftVisual.hpp>
#include <GalaxyEggbert/Game/ObjectVerticalPlacement.hpp>
#include <GalaxyEggbert/Game/TerrainLighting.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMeshPart.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>

#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>

#include <algorithm>
#include <Microsoft/Xna/Framework/BoundingFrustum.hpp>
#include <Microsoft/Xna/Framework/BoundingSphere.hpp>
#include <Microsoft/Xna/Framework/ContainmentType.hpp>
#include <chrono>
#include <unordered_set>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <vector>
#include "GameInternal.hpp"
#include <GalaxyEggbert/Game/WorldRenderer.hpp>

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::CNA::detail;

    void GalaxyEggbertGame::Draw(const Microsoft::Xna::Framework::GameTime& gameTime)
    {
        (void)gameTime;
        if (replayingResume_) return;
        const auto perfStart = std::chrono::steady_clock::now();
        DrawFrame();
        if (perfEnabled_)
        {
            perfDrawSeconds_ += std::chrono::duration<double>(std::chrono::steady_clock::now() - perfStart).count();
            if (++perfFrames_ >= 120)
            {
                std::cout << "GalaxyEggbertCNA perf: draw " << perfDrawSeconds_ * 1000.0 / perfFrames_
                          << " ms, models " << perfModelDraws_ / perfFrames_ << " ("
                          << perfModelTriangles_ / perfFrames_ << " triangles), culled "
                          << perfModelCulled_ / perfFrames_ << " a frame" << std::endl;
                perfFrames_ = 0;
                perfDrawSeconds_ = 0.0;
                perfModelDraws_ = perfModelTriangles_ = perfModelCulled_ = 0;
            }
        }
    }

    void GalaxyEggbertGame::DrawFrame()
    {
        auto& device = getGraphicsDeviceProperty();
        // Root-caused 2026-07-13 (NEXT.md §5's "screenshot_hud.png blue-
        // background anomaly", Vulkan-only): calling GetBackBufferData()
        // mid-frame -- as the terrain-visibility diagnostic below does --
        // corrupts the REST of that same frame's rendering on CNA's Vulkan
        // backend specifically (subsequent billboards lose alpha blending
        // and render as opaque white boxes, and the terrain itself reads
        // back as flat blue by the time the end-of-frame HUD screenshot
        // below captures it). Confirmed by temporarily disabling the
        // mid-frame readback: the HUD screenshot rendered perfectly on
        // Vulkan once that mid-frame GetBackBufferData call was removed.
        // Not fixable here (the bug is inside ../cna's Vulkan backend,
        // which needs separate approval to modify) -- worked around by
        // simply never letting both one-shot diagnostics fire within the
        // same frame: drawFrameIndex_ lets the HUD screenshot wait for a
        // frame strictly after whichever frame the terrain diagnostic
        // fired on (a fresh frame that never called GetBackBufferData
        // mid-draw), so it always captures an uncorrupted frame.
        ++drawFrameIndex_;

        // Dark navy (2026-07-18, user-reported: pillarbox margins around
        // Wait's/Init's menu screens should be dark blue, not the generic
        // XNA/MonoGame "CornflowerBlue" template default this was
        // previously left at). Matches Content/backgrounds/init.png's own
        // corner pixel color exactly (sampled directly: RGB(0,35,98)), so
        // the pillarbox seamlessly matches that background's own edge tone
        // instead of showing a visible seam between a lighter placeholder
        // blue and the actual (darker) menu artwork.
        //
        // Clears the DEPTH buffer as well, and must keep doing so: this is the
        // only Clear() in the whole frame, so without it stale depth values
        // survive from frame to frame and every depth-tested draw below --
        // terrain, billboards, the background quad, the whole 3D world -- gets
        // rejected, leaving just this clear colour under the depth-test-free
        // HUD (`SetDepthTestEnabled(false)` further down).
        //
        // That is exactly what happened (user-reported 2026-08-21: "nevidim 3d
        // svet, misto toho modrou barvu"). This used to read
        // `Clear(0.0f, 0.1373f, 0.3843f, 1.0f)`, and on EasyGL that four-float
        // overload cleared depth too -- as an undocumented side effect the
        // engine has since removed on purpose (`../cna` REMED-GFX-142,
        // 2026-07-28: "COLOUR ONLY ... it used to add ClearFlags::Depth -- so
        // asking XNA to clear only the colour target silently wiped the depth
        // buffer with it"). Upstream is right and this code was the one leaning
        // on the side effect, so the fix belongs here.
        //
        // The single-argument overload is real XNA's own
        // `Target | DepthBuffer | Stencil` at the viewport's MaxDepth, so it
        // states the intent instead of re-deriving flags. RGB(0,35,98) is the
        // same colour as the float triple it replaces (0.1373*255 = 35,
        // 0.3843*255 = 98), still init.png's sampled corner pixel.
        device.Clear(Microsoft::Xna::Framework::Color(0, 35, 98));
        device.SetDepthTestEnabled(true);

        // Reported live (2026-07-13): the 3D world was visible bleeding
        // through the pillarbox margins around Wait's loading-gauge screen
        // and Init's gamer-select menu -- both occur before any Play
        // session exists (Wait is a fake progress screen since
        // LoadContent() already loaded everything up front; Init is the
        // gamer-select menu, entered before any level/world is actually
        // being played), so unlike Pause/Win/Lost/Setup/Resume (which
        // legitimately freeze and show the in-progress game world behind
        // their overlay), there is no real game world to show here at all.
        // Skipping this whole block for those two phases removes the
        // bleed-through entirely instead of just papering over it with an
        // opaque quad. The Editor phase's own browser screen (plan.md
        // EDITOR-107, worldEditor_.IsBrowsing()) is the same situation --
        // no world is loaded yet while browsing -- so it's skipped too;
        // once a world is actually being edited, IsBrowsing() is false and
        // this block draws it same as any other phase.
        // The menu (Init) shows the world when it is Galaxy's title scene.
        // The Clear2 death's soul drawn as its 3D model this frame: the HUD leaves out its sprite.
        bool soulDrawnIn3D = false;
        if ((phase_ != GalaxyEggbert::Def::GamePhase::Wait || titleSceneActive_) &&
            (phase_ != GalaxyEggbert::Def::GamePhase::Init || titleSceneActive_) &&
            !(phase_ == GalaxyEggbert::Def::GamePhase::Editor && worldEditor_.IsBrowsing() && !titleSceneActive_))
        {
            // What to draw (GalaxyEggbertGameScene.cpp); with the 3D effects, first the sun's shadow map
            // (it has its own target, so before the frame's).
            BuildSceneFrame(sceneFrame_, soulDrawnIn3D);
            const auto assets = SceneAssets();
            worldRenderer_.RenderShadowMap(device, sceneFrame_, assets);
            // The 3D effects' finish (PostProcess.hpp): the world into a render target, then onto the
            // screen with its glow and grade; without them, or where the device cannot, straight on.
            const auto& effects = sceneFrame_.effects;
            const bool finish = (effects.smoothing || effects.glow) &&
                                postProcess_.Begin(device, Microsoft::Xna::Framework::Color(0, 35, 98));
            if (!finish && sceneFrame_.effects.enabled)
            {
                device.Clear(Microsoft::Xna::Framework::Color(0, 35, 98)); // the screen again after the shadow map
            }
            if (sceneFrame_.effects.enabled)
            {
                device.SetDepthTestEnabled(true);
            }
            worldIntoTarget_ = finish;
            DrawWorld(device, assets);
            worldIntoTarget_ = false;
            if (finish)
            {
                postProcess_.End(device, effects.glow);
            }
        }

        // Restore opaque state after the AlphaBlend block above (billboards
        // only) -- Hud manages its own blend state, but this keeps device
        // state predictable for anything drawn after this point.
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);

        // HUD quads must not be depth-tested against the 3D scene (Draw()
        // re-enables depth at the top of every frame regardless).
        device.SetDepthTestEnabled(false);

        DrawOverlay(device, soulDrawnIn3D);
        DrawFrameCaptures(device);
    }

    GalaxyEggbert::Game::WorldRenderAssets GalaxyEggbertGame::SceneAssets()
    {
        GalaxyEggbert::Game::WorldRenderAssets assets;
        assets.background = backgroundLoaded_ ? backgroundEffect_.get() : nullptr;
        assets.terrain = terrainEffect_.get();
        assets.grass = grassEffect_.get();
        assets.cubes = objectCubeEffect_.get();
        assets.models = blupiEffect_.get();
        assets.element = objectEffect_.get();
        assets.objectM = objectMPngEffect_.get();
        assets.explo = exploEffect_.get();
        assets.blupi = blupiObjectEffect_.get();
        assets.blupi1 = blupi1ObjectEffect_.get();
        assets.bigDecor = bigDecorEffect_.get();
        assets.terrainRenderer = terrainRenderer_.get();
        return assets;
    }

    void GalaxyEggbertGame::DrawWorld(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                      const GalaxyEggbert::Game::WorldRenderAssets& assets)
    {
        // The renderer draws the frame built (BuildSceneFrame) from the game's content (SceneAssets).
        worldRenderer_.ResetStatistics();
        worldRenderer_.Draw(device, sceneFrame_, assets,
                            [this](Microsoft::Xna::Framework::Graphics::GraphicsDevice& drawn) { CheckTerrainVisibility(drawn); });
        const auto& statistics = worldRenderer_.Statistics();
        perfModelDraws_ += statistics.modelDraws;
        perfModelTriangles_ += statistics.modelTriangles;
        perfModelCulled_ += statistics.modelsCulled;
    }

    // A one-shot check that the terrain is drawn textured (and screenshot.png of it), once the terrain and
    // every model are drawn and before the billboards.
    void GalaxyEggbertGame::CheckTerrainVisibility(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        if (worldIntoTarget_)
        {
            return; // the backbuffer cannot be read while the world goes into the effects' target
        }
        static bool terrainPixelPrinted = false;
        if (!terrainPixelPrinted)
        {
            terrainPixelPrinted = true;
            terrainPixelPrintedFrame_ = drawFrameIndex_;
            // Backbuffer, not Viewport -- see GetBackBufferSize()'s comment. This site
            // needs it for BOTH readbacks below: the whole-backbuffer capture, and the
            // 5x5 single-pixel grid, whose sample rectangles CNA validates against the
            // backbuffer bounds (a Viewport wider than the backbuffer made those throw
            // std::out_of_range instead).
            int w = 0, h = 0;
            GetBackBufferSize(device, w, h);

            // Sample a 5x5 grid over the central 60% of the screen —
            // more robust than one center pixel, which can miss terrain
            // if the exact screen center happens to fall between blocks.
            int nonBackgroundCount = 0;
            std::set<std::uint32_t> distinctTerrainColors;
            constexpr int kGridSize = 5;
            for (int gy = 0; gy < kGridSize; ++gy)
            {
                for (int gx = 0; gx < kGridSize; ++gx)
                {
                    const int px = w / 2 + (gx - kGridSize / 2) * (w / 10);
                    const int py = h / 2 + (gy - kGridSize / 2) * (h / 10);
                    const Microsoft::Xna::Framework::Rectangle sample(px, py, 1, 1);
                    Microsoft::Xna::Framework::Color pixel(0, 0, 0, 0);
                    device.GetBackBufferData(&sample, &pixel, 0, 1);
                    // Sky-blue clear color is (100, 149, 237); anything
                    // clearly different is terrain. Note (2026-07-09):
                    // when a real background image is loaded
                    // (backgroundLoaded_), sampled points can also
                    // legitimately differ from this flat reference by
                    // showing real sky/background art instead of solid
                    // blue -- this diagnostic's "non-background" count
                    // is a looser signal in that case (it can no longer
                    // assume everything non-blue is terrain), but is
                    // still useful together with distinctTerrainColors:
                    // real terrain plus a real photographic-ish
                    // background both produce many distinct colors,
                    // while a genuinely broken texture sample (a flat
                    // fallback tint) would not.
                    if (std::abs(static_cast<int>(pixel.getRProperty()) - 100) > 10 ||
                        std::abs(static_cast<int>(pixel.getGProperty()) - 149) > 10 ||
                        std::abs(static_cast<int>(pixel.getBProperty()) - 237) > 10)
                    {
                        ++nonBackgroundCount;
                        const std::uint32_t packed =
                            (static_cast<std::uint32_t>(pixel.getRProperty()) << 16) |
                            (static_cast<std::uint32_t>(pixel.getGProperty()) << 8) |
                            static_cast<std::uint32_t>(pixel.getBProperty());
                        distinctTerrainColors.insert(packed);
                    }
                }
            }
            std::cout << "GalaxyEggbertCNA: terrain visibility check — "
                      << nonBackgroundCount << "/" << (kGridSize * kGridSize)
                      << " sampled screen points show non-background (terrain) color, "
                      << distinctTerrainColors.size() << " distinct color(s) among them"
                      << " (>1 means the texture is actually being sampled, not a flat fallback)."
                      << std::endl;

            // One-shot full-frame screenshot for manual/visual
            // verification (2026-07-08, DirectionalCube render mode,
            // NEXT.md §8 task 1) -- the pixel-sample check above proves
            // terrain is textured, but not that a specific tile's
            // geometry (e.g. an open top/bottom face) looks right; a
            // real image is the only way to confirm that by eye.
            std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                Microsoft::Xna::Framework::Color(0, 0, 0, 0));
            device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
            Microsoft::Xna::Framework::Graphics::Texture2D screenshot(device, w, h);
            screenshot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
            screenshot.SaveAsPng("screenshot.png");
            std::cout << "GalaxyEggbertCNA: wrote screenshot.png (" << w << "x" << h << ")."
                      << std::endl;
        }
    }

    void GalaxyEggbertGame::DrawOverlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                        bool soulDrawnIn3D)
    {
        if (phase_ == GalaxyEggbert::Def::GamePhase::Play)
        {
            const auto& viewport = device.getViewportProperty();
            const auto& eye = camera_.Position();
            hud_.DrawUnderwaterTint(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                GalaxyEggbert::Game::WaterViewImmersion(worldRuntime_.GetWorld(),
                    eye.X + WorldRuntime::kWorldCenterX, eye.Y, eye.Z + WorldRuntime::kWorldCenterZ));
        }

        // Real mobile-eggbert bottom HUD + interim animation-state
        // indicator (2026-07-10, see Hud.hpp for the full layout AND for
        // why this is real 3D quads instead of SpriteBatch -- CNA's Vulkan
        // backend records every SpriteBatch batch before every 3D draw, so
        // a sprite HUD is always painted over by the scene; that was the
        // real cause of the twice-reported "icon visible for a second,
        // then gone" bug, which the earlier depth-test fix addressed only
        // for EasyGL). Drawn LAST so it wins submission order on both
        // backends.
        {
            const auto& viewport = device.getViewportProperty();
            // Pause/Win/Lost (2026-07-13, plan.md MENU-028..039/046..057):
            // the real HUD is skipped entirely, replaced by
            // inputPad_.DrawPause()/DrawWinLost()'s real background/
            // character/buttons below -- fully hiding the normal HUD
            // outside Play via a dedicated draw call instead of a generic
            // text message.
            // Whether the training hint's banner (Hud, the top 40 reference pixels) is up: Galaxy's own
            // notices are drawn under it then (the cheats keep the original's place).
            bool trainingHintShown = false;
            const bool phaseHasRealScreen = phase_ == GalaxyEggbert::Def::GamePhase::Pause ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Win ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Lost ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::PlaySetup ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::MainSetup ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Resume ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Wait ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Init ||
                                             phase_ == GalaxyEggbert::Def::GamePhase::Editor;
            if (!phaseHasRealScreen)
            {
                // Training-hint lookup (plan.md HUD-024): real grid position
                // (not render-centered, WorldRuntime::kWorldCenterX/Z offset
                // reversed, matching every other grid<->render conversion this
                // session).
                const int hintGridX = static_cast<int>(std::lround(blupi_.GetX())) + WorldRuntime::kWorldCenterX;
                const int hintGridZ = static_cast<int>(std::lround(blupi_.GetZ())) + WorldRuntime::kWorldCenterZ;
                // A 3D world that places its own hint zones (HintZoneRecord)
                // uses them; otherwise the original tables' 2D rects apply.
                const int hintGridY = static_cast<int>(std::floor(blupi_.GetY()));
                const auto& hintZones = worldRuntime_.GetHintZones();
                // The accelerometer's texts with it on (Decor.cpp:1287-1291).
                const bool hintAccel = saveData_.GetAccelActive();
                const std::string_view trainingHint = hintZones.empty()
                    ? FindTrainingHint(worldRuntime_.GetMissionNumber(), hintGridX, hintGridZ,
                                       interaction_.TreasuresCollected(), blupi_.IsInVehicle(),
                                       interaction_.DynamiteCount() > 0, hintAccel)
                    : FindTrainingHintInZones(worldRuntime_.GetMissionNumber(), hintZones, hintGridX, hintGridY,
                                              hintGridZ, interaction_.TreasuresCollected(), blupi_.IsInVehicle(),
                                              interaction_.DynamiteCount() > 0, hintAccel);
                trainingHintShown = !trainingHint.empty();
                // Real treasure-counter hub gate (SCORE-017, Decor.cpp:1236)
                // -- same "mission != 1 && mission % 10 != 0" expression
                // already used for the Pause-menu showRestart gate above.
                const int hudMission = worldRuntime_.GetMissionNumber();
                const bool showTreasureCounter = hudMission != 1 && hudMission % 10 != 0;
                hud_.Draw(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                          interaction_.Lives(),
                          interaction_.Key1Count() > 0, interaction_.Key2Count() > 0,
                          interaction_.Key3Count() > 0,
                          interaction_.TreasuresCollected(), interaction_.TotalTreasures(), showTreasureCounter,
                          interaction_.BulletCount(), interaction_.DynamiteCount(), interaction_.PersoCount(),
                          blupi_.IsNage(), blupi_.GetWaterGaugeLevel(),
                          // The yellow gauge shows the shared count for every timed state: a power,
                          // Invert, the balloon, the squash (Decor.cpp:5574, 5852, 6023, 6046:
                          // m_jauges[1].SetHide(false) as each starts).
                          blupi_.GetSecretPower() != GalaxyEggbert::Def::SecretPower::None || blupi_.IsInverted() ||
                              blupi_.IsBallooned() || blupi_.IsEcrased(),
                          blupi_.GetTimeShield(),
                          trainingHint,
                          PhaseOverlayMessage(),
                          cameraMode_ == CameraMode::FirstPerson ? blupi_.GetDisplayAnimIcon() : -1,
                          blupi_.AnimIconUsesElementSheet(),
                          // Not while the lava death's ascent holds (VoyageDraw draws icon 40 only
                          // once it moves, Decor.cpp:10328-10331; OR-0015): Blupi's own charred
                          // sprite is still on the spot then.
                          interaction_.VoyageActive() && interaction_.VoyageIconVisible() && !soulDrawnIn3D,
                          interaction_.VoyageIconId(),
                          interaction_.VoyageIsButtonChannel(), interaction_.VoyageIsBlupiChannel(),
                          interaction_.VoyageDrawX(), interaction_.VoyageDrawY(),
                          gameSpeed_, blupi_.GetDrawOpacity(worldRuntime_.GetAnimPhase()),
                          cheatCompass_ && GalaxyEggbert::Game::kCheatsEnabled
                              ? InteractionSystem::NearestObjective(
                                  worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ()) : std::nullopt,
                          tickInput_.touchControlsEnabled, blupi_.GetYaw());
            }

            // On-screen touch controls (2026-07-13, plan.md
            // MENU-021..027/028..039/046..057, see InputPad.hpp): the
            // real Pause screen (background/character/5 real buttons)
            // while paused, the real Win/Lost screen (background/
            // character animation/Return button) while won/lost, or the
            // real D-pad/Jump/Action/Pause overlay on top of the live 3D
            // scene + HUD while playing.
            if (phase_ == GalaxyEggbert::Def::GamePhase::Pause)
            {
                const int mission = worldRuntime_.GetMissionNumber();
                const bool showBack = mission != 1;
                const bool showRestart = mission != 1 && mission % 10 != 0;
                inputPad_.DrawPause(
                    device, viewport.getWidthProperty(), viewport.getHeightProperty(), showBack, showRestart,
                    phaseTimeSeconds_, fadeOutPhase_, CanRecordDemo() && !demoSlotsShown_, !demoSlotsShown_,
                    CanRevertWorld() && !demoSlotsShown_, revertArmed_ ? "Sure?" : "Original");
                if (demoSlotsShown_)
                    DrawDemoSlotsScreen(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                else if (demoNoticeSeconds_ > 0.0f)
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                               {demoNotice_});
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Win || phase_ == GalaxyEggbert::Def::GamePhase::Lost)
            {
                inputPad_.DrawWinLost(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phase_ == GalaxyEggbert::Def::GamePhase::Win, phaseTimeSeconds_);
                if (demoNoticeSeconds_ > 0.0f)
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                               {demoNotice_});
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::PlaySetup ||
                     phase_ == GalaxyEggbert::Def::GamePhase::MainSetup)
            {
                inputPad_.DrawSetup(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                    sound_.IsEnabled(), saveData_.GetJumpRight(), saveData_.GetAutoZoom(),
                                    saveData_.GetAccelActive(), phase_ == GalaxyEggbert::Def::GamePhase::MainSetup,
                                    saveData_.GetSelectedGamer(), saveData_.GetEffectsVolume(),
                                    saveData_.GetMusicVolume(), saveData_.GetAccelSensitivity(), phaseTimeSeconds_,
                                    fadeOutPhase_, TerrainReliefWanted(), EffectsLevel());
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Resume)
            {
                inputPad_.DrawResume(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phaseTimeSeconds_, fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Wait)
            {
                if (titleSceneActive_)
                {
                    inputPad_.DrawGalaxyLoading(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                                phaseTimeSeconds_);
                }
                else
                {
                    inputPad_.DrawWait(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                       phaseTimeSeconds_);
                }
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Init)
            {
                std::array<InputPad::InitGamerInfo, 3> gamers;
                for (int g = 0; g < 3; ++g)
                {
                    gamers[static_cast<std::size_t>(g)] = InputPad::InitGamerInfo{
                        saveData_.GetLivesForGamer(g), saveData_.GetMainGatesForGamer(g),
                        saveData_.GetSecondaryGatesForGamer(g)};
                    // The player's own galaxy (BD-0119), once it has a main hub or progress, by its name.
                    std::error_code ignored;
                    if (saveData_.GalaxyProgress(g).hasProgress ||
                        std::filesystem::exists(GalaxyEggbert::Editor::GalaxyWorldPath(g, 1), ignored))
                        gamers[static_cast<std::size_t>(g)].galaxyGates = saveData_.GetGalaxyGatesForGamer(g);
                    if (galaxyNamesStale_)
                        galaxyNames_[static_cast<std::size_t>(g)] = GalaxyEggbert::Editor::GalaxyName(g);
                    gamers[static_cast<std::size_t>(g)].galaxyName = galaxyNames_[static_cast<std::size_t>(g)];
                }
                galaxyNamesStale_ = false;
                inputPad_.DrawInit(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                   phaseTimeSeconds_, saveData_.GetSelectedGamer(), gamers, fadeOutPhase_,
                                   /*overTitleScene=*/titleSceneActive_, graphics_ && graphics_->getIsFullScreenProperty());
                if (demoSlotsShown_)
                {
                    // Over the pause screen's own picture, as from Pause (the menu's buttons hidden).
                    inputPad_.DrawPause(device, viewport.getWidthProperty(), viewport.getHeightProperty(), false, false,
                                        phaseTimeSeconds_, GalaxyEggbert::Def::GamePhase::None, false, false);
                    DrawDemoSlotsScreen(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                }
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Play)
            {
                if (tickInput_.touchControlsEnabled)
                    inputPad_.DrawPlay(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                if (demoActive_)
                {
                    // Speedy Blupi shows TX_DEMOPLAY ("Demo") top left while a demo plays.
                    std::vector<std::string> names{"Demo"};
                    names.insert(names.end(), activeCheats_.begin(), activeCheats_.end());
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(), names);
                }
                else
                {
                    // Speedy Blupi's TX_DEMOREC ("REC") while the player records a demo (BD-0116), and
                    // what the last recording became.
                    std::vector<std::string> names;
                    if (demoRecording_ && demoRecordSlot_ > 0) names.push_back("REC");
                    const bool noticeApart = demoNoticeSeconds_ > 0.0f && trainingHintShown;
                    if (demoNoticeSeconds_ > 0.0f && !noticeApart) names.push_back(demoNotice_);
                    names.insert(names.end(), activeCheats_.begin(), activeCheats_.end());
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(), names);
                    if (noticeApart)
                    {
                        // Under the hint's banner (40 reference pixels) and under the list beside it.
                        const float listBottom = names.empty() ? 0.0f : 5.0f + 18.0f * static_cast<float>(names.size());
                        inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                                   {demoNotice_}, std::max(40.0f, listBottom));
                    }
                }
                if (cheatDebug_ && GalaxyEggbert::Game::kCheatsEnabled)
                {
                    GalaxyEggbert::Game::DebugOverlayInfo info;
                    info.ticks = worldRuntime_.GetAnimPhase();
                    info.mission = worldRuntime_.GetMissionNumber();
                    info.region = worldRuntime_.GetSkyRegion();
                    info.lives = interaction_.Lives();
                    info.position = {blupi_.GetX() + WorldRuntime::kWorldCenterX, blupi_.GetY(),
                                     blupi_.GetZ() + WorldRuntime::kWorldCenterZ};
                    info.velocity = debugVelocity_;
                    info.yawDegrees = blupi_.GetYaw() * 180.f / 3.14159265f;
                    info.animation = static_cast<int>(blupi_.GetAnimState());
                    info.icon = blupi_.GetRawAnimIcon();
                    info.speed = GameSpeedFactor(gameSpeed_);
                    info.keys = static_cast<int>(tickInput_.keys.size());
                    info.touches = static_cast<int>(std::count_if(tickInput_.touches.begin(), tickInput_.touches.end(),
                        [](const auto& t) {
                            using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
                            return t.state == int(TouchLocationState::Pressed) || t.state == int(TouchLocationState::Moved);
                        }));
                    info.gamepad = !SuppressLiveInput() && ReadGamepadFrame().connected;
                    info.accelerometer = tickInput_.accelValid;
                    info.mode = blupi_.IsOnGround() ? "ground" : "air";
                    using VM = BlupiController::VehicleMode;
                    switch (blupi_.GetVehicleMode())
                    {
                        case VM::Helicopter: info.mode += " heli"; break;
                        case VM::Jeep: info.mode += " jeep"; break;
                        case VM::Tank: info.mode += " tank"; break;
                        case VM::Overcraft: info.mode += " overcraft"; break;
                        case VM::Skateboard: info.mode += " skate"; break;
                        case VM::None: break;
                    }
                    if (blupi_.IsNage()) info.mode += " swim";
                    if (blupi_.IsSurf()) info.mode += " surf";
                    if (blupi_.IsGhost()) info.mode += " ghost";
                    if (blupi_.IsBallooned()) info.mode += " balloon";
                    if (blupi_.IsEcrased()) info.mode += " squashed";
                    if (blupi_.IsSuspended()) info.mode += " hanging";
                    if (blupi_.IsInverted()) info.mode += " inverted";
                    if (blupi_.IsDeathLocked()) info.mode += " dead";
                    inputPad_.DrawDebug(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                       GalaxyEggbert::Game::DebugOverlayLines(info), tickInput_.touchControlsEnabled);
                }
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Editor)
            {
                if (worldEditor_.IsBrowsing())
                {
                    worldEditor_.DrawBrowsing(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                }
                else
                {
                    worldEditor_.Draw(device, camera_,
                                      viewport.getWidthProperty(), viewport.getHeightProperty(),worldRuntime_.GetWorld());
                }
            }

            if (phase_ == GalaxyEggbert::Def::GamePhase::Editor || phase_ == GalaxyEggbert::Def::GamePhase::Init)
                editorDevices_.Draw(device, viewport.getWidthProperty(), viewport.getHeightProperty());

            // Hidden cheat menu overlay (2026-07-13, plan.md
            // CHEAT-001..009) -- real source renders it as a transparent
            // overlay layered on top of whatever's already on screen, no
            // background swap and no phase change, so this draws LAST,
            // on top of the just-drawn Play controls above (only ever
            // shown during Play, see Update()'s own gating).
            if (cheatMenuShown_ && phase_ == GalaxyEggbert::Def::GamePhase::Play)
            {
                inputPad_.DrawCheatMenu(device, viewport.getWidthProperty(), viewport.getHeightProperty());
            }
            if (controlsHelp_.shown)
            {
                inputPad_.DrawControlsHelp(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                          controlsHelp_.cheats);
            }
        }
    }

    void GalaxyEggbertGame::DrawFrameCaptures(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        // One-shot full-frame screenshot, taken here (2026-07-11) rather
        // than reusing the terrain-only screenshot.png above (captured
        // right after the opaque terrain pass, before billboards/HUD) --
        // that one is a deliberately earlier terrain-geometry diagnostic,
        // not meant to also cover the 2D HUD added above. This one runs
        // after every draw call in the frame, including the new HUD, so it
        // can be inspected for visual HUD verification the same way
        // screenshot.png already is for terrain.
        //
        // Gated on drawFrameIndex_ > terrainPixelPrintedFrame_ (NEXT.md §5,
        // 2026-07-13): the terrain diagnostic's own mid-frame
        // GetBackBufferData call corrupts the rest of THAT frame's
        // rendering on Vulkan (see Draw()'s own opening comment) -- waiting
        // for a later frame guarantees this screenshot captures a frame
        // that never had a mid-draw readback.
        static bool hudScreenshotWritten = false;
        if (!hudScreenshotWritten && drawFrameIndex_ > terrainPixelPrintedFrame_)
        {
            hudScreenshotWritten = true;
            int w = 0, h = 0;
            GetBackBufferSize(device, w, h);
            std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                Microsoft::Xna::Framework::Color(0, 0, 0, 0));
            device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
            Microsoft::Xna::Framework::Graphics::Texture2D hudScreenshot(device, w, h);
            hudScreenshot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
            hudScreenshot.SaveAsPng("screenshot_hud.png");
            std::cout << "GalaxyEggbertCNA: wrote screenshot_hud.png (" << w << "x" << h << ")."
                      << std::endl;
        }

        // A save's thumbnail (ED-0013), from the frame just drawn; the dice's play-test waits for it.
        if (phase_ == GalaxyEggbert::Def::GamePhase::Editor && !worldEditor_.IsBrowsing())
        {
            if (const auto thumbnail = worldEditor_.ConsumeThumbnailRequest(); !thumbnail.empty())
            {
                SaveEditorThumbnail(device, thumbnail);
            }
        }

        // AUTOPLAY_SHOTS (EnableAutoplay()): this frame, HUD and all, as the player would see it.
        if (autoplayShotPending_ && drawFrameIndex_ > terrainPixelPrintedFrame_)
        {
            autoplayShotPending_ = false;
            int w = 0, h = 0;
            GetBackBufferSize(device, w, h);
            std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                Microsoft::Xna::Framework::Color(0, 0, 0, 0));
            device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
            Microsoft::Xna::Framework::Graphics::Texture2D shot(device, w, h);
            shot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
            std::error_code ignored; // `shots N` without AUTOPLAY_SHOTS: the directory may not exist yet
            std::filesystem::create_directories("autoplay_shots", ignored);
            char filename[64];
            std::snprintf(filename, sizeof(filename), "autoplay_shots/%07d.png", autoplayTick_);
            shot.SaveAsPng(filename);
        }

        // INFRA-001 (plan.md §7) -- see EnableGoldenCaptureMode()'s own
        // comment. Fixed tick indices chosen to let the world settle
        // (terrain/camera/spawn) and cover a couple of seconds of
        // deterministic simulation at the default 60Hz fixed timestep.
        // Gated the same way as the HUD screenshot above (avoids the
        // Vulkan mid-draw-readback corruption).
        if (goldenCaptureMode_ && drawFrameIndex_ > terrainPixelPrintedFrame_)
        {
            if (goldenCaptureAwaitingDraw_ &&
                goldenCaptureNextIndex_ < kGoldenCaptureCount)
            {
                int w = 0, h = 0;
                GetBackBufferSize(device, w, h);
                std::vector<Microsoft::Xna::Framework::Color> backBuffer(
                    static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                    Microsoft::Xna::Framework::Color(0, 0, 0, 0));
                device.GetBackBufferData(backBuffer.data(), 0, static_cast<int>(backBuffer.size()));
                Microsoft::Xna::Framework::Graphics::Texture2D goldenShot(device, w, h);
                goldenShot.SetData(backBuffer.data(), static_cast<int>(backBuffer.size()));
                char filename[64];
                std::snprintf(filename, sizeof(filename), "golden_frame_%04d.png",
                              kGoldenCaptureTicks[goldenCaptureNextIndex_]);
                goldenShot.SaveAsPng(filename);
                std::cout << "GalaxyEggbertCNA: wrote " << filename << " (" << w << "x" << h << ")."
                          << std::endl;
                ++goldenCaptureNextIndex_;
                goldenCaptureAwaitingDraw_ = false;
                if (goldenCaptureNextIndex_ >= kGoldenCaptureCount)
                {
                    std::cout << "GalaxyEggbertCNA: golden capture complete, exiting." << std::endl;
                    Exit();
                }
            }
        }
    }
}
