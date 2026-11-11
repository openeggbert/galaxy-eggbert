// GalaxyEggbertGame's frame: the 3D world (DrawWorld), the HUD, menus and editor over it
// (DrawOverlay) and the frame captures (DrawFrameCaptures). Split out of GalaxyEggbertGame.cpp.
#include "GalaxyEggbertGame.hpp"
#include "BuildStamp.hpp"
#include "NetGame.hpp"

#include <GalaxyEggbert/Version.hpp>

#include <exception>

#include <Microsoft/Xna/Framework/Vector4.hpp>

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
#include "NetGame.hpp"
#include <GalaxyEggbert/Net/NetRules.hpp>
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
        // Above 60 frames a second, between two steps: the camera and Blupi drawn part way between them.
        const bool between = frameRateInterpolating_ && drawAlpha_ < 0.999f && phase_ == GalaxyEggbert::Def::GamePhase::Play;
        const auto eye = camera_.Position();
        const auto target = cameraTarget_;
        if (between)
        {
            const float t = drawAlpha_;
            const auto lerp = [t](const Microsoft::Xna::Framework::Vector3& a, const Microsoft::Xna::Framework::Vector3& b)
            { return a + (b - a) * t; };
            const Microsoft::Xna::Framework::Vector3 blupi(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
            const auto drawnEye = lerp(previousEye_, eye);
            const auto drawnTarget = lerp(previousTarget_, target);
            if (drawnEye != drawnTarget)
            {
                camera_.SetLookAt(drawnEye, drawnTarget);
            }
            blupiDrawOffset_ = lerp(previousBlupi_, blupi) - blupi;
            particleLag_ = 1.0f - t;
        }
        // Every moving object too (the lifts, the enemies, the mines...): part way from where the step
        // found it, unless it was not the same one then or jumped (a respawn, a teleport).
        std::vector<std::pair<std::size_t, ObjectPlace>> moved;
        if (between)
        {
            auto& objects = worldRuntime_.GetMobileObjectsMutable();
            const float t = drawAlpha_;
            for (std::size_t i = 0; i < objects.size() && i < previousObjects_.size(); ++i)
            {
                auto& obj = objects[i];
                const auto& before = previousObjects_[i];
                const float dx = obj.currentX - before.x, dy = obj.currentY - before.y, dz = obj.currentZ - before.z;
                if (!obj.active || !before.active || obj.type != before.type || dx * dx + dy * dy + dz * dz > 4.0f ||
                    (dx == 0.0f && dy == 0.0f && dz == 0.0f))
                {
                    continue;
                }
                moved.emplace_back(i, ObjectPlace{obj.type, obj.active, obj.currentX, obj.currentY, obj.currentZ});
                obj.currentX = before.x + dx * t;
                obj.currentY = before.y + dy * t;
                obj.currentZ = before.z + dz * t;
            }
        }
        static std::FILE* interpTrace = []() -> std::FILE*
        {
            const char* path = std::getenv("GALAXY_INTERP_TRACE");
            return path != nullptr && *path != '\0' ? std::fopen(path, "w") : nullptr;
        }();
        if (interpTrace != nullptr && phase_ == GalaxyEggbert::Def::GamePhase::Play)
        {
            const auto drawnEye = camera_.Position();
            std::fprintf(interpTrace, "step %llu alpha %.3f between %d eye %.4f %.4f %.4f shake %.2f %.2f lag %.3f moved %zu",
                         static_cast<unsigned long long>(interpolatedTicks_), drawAlpha_, between ? 1 : 0, drawnEye.X,
                         drawnEye.Y, drawnEye.Z, cameraShake_.GetOffsetX(), cameraShake_.GetOffsetY(), particleLag_,
                         moved.size());
            if (!moved.empty())
            {
                // A projectile (ObjectType23) when one flies, else the first.
                std::size_t shown = moved.front().first;
                for (const auto& [index, place] : moved)
                {
                    if (place.type == GalaxyEggbert::Def::ObjectType::ObjectType23)
                    {
                        shown = index;
                        break;
                    }
                }
                const auto& obj = worldRuntime_.GetMobileObjects()[shown];
                std::fprintf(interpTrace, " object %zu type %d at %.4f %.4f %.4f", shown, static_cast<int>(obj.type),
                             obj.currentX, obj.currentY, obj.currentZ);
            }
            std::fputc('\n', interpTrace);
            std::fflush(interpTrace);
        }
        DrawFrame();
        if (between)
        {
            camera_.SetLookAt(eye, target);
            blupiDrawOffset_ = Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 0.0f);
            particleLag_ = 0.0f;
            auto& objects = worldRuntime_.GetMobileObjectsMutable();
            for (const auto& [i, place] : moved)
            {
                objects[i].currentX = place.x;
                objects[i].currentY = place.y;
                objects[i].currentZ = place.z;
            }
        }
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

    void GalaxyEggbertGame::DrawPositionAxes(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                             int viewportH, float left, float top)
    {
        // The "position" cheat's axes (the owner, 2026-11-09): the world's X, Y and Z as three arrows in
        // the editor's colours (red, green, blue), under the cheats' list, turning as the camera turns --
        // which way to face to go along X, Y or Z. Drawn in the world a little in front of the camera
        // (so they turn with it), over everything, about two lines of text high.
        using Microsoft::Xna::Framework::Vector3;
        using Microsoft::Xna::Framework::Vector4;
        const float s = static_cast<float>(viewportH) / 480.0f;
        // Their foot: under the list, a little in from its left.
        const float footX = left + 36.0f * s;
        const float footY = top + 44.0f * s;
        const float ndcX = footX / static_cast<float>(viewportW) * 2.0f - 1.0f;
        const float ndcY = 1.0f - footY / static_cast<float>(viewportH) * 2.0f;
        constexpr float kDepth = 2.0f;
        const float tanY = std::tan(camera_.FieldOfView() * 0.5f);
        const float tanX = tanY * camera_.AspectRatio();
        const Vector3 forward = camera_.Forward();
        const Vector3 right = camera_.Right();
        const Vector3 up = camera_.Up();
        const Vector3 foot = camera_.Position() + forward * kDepth + right * (ndcX * kDepth * tanX) + up * (ndcY * kDepth * tanY);
        // About 22 reference pixels long on the screen.
        const float length = 22.0f / 480.0f * 2.0f * kDepth * tanY;
        struct Axis
        {
            Vector3 direction;
            Microsoft::Xna::Framework::Color colour;
            const char* name;
        };
        const Axis axes[3] = {{Vector3(1.0f, 0.0f, 0.0f), Microsoft::Xna::Framework::Color(235, 70, 60), "X"},
                              {Vector3(0.0f, 1.0f, 0.0f), Microsoft::Xna::Framework::Color(70, 215, 95), "Y"},
                              {Vector3(0.0f, 0.0f, 1.0f), Microsoft::Xna::Framework::Color(70, 135, 250), "Z"}};
        positionAxes_.Clear();
        std::vector<GalaxyEggbert::Game::InputPad::ScreenLabel> labels;
        const auto viewProjection = camera_.ViewProjection();
        for (const auto& axis : axes)
        {
            const Vector3 tip = foot + axis.direction * length;
            // A thicker shaft (three lines a hair apart across the view) and an arrowhead facing the camera.
            Vector3 across = Vector3::Cross(axis.direction, forward);
            if (across.LengthSquared() < 1e-4f)
            {
                across = up;
            }
            across.Normalize();
            const float hair = length * 0.03f;
            for (const float o : {-hair, 0.0f, hair})
            {
                positionAxes_.Line(foot + across * o, tip + across * o, axis.colour);
            }
            const Vector3 back = tip - axis.direction * (length * 0.28f);
            positionAxes_.Line(tip, back + across * (length * 0.14f), axis.colour);
            positionAxes_.Line(tip, back - across * (length * 0.14f), axis.colour);
            positionAxes_.Line(back + across * (length * 0.14f), back - across * (length * 0.14f), axis.colour);
            // Its name past its tip, where the camera sees it.
            const Vector3 at = foot + axis.direction * (length * 1.35f);
            const Vector4 clip = Vector4::Transform(Vector4(at.X, at.Y, at.Z, 1.0f), viewProjection);
            if (clip.W > 0.01f)
            {
                labels.push_back({(clip.X / clip.W * 0.5f + 0.5f) * static_cast<float>(viewportW),
                                  (0.5f - clip.Y / clip.W * 0.5f) * static_cast<float>(viewportH), axis.name,
                                  Vector3(axis.colour.getRProperty() / 255.0f, axis.colour.getGProperty() / 255.0f,
                                          axis.colour.getBProperty() / 255.0f)});
            }
        }
        positionAxes_.Draw(device, camera_, false);
        inputPad_.DrawLabels(device, viewportW, viewportH, labels);
    }

    void GalaxyEggbertGame::DrawNetNames(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportW,
                                         int viewportH)
    {
        // The other players' names over their heads, in their colours (a Galaxy addition, the owner's
        // Q-021: in 3D another Blupi is easily lost from sight, behind a wall or far off; BD-0120).
        // Through walls, as far as 30 cells; not a hidden one (the drink's Hide, SEC_HIDE).
        using Microsoft::Xna::Framework::Vector3;
        using Microsoft::Xna::Framework::Vector4;
        namespace Net = GalaxyEggbert::Net;
        if (!NetMatchActive() || viewportW <= 0 || viewportH <= 0) return;
        const auto& n = *net_;
        static const Vector3 kTeamColours[Net::kTeams] = {Vector3(1.0f, 0.88f, 0.15f), Vector3(1.0f, 0.55f, 0.10f),
                                                          Vector3(0.35f, 0.60f, 1.0f), Vector3(0.35f, 0.90f, 0.35f)};
        const auto viewProjection = camera_.ViewProjection();
        const float s = static_cast<float>(viewportH) / 480.0f;
        std::vector<GalaxyEggbert::Game::InputPad::ScreenLabel> labels;
        for (std::size_t i = 0; i < n.peers.size(); ++i)
        {
            const auto& peer = n.peers[i];
            const auto& p = peer.pose;
            if (!peer.present || !peer.hasPose || (p.flags & Net::kPoseHidden) != 0 ||
                p.aura == GalaxyEggbert::Def::ToRaw(GalaxyEggbert::Def::SecretPower::Hide))
                continue;
            const float t = NetPoseBlend(n, i);
            const Vector3 at(peer.previous.x + (p.x - peer.previous.x) * t, peer.previous.y + (p.y - peer.previous.y) * t + 0.7f,
                             peer.previous.z + (p.z - peer.previous.z) * t);
            const float dx = at.X - blupi_.GetX(), dy = at.Y - blupi_.GetY(), dz = at.Z - blupi_.GetZ();
            if (dx * dx + dy * dy + dz * dz > 30.0f * 30.0f) continue;
            const Vector4 clip = Vector4::Transform(Vector4(at.X, at.Y, at.Z, 1.0f), viewProjection);
            if (clip.W <= 0.01f) continue;
            const float x = (clip.X / clip.W * 0.5f + 0.5f) * static_cast<float>(viewportW);
            const float y = (0.5f - clip.Y / clip.W * 0.5f) * static_cast<float>(viewportH);
            if (x < -50.0f || x > static_cast<float>(viewportW) + 50.0f || y < -20.0f || y > static_cast<float>(viewportH) + 20.0f)
                continue;
            labels.push_back({x + 1.2f * s, y + 1.2f * s, peer.name, Vector3(0.0f, 0.0f, 0.0f)});
            labels.push_back({x, y, peer.name, kTeamColours[peer.team % Net::kTeams]});
        }
        inputPad_.DrawLabels(device, viewportW, viewportH, labels);
    }

    std::vector<std::string> GalaxyEggbertGame::ActiveCheatNames() const
    {
        // The cheats' names as listed; the "fps" cheat's with its reading, the "position" cheat's with
        // Blupi's place (the editor's cells: x and z from the world's corner, y up).
        std::vector<std::string> names = activeCheats_;
        for (auto& name : names)
        {
            if (name == "fps")
            {
                char text[32];
                if (fpsShown_ > 0.0f)
                    std::snprintf(text, sizeof(text), "fps %.0f", static_cast<double>(fpsShown_));
                else
                    std::snprintf(text, sizeof(text), "fps --"); // before its first half second
                name = text;
            }
            else if (name == "position")
            {
                char text[64];
                std::snprintf(text, sizeof(text), "x=%.1f y=%.1f z=%.1f",
                              static_cast<double>(blupi_.GetX() + GalaxyEggbert::Game::WorldRuntime::kWorldCenterX),
                              static_cast<double>(blupi_.GetY()),
                              static_cast<double>(blupi_.GetZ() + GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ));
                name = text;
            }
        }
        return names;
    }

    void GalaxyEggbertGame::DrawFrame()
    {
        auto& device = getGraphicsDeviceProperty();
        // The frames drawn a second, for the "fps" cheat: counted always (cheap), read every half second.
        {
            const auto now = std::chrono::steady_clock::now();
            if (fpsFrames_ == 0 && fpsSince_ == std::chrono::steady_clock::time_point{})
            {
                fpsSince_ = now;
            }
            ++fpsFrames_;
            const float elapsed = std::chrono::duration<float>(now - fpsSince_).count();
            if (elapsed >= 0.5f)
            {
                fpsShown_ = static_cast<float>(fpsFrames_) / elapsed;
                fpsFrames_ = 0;
                fpsSince_ = now;
            }
        }
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
        // Nor under the Setup, which covers the whole screen (setup.png and its bars): with the 3D effects
        // the hidden world cost a whole frame's shadows and finish there.
        if ((phase_ != GalaxyEggbert::Def::GamePhase::Wait || titleSceneActive_) &&
            (phase_ != GalaxyEggbert::Def::GamePhase::Init || titleSceneActive_) &&
            phase_ != GalaxyEggbert::Def::GamePhase::MainSetup && phase_ != GalaxyEggbert::Def::GamePhase::PlaySetup &&
            !(phase_ == GalaxyEggbert::Def::GamePhase::Editor && worldEditor_.IsBrowsing() && !titleSceneActive_))
        {
            // What to draw (GalaxyEggbertGameScene.cpp); with the 3D effects, first the sun's shadow map
            // (it has its own target, so before the frame's).
            BuildSceneFrame(sceneFrame_, soulDrawnIn3D);
            // Ultra draws the world at the window's own pixels, not the 800x480 logical surface's (which
            // CNA scales up to the window); the HUD after it stays on the logical surface. Its frames are
            // moved by a fraction of a pixel each (the TAA's samples).
            int targetW = 0, targetH = 0;
            if (sceneFrame_.effects.ultra)
            {
                const auto client = getWindowProperty().getClientBoundsProperty();
                targetW = client.Width;
                targetH = client.Height;
                sceneFrame_.camera.projection = ultraFinish_.Jitter(sceneFrame_.camera.projection, targetW, targetH);
            }
            const auto assets = SceneAssets();
            worldRenderer_.RenderShadowMap(device, sceneFrame_, assets);
            // The 3D effects' finish (PostProcess.hpp): the world into a render target, then onto the
            // screen with its glow and grade; without them, or where the device cannot, straight on.
            const auto& effects = sceneFrame_.effects;
            // Ultra: first what each pixel shows (its normal, depth, mirror), and the screen's occlusion of it
            // for the world's shaders (UltraFinish.hpp).
            ultraAssets_.screenOcclusion = nullptr;
            if (effects.ultra && ultraFinish_.BeginGeometry(device, targetW, targetH))
            {
                if (worldRenderer_.DrawGeometry(device, sceneFrame_, assets))
                {
                    ultraAssets_.screenOcclusion = ultraFinish_.EndGeometry(
                        device, sceneFrame_, GalaxyEggbert::Game::SceneShaders::SunDirection());
                }
                else
                {
                    device.SetRenderTarget(nullptr);
                }
            }
            const bool ultra = effects.ultra &&
                               ultraFinish_.Begin(device, Microsoft::Xna::Framework::Color(0, 35, 98), targetW, targetH);
            const bool finish = ultra || ((effects.smoothing || effects.glow) &&
                                          postProcess_.Begin(device, Microsoft::Xna::Framework::Color(0, 35, 98), targetW, targetH));
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
            // The editor's gizmos with the world, against its depth (in the effects' target when there is one).
            if (phase_ == GalaxyEggbert::Def::GamePhase::Editor && !worldEditor_.IsBrowsing())
            {
                worldEditor_.DrawWorldOverlays(device, camera_, worldRuntime_.GetWorld());
            }
            worldIntoTarget_ = false;
            if (ultra)
            {
                // The sun for its fog: the shadow map's cascades.
                const auto& scene = worldRenderer_.Scene();
                GalaxyEggbert::Game::UltraSun sun;
                sun.shadowed = scene.ShadowMapped();
                sun.shadowNear = scene.ShadowMap(0);
                sun.shadowFar = scene.ShadowMap(1);
                sun.nearViewProjection = scene.LightViewProjection(0);
                sun.farViewProjection = scene.LightViewProjection(1);
                sun.direction = GalaxyEggbert::Game::SceneShaders::SunDirection();
                ultraFinish_.End(device, sceneFrame_, sun);
            }
            else if (finish)
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
        assets.ultra = UltraSceneAssetsForFrame(getGraphicsDeviceProperty());
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
            // A development aid only: a game folder that cannot be written (installed for all users,
            // /opt, Program Files) must not end the game on its first frame (2026-11-11, the 0.1.0
            // packages: "ImageLoader: failed to save PNG" aborted it).
            try
            {
                screenshot.SaveAsPng("screenshot.png");
                std::cout << "GalaxyEggbertCNA: wrote screenshot.png (" << w << "x" << h << ")."
                          << std::endl;
            }
            catch (const std::exception& failure)
            {
                std::cout << "GalaxyEggbertCNA: no screenshot.png (" << failure.what() << ")." << std::endl;
            }
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
                hud_.SetPersoTeam(NetTeam()); // his team's bomb icon in a network game
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
                const bool showBack = mission != 1 && !NetMatchActive();
                const bool showRestart = mission != 1 && mission % 10 != 0 && !NetMatchActive();
                if (NetTypingInMatch())
                    DrawNetMatchKeyboard(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                else
                    inputPad_.DrawPause(
                        device, viewport.getWidthProperty(), viewport.getHeightProperty(), showBack, showRestart,
                        phaseTimeSeconds_, fadeOutPhase_, CanRecordDemo() && !demoSlotsShown_, !demoSlotsShown_,
                        (CanRevertWorld() || NetMatchActive()) && !demoSlotsShown_,
                        NetMatchActive() ? "Message" : revertArmed_ ? "Sure?" : "Original");
                if (demoSlotsShown_)
                    DrawDemoSlotsScreen(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                else if (demoNoticeSeconds_ > 0.0f)
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                               {demoNotice_});
            }
            else if (NetSpectating())
            {
                // The spectator (Q-022): the world under the one watched, his name, the notices.
                const auto& watched = net_->peers[static_cast<std::size_t>(net_->watched)];
                std::vector<std::string> lines{"Watching " + watched.name + "  (" + std::to_string(watched.lives) +
                                                   (watched.lives == 1 ? " life)" : " lives)"),
                                               "Left / right: another player.  Space: the results."};
                const auto notices = NetNotices();
                lines.insert(lines.end(), notices.begin(), notices.end());
                inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(), lines);
                DrawNetNames(device, viewport.getWidthProperty(), viewport.getHeightProperty());
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Win || phase_ == GalaxyEggbert::Def::GamePhase::Lost)
            {
                inputPad_.DrawWinLost(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phase_ == GalaxyEggbert::Def::GamePhase::Win, phaseTimeSeconds_);
                if (NetMatchActive())
                {
                    // The match's results under the original's text (Q-021).
                    std::vector<std::string> lines;
                    const bool won = phase_ == GalaxyEggbert::Def::GamePhase::Win;
                    if (net_->timeUp)
                    {
                        // The time limit's end (Q-022): a first place alone or shared.
                        int firsts = 0;
                        for (const auto place : net_->finalPlaces) firsts += place == 1;
                        lines.emplace_back(!won ? GalaxyEggbert::Net::kTextTimeUp
                                                : firsts > 1 ? GalaxyEggbert::Net::kTextTimeUpShared
                                                             : GalaxyEggbert::Net::kTextTimeUpWon);
                    }
                    else if (won) lines.emplace_back(GalaxyEggbert::Net::kTextWon);
                    if (NetHostHoldsClock())
                        lines.push_back("The others play on: time left " +
                                        GalaxyEggbert::Net::ClockText(net_->limitSeconds - net_->matchSeconds) +
                                        " (the host waits for the end).");
                    const auto results = NetResultLines();
                    lines.insert(lines.end(), results.begin(), results.end());
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(), lines);
                }
                else if (demoNoticeSeconds_ > 0.0f)
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
                                    fadeOutPhase_, TerrainReliefWanted(), EffectsLevel(), saveData_.GetFrameRate(),
                                    saveData_.GetTextureScale(), saveData_.GetPlayerName(saveData_.GetSelectedGamer()));
                if (nicknameEditing_ && phase_ == GalaxyEggbert::Def::GamePhase::MainSetup)
                    DrawNicknameEditing(device, viewport.getWidthProperty(), viewport.getHeightProperty());
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Resume)
            {
                inputPad_.DrawResume(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phaseTimeSeconds_, fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::Def::GamePhase::Wait)
            {
                // Galaxy's own loading screen from the first frame: over the title scene once it is
                // up, before that (and without it) over the logo; never Mobile Eggbert's wait.png.
                inputPad_.DrawGalaxyLoading(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                            phaseTimeSeconds_, titleSceneActive_);
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
                    gamers[static_cast<std::size_t>(g)].nickname = saveData_.GetNickname(g);
                }
                galaxyNamesStale_ = false;
                if (goodbyeShown_)
                {
                    inputPad_.DrawGoodbye(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                          goodbyeSeconds_, titleSceneActive_);
                }
                else
                {
                    inputPad_.DrawInit(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                       phaseTimeSeconds_, saveData_.GetSelectedGamer(), gamers, fadeOutPhase_,
                                       /*overTitleScene=*/titleSceneActive_,
                                       graphics_ && graphics_->getIsFullScreenProperty(),
                                       "Version " + std::string(GalaxyEggbert::VersionString()),
                                       BuildStampText());
                }
                if (demoSlotsShown_)
                {
                    // Over the pause screen's own picture, as from Pause (the menu's buttons hidden).
                    inputPad_.DrawPause(device, viewport.getWidthProperty(), viewport.getHeightProperty(), false, false,
                                        phaseTimeSeconds_, GalaxyEggbert::Def::GamePhase::None, false, false);
                    DrawDemoSlotsScreen(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                }
                else if (NetScreenShown())
                {
                    // The network game's screens, over the same picture.
                    inputPad_.DrawPause(device, viewport.getWidthProperty(), viewport.getHeightProperty(), false, false,
                                        phaseTimeSeconds_, GalaxyEggbert::Def::GamePhase::None, false, false);
                    DrawNetScreen(device, viewport.getWidthProperty(), viewport.getHeightProperty());
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
                    const auto cheats = ActiveCheatNames();
                    names.insert(names.end(), cheats.begin(), cheats.end());
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(), names);
                }
                else
                {
                    // Speedy Blupi's TX_DEMOREC ("REC") while the player records a demo (BD-0116), and
                    // what the last recording became.
                    // The network game's notices (the original's NotifPush) and another player's pause.
                    std::vector<std::string> names = NetNotices();
                    if (demoRecording_ && demoRecordSlot_ > 0) names.push_back("REC");
                    const bool noticeApart = demoNoticeSeconds_ > 0.0f && trainingHintShown;
                    if (demoNoticeSeconds_ > 0.0f && !noticeApart) names.push_back(demoNotice_);
                    const auto cheats = ActiveCheatNames();
                    names.insert(names.end(), cheats.begin(), cheats.end());
                    inputPad_.DrawActiveCheats(device, viewport.getWidthProperty(), viewport.getHeightProperty(), names);
                    DrawNetNames(device, viewport.getWidthProperty(), viewport.getHeightProperty());
                    if (std::find(activeCheats_.begin(), activeCheats_.end(), "position") != activeCheats_.end())
                    {
                        const auto list = GalaxyEggbert::Game::InputPad::ActiveCheatsRect(names, viewport.getHeightProperty());
                        DrawPositionAxes(device, viewport.getWidthProperty(), viewport.getHeightProperty(), list.x0, list.y1);
                    }
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
            try // a development aid only, as screenshot.png above
            {
                hudScreenshot.SaveAsPng("screenshot_hud.png");
                std::cout << "GalaxyEggbertCNA: wrote screenshot_hud.png (" << w << "x" << h << ")."
                          << std::endl;
            }
            catch (const std::exception& failure)
            {
                std::cout << "GalaxyEggbertCNA: no screenshot_hud.png (" << failure.what() << ")." << std::endl;
            }
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
    void GalaxyEggbertGame::DrawNicknameEditing(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                                int viewportW, int viewportH)
    {
        // As the network game's lines typed: the keyboard above, the name and a hint under it.
        const float s = static_cast<float>(viewportH) / 480.0f;
        const float left = static_cast<float>(viewportW) * 0.5f - 290.0f * s;
        const int gamer = saveData_.GetSelectedGamer();
        const std::string player = std::string("Player ") + static_cast<char>('A' + gamer);
        std::vector<GalaxyEggbert::Game::PanelLine> lines;
        lines.push_back({"Nickname:  " + nicknameDraft_ + "_", left, 268.0f * s, 0.5f, false, false});
        lines.push_back({"Shown in the menu and in the network game. Empty: " + player + ".", left, 300.0f * s, 0.42f,
                         false, true});
        lines.push_back({"OK or Enter keeps it, Cancel or Escape leaves it as it was.", left, 322.0f * s, 0.42f, false,
                         true});
        inputPad_.DrawPanel(device, viewportW, viewportH, player + "'s nickname", {}, lines, &nicknameKeyboard_,
                            focusHighlight_);
    }

}
