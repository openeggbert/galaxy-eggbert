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

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::CNA::detail;

    void GalaxyEggbertGame::DrawEditorSpawn(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        if (phase_!=GalaxyEggbert::Def::GamePhase::Editor || worldEditor_.IsBrowsing() ||
            !blupiModelIsReal_ || !blupiModel_ || !blupiEffect_ ||
            blupiModel_->Clips.count("Stop")==0) return;
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Graphics;
        auto starts=CollectPlayerStarts(worldRuntime_.GetWorld());
        if (starts.empty()) return;
        const auto target=worldEditor_.CurrentEditTarget(worldRuntime_.GetWorld());
        if (target && target->kind==GalaxyEggbert::Editor::WorldEditor::EditTarget::Kind::PlayerStart)
            std::stable_partition(starts.begin(),starts.end(),[&](const auto& start) { return start.color!=target->color; });
        std::vector<Matrix> bones;
        blupiModel_->ComputeBoneTransformsEXT("Stop",System::TimeSpan::FromSeconds(0),false,bones);
        blupiEffect_->SetBoneTransforms(bones);
        blupiEffect_->setViewProperty(camera_.View());
        blupiEffect_->setProjectionProperty(camera_.Projection());
        const auto diffuse=blupiEffect_->getDiffuseColorProperty();
        blupiEffect_->setDiffuseColorProperty(Vector3::One);
        blupiEffect_->setAlphaProperty(1.f);
        device.setBlendStateProperty(BlendState::Opaque);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        Texture2D* face=nullptr;
        if (const auto faces=blupiClipFaces_.find("Stop"); faces!=blupiClipFaces_.end() && !faces->second.empty())
        {
            const int index=faces->second.front();
            if (index>=0 && index<static_cast<int>(blupiFaces_.size())) face=&blupiFaces_[index];
        }
        const auto drawPart = [&](const auto& part) {
            blupiEffect_->Apply();
            device.SetVertexBuffer(part.Part->getVertexBufferProperty());
            device.SetIndexBuffer(part.Part->getIndexBufferProperty());
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList,part.Part->getVertexOffsetProperty(),0,
                part.Part->getNumVerticesProperty(),part.Part->getStartIndexProperty(),part.Part->getPrimitiveCountProperty());
        };
        for (const auto& start : starts)
        {
            blupiEffect_->setWorldProperty(Matrix::CreateScale(kRealModelScale)*Matrix::CreateRotationY(3.14159265f-PlayerStartYaw(start))*
                Matrix::CreateTranslation(float(start.x)-WorldRuntime::kWorldCenterX,float(start.y)-.5f,
                                          float(start.z)-WorldRuntime::kWorldCenterZ));
            const auto coloredTexture = [&](Texture2D* source) -> Texture2D* {
                if (!source || start.color==PlayerStartColor::Yellow) return source;
                auto& textures=editorStartTextures_[static_cast<int>(start.color)-1];
                if (const auto found=textures.find(source);found!=textures.end()) return found->second.get();
                const int width=source->getWidthProperty(),height=source->getHeightProperty();
                std::vector<Color> pixels(static_cast<std::size_t>(width)*height);
                source->GetData(pixels.data(),static_cast<int>(pixels.size()));
                for (auto& pixel : pixels)
                {
                    std::uint8_t r=pixel.getRProperty(),g=pixel.getGProperty(),b=pixel.getBProperty();
                    GalaxyEggbert::Editor::RecolorPlayerStartPixel(start.color,r,g,b);
                    pixel=Color(r,g,b,pixel.getAProperty());
                }
                auto texture=std::make_unique<Texture2D>(device,width,height);
                texture->SetData(pixels.data(),static_cast<int>(pixels.size()));
                auto* result=texture.get();
                textures.emplace(source,std::move(texture));
                return result;
            };
            for (const auto& part : blupiModel_->Parts)
            {
                blupiEffect_->setTextureProperty(coloredTexture(face && part.Name==blupiFacePart_ ? face : part.Texture));
                drawPart(part);
            }
            if (target && target->kind==GalaxyEggbert::Editor::WorldEditor::EditTarget::Kind::PlayerStart &&
                target->color==start.color)
            {
                // The model itself stays opaque; selection is the same red overlay as other objects.
                if (!editorTintTexture_)
                {
                    editorTintTexture_=std::make_unique<Texture2D>(device,1,1);
                    const auto white=Color::White;
                    editorTintTexture_->SetData(&white,1);
                }
                const auto emission=blupiEffect_->getEmissiveColorProperty();
                blupiEffect_->setEmissiveColorProperty({1.f,0.f,0.f});
                blupiEffect_->setDiffuseColorProperty(Vector3::Zero);
                blupiEffect_->setAlphaProperty(.35f);
                blupiEffect_->setTextureProperty(editorTintTexture_.get());
                device.setBlendStateProperty(BlendState::NonPremultiplied);
                device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
                for (const auto& part : blupiModel_->Parts) drawPart(part);
                blupiEffect_->setEmissiveColorProperty(emission);
                blupiEffect_->setDiffuseColorProperty(Vector3::One);
                blupiEffect_->setAlphaProperty(1.f);
                device.setBlendStateProperty(BlendState::Opaque);
                device.setDepthStencilStateProperty(DepthStencilState::Default);
            }
        }
        blupiEffect_->setAlphaProperty(1);
        blupiEffect_->setDiffuseColorProperty(diffuse);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        device.setBlendStateProperty(BlendState::Opaque);
    }

    void GalaxyEggbertGame::DrawEnemyBlupis(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        // OR-0063 (EnemyBlupi.hpp): his model and his helicopter or tank in the clip and frame the
        // original's icon shows, the textures turned green and blue as blupi1.png's, facing along
        // the patrol. A turn swings his front past the camera, as the sprites turn through their
        // viewer (the tank's mid-turn frames point the gun out of the screen).
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Graphics::Texture2D;
        constexpr float kPi = 3.14159265358979f;
        const auto cameraPosition = Matrix::Invert(camera_.View()).getTranslationProperty();
        std::vector<Matrix> boneTransforms;
        const auto recolored = [&](Texture2D* texture) -> Texture2D*
        {
            const auto found = enemyBlupiTextures_.find(texture);
            return found != enemyBlupiTextures_.end() ? found->second.get() : texture;
        };
        const auto drawParts = [&](const Microsoft::Xna::Framework::Graphics::SkinnedModelEXT& model, Texture2D* face)
        {
            for (const auto& part : model.Parts)
            {
                blupiEffect_->setTextureProperty(face != nullptr && part.Name == blupiFacePart_ ? face
                                                                                                 : recolored(part.Texture));
                blupiEffect_->Apply();
                device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                device.DrawIndexedPrimitives(Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                                             part.Part->getVertexOffsetProperty(), 0,
                                             part.Part->getNumVerticesProperty(), part.Part->getStartIndexProperty(),
                                             part.Part->getPrimitiveCountProperty());
            }
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
            // Facing posStart (yaw0) and posEnd (yaw0 + pi); a model's +Z turned by a yaw a points
            // along (sin a, cos a). Standing still (posStart = posEnd) he faces +X, the sprites'
            // right.
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
            blupiEffect_->setWorldProperty(Matrix::CreateScale(kRealModelScale) * Matrix::CreateRotationY(yaw) *
                                           Matrix::CreateTranslation(obj.currentX, obj.currentY - 0.5f, obj.currentZ));
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
            blupiModel_->ComputeBoneTransformsEXT(clip, System::TimeSpan::FromSeconds(seconds), /*loop=*/true,
                                                  boneTransforms);
            blupiEffect_->SetBoneTransforms(boneTransforms);
            drawParts(*blupiModel_, face);
            for (const auto& [vehicleName, vehicle] : blupiVehicles_)
            {
                if (vehicle->Clips.count(clip) == 0)
                {
                    continue;
                }
                vehicle->ComputeBoneTransformsEXT(clip, System::TimeSpan::FromSeconds(seconds), /*loop=*/true,
                                                  boneTransforms);
                blupiEffect_->SetBoneTransforms(boneTransforms);
                drawParts(*vehicle, nullptr);
            }
        }
    }

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
            DrawWorld(device, soulDrawnIn3D);
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

    void GalaxyEggbertGame::DrawWorld(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                      bool& soulDrawnIn3D)
    {
        using TargetKind = GalaxyEggbert::Editor::WorldEditor::EditTarget::Kind;
        const auto editorTarget = phase_ == GalaxyEggbert::Def::GamePhase::Editor && !worldEditor_.IsBrowsing()
            ? worldEditor_.CurrentEditTarget(worldRuntime_.GetWorld()) : std::nullopt;
        const auto editorSelected = [&](TargetKind kind, float rawX, float rawY, float rawZ) {
            return editorTarget && editorTarget->kind == kind &&
                std::lround(rawX) == editorTarget->x && std::lround(rawY) == editorTarget->y &&
                std::lround(rawZ) == editorTarget->z;
        };
        // The exact posed mesh overlaid in depth-tested translucent red.
        const auto drawEditorTint = [&](const Microsoft::Xna::Framework::Graphics::SkinnedModelEXT& model,
                                        bool selected, bool boothFrontOnly = false, float poseSeconds = -1.f) {
            if (!selected || !blupiEffect_) return;
            using namespace Microsoft::Xna::Framework::Graphics;
            if (!editorTintTexture_)
            {
                editorTintTexture_ = std::make_unique<Texture2D>(device,1,1);
                const auto white = Microsoft::Xna::Framework::Color::White;
                editorTintTexture_->SetData(&white,1);
            }
            const auto diffuse = blupiEffect_->getDiffuseColorProperty();
            const auto alpha = blupiEffect_->getAlphaProperty();
            const auto emission = blupiEffect_->getEmissiveColorProperty();
            blupiEffect_->setEmissiveColorProperty({1.0f,0.0f,0.0f});
            blupiEffect_->setDiffuseColorProperty({0.0f,0.0f,0.0f});
            blupiEffect_->setAlphaProperty(0.35f);
            blupiEffect_->setTextureProperty(editorTintTexture_.get());
            device.setBlendStateProperty(BlendState::NonPremultiplied);
            device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
            for (const auto& part : model.Parts)
            {
                if (poseSeconds>=0 && !GalaxyEggbert::Game::IsVisibleSawPart(part.Name,poseSeconds)) continue;
                if (boothFrontOnly && GalaxyEggbert::Game::IsBoothBodyPart(part.Name)) continue;
                blupiEffect_->Apply();
                device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                device.DrawIndexedPrimitives(PrimitiveType::TriangleList,part.Part->getVertexOffsetProperty(),0,
                    part.Part->getNumVerticesProperty(),part.Part->getStartIndexProperty(),part.Part->getPrimitiveCountProperty());
            }
            blupiEffect_->setDiffuseColorProperty(diffuse);
            blupiEffect_->setAlphaProperty(alpha);
            blupiEffect_->setEmissiveColorProperty(emission);
            device.setBlendStateProperty(BlendState::Opaque);
            device.setDepthStencilStateProperty(DepthStencilState::Default);
        };
        // Real background image backdrop (NEXT.md §3, 2026-07-09) -- one
        // huge camera-facing billboard quad placed far behind the scene
        // (see backgroundMeshRenderer_'s header comment for why this
        // replaced an earlier SpriteBatch attempt). Sized so its edges sit
        // outside the view frustum at kBackgroundDistance regardless of
        // window aspect ratio (kBackgroundMargin > 1 covers the
        // diagonal/aspect slop), so it always fills the whole screen behind
        // real geometry. Rebuilt every frame like the other billboards
        // since it must keep following the camera. Falls back to the flat
        // device.Clear() color above when no background loaded
        // (backgroundLoaded_ false -- see LoadContent()).
        if (backgroundLoaded_ && backgroundEffect_)
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            constexpr float kBackgroundDistance = 900.0f; // < camera_'s 1000.0f far plane
            constexpr float kBackgroundMargin = 1.3f;
            const float halfHeight = kBackgroundDistance *
                std::tan(camera_.FieldOfView() * 0.5f) * kBackgroundMargin;
            const float halfWidth = halfHeight * camera_.AspectRatio();

            const auto& camPos = camera_.Position();
            const auto& camTarget = cameraTarget_;
            Microsoft::Xna::Framework::Vector3 forward(
                camTarget.X - camPos.X, camTarget.Y - camPos.Y, camTarget.Z - camPos.Z);
            forward.Normalize();
            const Microsoft::Xna::Framework::Vector3 quadCenter(
                camPos.X + forward.X * kBackgroundDistance,
                camPos.Y + forward.Y * kBackgroundDistance,
                camPos.Z + forward.Z * kBackgroundDistance);

            // A distant panorama (owner 2026-11-02, OR-0053: it stood still on the screen as the
            // camera turned). The original tiles its 640 x 480 picture and scrolls it at 2/3 of
            // the world's scroll (Decor.cpp:650-700); here the picture is wrapped round the
            // horizon N times, N the whole number of screen widths in a turn (so it keeps about
            // the width it had), turning the camera scrolls it, and the camera's height moves it
            // up and down at 2/3, as the original's vertical scroll.
            constexpr float kTwoPi = 6.28318530718f;
            const float verticalFov = camera_.FieldOfView();
            const float horizontalFov = 2.0f * std::atan(std::tan(verticalFov * 0.5f) * camera_.AspectRatio());
            const float repeats = std::max(1.0f, std::round(kTwoPi / horizontalFov));
            const float imageAngleWide = kTwoPi / repeats;      // one picture's width, radians
            const float imageAngleHigh = imageAngleWide * 0.75f; // 640 x 480
            const float quadAngleWide = 2.0f * std::atan(halfWidth / kBackgroundDistance);
            const float quadAngleHigh = 2.0f * std::atan(halfHeight / kBackgroundDistance);
            const float yaw = std::atan2(forward.X, -forward.Z);
            const float pitch = std::asin(std::clamp(forward.Y, -1.0f, 1.0f));
            const float uWidth = quadAngleWide / imageAngleWide;
            const float vHeight = quadAngleHigh / imageAngleHigh;
            const float uCenter = yaw / imageAngleWide;
            constexpr float kParallax = 2.0f / 3.0f;
            const float vCenter = 0.5f - (camPos.Y * 64.0f * kParallax) / 480.0f - pitch / imageAngleHigh;
            auto mesh = MakeTexturedMeshBuilder();
            mesh.AddBillboard(quadCenter,
                              Microsoft::Xna::Framework::Vector2(halfWidth * 2.0f, halfHeight * 2.0f),
                              cameraRight, cameraUp,
                              CnaEngine::Graphics::UvRect{uCenter - uWidth * 0.5f, vCenter - vHeight * 0.5f,
                                                          uCenter + uWidth * 0.5f, vCenter + vHeight * 0.5f});

            if (!mesh.Mesh().Empty())
            {
                backgroundMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                backgroundEffect_->View = camera_.View();
                backgroundEffect_->Projection = camera_.Projection();
                backgroundEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                // The panorama wraps (its UVs run past 0..1).
                device.getSamplerStatesProperty()[0] = Microsoft::Xna::Framework::Graphics::SamplerState::LinearWrap;
                // No CullNone override needed here -- fixed at the source
                // (2026-07-09, NEXT.md §8 task 0): billboards used to be
                // back-facing under the default CullCounterClockwise state;
                // easy-3d's AppendBillboardMesh winding was reversed then, and
                // cna-engine's MeshBuilder::AddBillboard took it over as is,
                // so every billboard renders under the default cull state.
                backgroundMeshRenderer_->Draw(device, *backgroundEffect_);
            }
        }

        if (terrainRenderer_ && terrainEffect_)
        {
            terrainEffect_->View = camera_.View();
            terrainEffect_->Projection = camera_.Projection();
            terrainEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            // The reliefs near the camera's target (Q-012), then the opaque terrain; its
            // semi-transparent pass (water) after the models below.
            {
                const auto reliefStart = std::chrono::steady_clock::now();
                const int shownBefore = terrainRenderer_->ReliefChunksShown();
                terrainRenderer_->UpdateRelief(device, cameraTarget_, terrainReliefRadius_, terrainReliefWarm_ ? 1 : -1);
                if (!terrainReliefWarm_ && std::getenv("GALAXY_TERRAIN_RELIEF_TIMING") != nullptr)
                {
                    const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - reliefStart);
                    std::cout << "GalaxyEggbertCNA: relief: " << terrainRenderer_->ReliefChunksShown() - shownBefore
                              << " chunk(s), " << terrainRenderer_->ReliefPrimitiveCount() << " triangles built in "
                              << ms.count() << " ms." << std::endl;
                }
                terrainReliefWarm_ = true;
            }
            terrainRenderer_->DrawOpaque(device, *terrainEffect_);

            if (objectCubeEffect_)
            {
                // Platform-lift/crate UniformCube objects (NEXT.md §8 task 3)
                // -- opaque, drawn alongside the main terrain pass, same
                // texture sheet (terrainTexture_) as terrainEffect_ but its
                // own effect instance since World/View/Projection are set
                // independently per draw call. Rebuilt every frame
                // (2026-07-09) using each MobileObjSpec's real per-instance
                // phase, same reason and pattern as the billboard batches
                // below -- types 47/48's chenille icon formulas now actually
                // animate instead of being frozen at their first frame.
                // An uploaded GpuMesh has no in-place update API (only
                // construction from mesh data), so a fresh mesh is
                // the only way to reflect a new icon/UV per frame -- cheap
                // here since there are only a handful of cube objects.
                auto cubeMesh = MakeLitTexturedMeshBuilder();
                for (const auto& obj : worldRuntime_.GetMobileObjects())
                {
                    const auto visual = ResolveObjectVisual(
                        obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                    if (!obj.active || !visual.visible ||
                        visual.renderMode != ObjectRenderMode::SolidCube)
                    {
                        continue;
                    }
                    if (ConveyorLiftModel(obj) != nullptr)
                    {
                        continue; // drawn as Decor/chenille.rds in the model pass
                    }
                    const Microsoft::Xna::Framework::Vector3 center(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ);
                    const auto tileUv = tileAtlas_.GetTileUv(visual.icon);
                    if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType1 &&
                        ChooseLiftDrawPath(visual.icon, false) == LiftDrawPath::BarAndStem)
                    {
                        // The lift tile 29's bar and stem, not a cube of its mostly transparent tile.
                        const float du = tileUv.u1 - tileUv.u0;
                        const float dv = tileUv.v1 - tileUv.v0;
                        for (const auto& part : LiftVisualParts())
                        {
                            AddCenteredBox(cubeMesh, center + Microsoft::Xna::Framework::Vector3(0.0f, part.centerYOffset, 0.0f),
                                           Microsoft::Xna::Framework::Vector3(part.width, part.height, part.width),
                                           CnaEngine::Graphics::UvRect{tileUv.u0 + part.u0 * du, tileUv.v0 + part.v0 * dv,
                                                                       tileUv.u0 + part.u1 * du, tileUv.v0 + part.v1 * dv});
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
                        AddCenteredBox(cubeMesh, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), faces);
                        continue;
                    }
                    AddCenteredBox(cubeMesh, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), tileUv);
                }

                if (!cubeMesh.Mesh().Empty())
                {
                    objectCubeMeshRenderer_ = UploadLitTexturedMesh(device, cubeMesh.Mesh());
                    objectCubeEffect_->View = camera_.View();
                    objectCubeEffect_->Projection = camera_.Projection();
                    objectCubeEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                    objectCubeMeshRenderer_->Draw(device, *objectCubeEffect_);
                }
                // The crates blown away (InteractionSystem::ByeByePiece, BD-0049): each its cube,
                // spinning about the view axis where it flies.
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
                    auto pieceMesh = MakeLitTexturedMeshBuilder();
                    AddCenteredBox(pieceMesh, Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 0.0f),
                                   Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), tileAtlas_.GetTileUv(visual.icon));
                    auto pieceRenderer = UploadLitTexturedMesh(device, pieceMesh.Mesh());
                    objectCubeEffect_->View = camera_.View();
                    objectCubeEffect_->Projection = camera_.Projection();
                    objectCubeEffect_->World =
                        Microsoft::Xna::Framework::Matrix::CreateRotationZ(-piece.rotationDegrees * 3.14159265f / 180.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(
                            piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z);
                    pieceRenderer->Draw(device, *objectCubeEffect_);
                    objectCubeEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                }
            }

            if (grassEffect_)
            {
                // Icon 107's grass-top overlay (NEXT.md §8 task 3) — a
                // separate texture/effect (BasicEffect only binds one
                // texture at a time), drawn right after the main opaque
                // terrain pass so it composites correctly (both opaque, no
                // special blend/depth state needed, unlike water/icons
                // 30-31).
                grassEffect_->View = camera_.View();
                grassEffect_->Projection = camera_.Projection();
                grassEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                terrainRenderer_->DrawGrass(device, *grassEffect_);
            }

            // Third-person placeholder 3D model (2026-07-09, NEXT.md §3) --
            // only drawn in third-person mode (first-person mode has
            // nothing to show, same reasoning as the pre-2026-07-09 comment
            // this replaced). World transform bakes in a scale and a yaw
            // rotation to face Blupi's movement direction -- exact
            // forward-facing alignment for this specific placeholder asset
            // hasn't been visually verified, a real Blupi model may need a
            // different constant rotation offset here.
            //
            // kPlaceholderModelScale (2026-07-18, user-reported "~160% of
            // block height" bug): the placeholder fox mesh's own local-space
            // vertex Y span is -0.122..78.907 (measured directly from
            // avatars3d/blupi_placeholder/fox1.verts.bin, stride 52 bytes,
            // position = first 3 floats per convert_avatar.py's own
            // struct.pack("<3f3f2f4f4B", ...) layout), i.e. ~79.03 units
            // tall with its feet already at local Y~=0. Real mobile-eggbert
            // Blupi's own standing collision box (Decor::BlupiRect()'s
            // default case, Decor.cpp:2519-2520: Top=pos.Y+11,
            // Bottom=pos.Y+60-2, i.e. 47px inside a 60px DIMBLUPIY cell,
            // Def.hpp:154) brackets the user's own reported 71.875% figure
            // (exact source of that specific fraction not located as a
            // literal constant, likely the user's own sprite measurement) --
            // since 1 block = 1.0 world unit here (BlupiController::
            // GroundHeightAt() returns topmost-solid-block-Y + 1), target
            // height = 0.71875 world units, giving scale = 0.71875 / 79.029
            // =~ 0.009095.
            //
            // kPlaceholderModelYOffset (2026-07-18, user-reported "the fox
            // floats in the air" bug): BlupiController's own GetY() is
            // NOT flush with the terrain's rendered surface -- TerrainRenderer
            // renders a solid block at grid Y with Center.Y = Y directly (no
            // +0.5), so a column's topmost solid block at grid Y=g has its
            // visual top surface at world Y=g+0.5, while GroundHeightAt()
            // (by design, matches the MoveObject/BigDecor cube convention of
            // sitting a full unit above the floor block's own grid index,
            // see TerrainRenderer.cpp/CubeMesh.cpp) returns g+1 -- so
            // GetY() sits a constant 0.5 world units above the true visual
            // ground surface. This is invisible for the first-person camera
            // (kEyeHeight is just a feel-tuned offset, already screenshot-
            // verified against this same convention) and the collision-only
            // point that existed before this model, but is glaringly visible
            // now that a real body silhouette is rendered against the
            // terrain mesh -- confirmed live via a forced third-person
            // screenshot showing the fox's feet hovering above the floor
            // tiles. Deliberately NOT fixed by changing GetY()/
            // GroundHeightAt() itself (would ripple into jump/fall physics,
            // kFallDeathY, teleporter/water Y thresholds, and every already
            // screenshot-tuned camera constant that implicitly assumes this
            // exact convention) -- this is a render-only correction local to
            // this specific placeholder mesh's own translation, exactly like
            // kPlaceholderModelScale above.
            //
            // kPlaceholderModelDepthOffset (2026-07-21, live user bug report:
            // "liska... muze vejit skoro cela do zdi" -- the fox model can
            // walk almost entirely into a wall): Blupi's own collision is a
            // single POINT (not a rect), so it stops exactly flush with a
            // wall's face -- but the fox mesh has real nose-to-tail depth
            // (measured directly from fox1.verts.bin the same way as
            // kPlaceholderModelScale above: local Z span -88.095..66.625,
            // ~154.72 units, by far the largest of the 3 axes, consistent
            // with a quadruped's body-length axis -- X's 25.19 span is
            // shoulder width, Y's already-established 79.03 span is height).
            // With the model's own origin centered on Blupi's collision
            // point and no compensating offset, roughly half that real body
            // depth (~0.7 world units at kPlaceholderModelScale) visually
            // projected forward past the point where collision actually
            // stops, into any wall Blupi walked up to. Confirmed live
            // (forced third-person screenshot, Blupi placed flush against a
            // wall): baseline showed only a tiny sliver of the fox visible;
            // a local-space translation of +0.6 along the model's own
            // (pre-rotation) Z axis -- applied THROUGH the yaw rotation
            // below, not after it, so it always points opposite the model's
            // current facing regardless of which way Blupi is turned --
            // fully clears the mesh's front half of the wall while still
            // looking correctly grounded/positioned in open floor (both
            // re-verified live). Empirically tuned by eye, same category and
            // rigor as kPlaceholderModelScale/YOffset above, not a computed
            // value -- render-only, does not touch Blupi's own collision
            // point at all.
            //
            // Real `Hide` during the death-lock's own life-loss-Voyage window
            // (death-VFX follow-up, `IsDeathHidden()`'s own comment) -- first-
            // person mode already renders no Blupi model at all, so this is
            // the only render-side change needed. Likewise the real frame
            // tables' invisible frames (Clear2, Clear3's tail, Teleporting's
            // -1 entries) draw no real model.
            // The clip is chosen in Update()'s Play block, so a frame drawn
            // before the first Play tick (a mission just loaded from the
            // menu) has none yet: skip the model until it has one. Starting
            // in third person made this reachable (2026-10-25, the game
            // aborted with "Unknown animation clip").
            const bool translucentBlupi = blupi_.GetDrawOpacity(worldRuntime_.GetAnimPhase()) < 1.0f;
            const auto drawBlupi = [&]()
            {
                if (phase_ != GalaxyEggbert::Def::GamePhase::Editor &&
                    cameraMode_ == CameraMode::ThirdPersonModel && blupiModelLoaded_ && blupiEffect_ &&
                    blupiModel_->Clips.count(blupiActiveClipName_) != 0 &&
                    !(blupiModelIsReal_ && blupi_.IsAnimIconInvisible()) && !BlupiDrawnAsDeathSprite())
                {
                    constexpr float kCharredShade = 0.07f; // the sprites' soot black
                    // Charred (Clear1, Clear3: CharredBlupi.hpp), shrunk toward his feet as the frame
                    // crumbles and spread out as the heap of ash.
                    auto charred = BlupiCharredShape();
                    // Glued (BlupiGlueModel()): his model flattened and spread under the glue model, the
                    // dark green puddle of the sprites (168-171: about a sixteenth of his height, half as
                    // wide again).
                    const bool gluePuddle = BlupiGlueModel() != nullptr;
                    if (gluePuddle)
                    {
                        charred = GalaxyEggbert::Game::CharredShape{true, 0.06f, 1.5f, false};
                    }
                    const auto world = BlupiModelWorld(charred);
                    std::vector<Microsoft::Xna::Framework::Matrix> boneTransforms;
                    blupiModel_->ComputeBoneTransformsEXT(
                        blupiActiveClipName_,
                        System::TimeSpan::FromSeconds(blupiClipTimeSeconds_),
                        /*loop=*/true,
                        boneTransforms);
                    blupiEffect_->setWorldProperty(world);
                    blupiEffect_->setViewProperty(camera_.View());
                    blupiEffect_->setProjectionProperty(camera_.Projection());
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    // XNA's default culling, set explicitly so no earlier draw's state
                    // applies: the real model's triangles are clockwise like XNA
                    // content (tools/rd4_to_gltf/export_cna_avatar.py). Converted
                    // straight from glTF they were counter-clockwise, this state
                    // culled his front faces, and he drew inside out
                    // (upstream_bugs.md UB-003).
                    device.setRasterizerStateProperty(
                        Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);

                    auto* face = BlupiFaceTexture();

                    const auto drawParts = [&](const Microsoft::Xna::Framework::Graphics::SkinnedModelEXT& model,
                                               Microsoft::Xna::Framework::Graphics::Texture2D* faceTexture)
                    {
                        for (const auto& part : model.Parts)
                        {
                            blupiEffect_->setTextureProperty(faceTexture != nullptr && part.Name == blupiFacePart_
                                                                 ? faceTexture
                                                                 : part.Texture);
                            blupiEffect_->Apply();
                            device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                            device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                            device.DrawIndexedPrimitives(
                                Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                                part.Part->getVertexOffsetProperty(),
                                0,
                                part.Part->getNumVerticesProperty(),
                                part.Part->getStartIndexProperty(),
                                part.Part->getPrimitiveCountProperty());
                        }
                    };
                    // The drink's Hide (Decor.cpp:828-838, OR-0016): his own clips, drawn at 30 %
                    // opacity, flashing to full every other pair of ticks once the gauge is down to 25.
                    // Not depth-written, so what lies behind him still shows through.
                    const float blupiAlpha = blupi_.GetDrawOpacity(worldRuntime_.GetAnimPhase());
                    if (blupiAlpha < 1.0f)
                    {
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                        device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::DepthRead);
                    }
                    blupiEffect_->setAlphaProperty(blupiAlpha);
                    if (charred.charred)
                    {
                        // Burnt black: soot, no sheen; glued, the puddle's dark green.
                        blupiEffect_->setDiffuseColorProperty(
                            gluePuddle ? Microsoft::Xna::Framework::Vector3(0.20f, 0.32f, 0.03f)
                                       : Microsoft::Xna::Framework::Vector3(kCharredShade, kCharredShade, kCharredShade));
                        blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                    }
                    drawParts(*blupiModel_, face);
                    if (charred.charred)
                    {
                        blupiEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3::One);
                        blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::One);
                    }

                    // The vehicle he rides in this clip (helicopter, jeep, tank,
                    // overcraft, skateboard): its own model, built from the same
                    // Ray Dream frames in the same space (clips.txt "vehicle"
                    // lines), so it takes his world transform and clip time.
                    for (const auto& [vehicleName, vehicle] : blupiVehicles_)
                    {
                        if (vehicle->Clips.find(blupiActiveClipName_) == vehicle->Clips.end())
                        {
                            continue;
                        }
                        vehicle->ComputeBoneTransformsEXT(
                            blupiActiveClipName_,
                            System::TimeSpan::FromSeconds(blupiClipTimeSeconds_),
                            /*loop=*/true,
                            boneTransforms);
                        blupiEffect_->SetBoneTransforms(boneTransforms);
                        drawParts(*vehicle, nullptr);
                    }
                    blupiEffect_->setAlphaProperty(1.0f);
                    if (blupiAlpha < 1.0f)
                    {
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
                        device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::Default);
                    }

                    if (!hideGhosts_.empty())
                    {
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                        device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::DepthRead);
                        for (const auto& ghost : hideGhosts_)
                        {
                            if (blupiModel_->Clips.count(ghost.clip) == 0)
                            {
                                continue;
                            }
                            blupiModel_->ComputeBoneTransformsEXT(ghost.clip, System::TimeSpan::FromSeconds(ghost.clipSeconds),
                                                                  /*loop=*/true, boneTransforms);
                            blupiEffect_->setWorldProperty(ghost.world);
                            blupiEffect_->SetBoneTransforms(boneTransforms);
                            blupiEffect_->setAlphaProperty(std::max(0.0f, (20.0f - ghost.ageTicks) * 0.3f / 20.0f));
                            drawParts(*blupiModel_, ghost.face);
                        }
                        blupiEffect_->setAlphaProperty(1.0f);
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
                        device.setDepthStencilStateProperty(Microsoft::Xna::Framework::Graphics::DepthStencilState::Default);
                    }
                }
            };
            if (!translucentBlupi) drawBlupi();

            // Parked vehicles: a helicopter, jeep, skateboard, tank or
            // overcraft pickup is drawn as its real vehicle model in the
            // first frame of its standing clip instead of its element.png
            // billboard (skipped there, ParkedVehicleModel), in both camera
            // modes. It stands on the floor of its cell, facing +X, the way
            // the sprites face right.
            if (blupiEffect_ && blupiModelIsReal_ && !blupiVehicles_.empty())
            {
                constexpr float kHalfPi = 1.57079632679f;
                constexpr float kSkateLift = 0.45f; // the upright board hangs this far above the floor, tuned by eye
                std::vector<Microsoft::Xna::Framework::Matrix> boneTransforms;
                blupiEffect_->setViewProperty(camera_.View());
                blupiEffect_->setProjectionProperty(camera_.Projection());
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                for (const auto& obj : worldRuntime_.GetMobileObjects())
                {
                    const auto parked = ParkedVehicleModel(obj.type);
                    if (parked.model == nullptr || !obj.active ||
                        !ResolveObjectVisual(obj.type, static_cast<int>(obj.phase), obj.visualIcon).visible)
                    {
                        continue;
                    }
                    parked.model->ComputeBoneTransformsEXT(
                        parked.clip, System::TimeSpan::FromSeconds(0.0), /*loop=*/false, boneTransforms);
                    // The parked skateboard of the original (table_skate, 34 ticks, element icons 129-135) hangs
                    // upright, its long axis up, a little above the floor, and turns about that axis: edge-on,
                    // flat to the viewer, edge-on, back, 30 degrees an icon (Decor.cpp:8339; OR-0083).
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
                    // Facing +X, turned in the editor a quarter at a time (OR-0071).
                    blupiEffect_->setWorldProperty(
                        Microsoft::Xna::Framework::Matrix::CreateScale(kRealModelScale) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationX(skateStand) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(skateSpin) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(
                            kHalfPi * static_cast<float>(1 + CellObjectQuarterTurns(obj.posStartX, obj.posStartY, obj.posStartZ))) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(
                            obj.currentX, obj.currentY - 0.5f + skateLift, obj.currentZ));
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    for (const auto& part : parked.model->Parts)
                    {
                        blupiEffect_->setTextureProperty(part.Texture);
                        blupiEffect_->Apply();
                        device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                        device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                        device.DrawIndexedPrimitives(
                            Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                            part.Part->getVertexOffsetProperty(),
                            0,
                            part.Part->getNumVerticesProperty(),
                            part.Part->getStartIndexProperty(),
                            part.Part->getPrimitiveCountProperty());
                    }
                    drawEditorTint(*parked.model,editorSelected(TargetKind::Object,
                        obj.posStartX+WorldRuntime::kWorldCenterX,obj.posStartY,obj.posStartZ+WorldRuntime::kWorldCenterZ));
                }
                DrawEnemyBlupis(device);
                // The lost helicopter flying away (ByeByeHelico, BD-0049): its parked model, turned
                // about its middle as the original's picture spins.
                const auto helicopter = ParkedVehicleModel(GalaxyEggbert::Def::ObjectType::ObjectType13);
                for (const auto& piece : interaction_.ByeByePieces())
                {
                    if (!piece.helicopter || helicopter.model == nullptr)
                    {
                        continue;
                    }
                    helicopter.model->ComputeBoneTransformsEXT(
                        helicopter.clip, System::TimeSpan::FromSeconds(0.0), /*loop=*/false, boneTransforms);
                    blupiEffect_->setWorldProperty(
                        Microsoft::Xna::Framework::Matrix::CreateScale(kRealModelScale) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(kHalfPi) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, -0.5f, 0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationZ(-piece.rotationDegrees * 3.14159265f / 180.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(piece.x, piece.y, piece.z));
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    for (const auto& part : helicopter.model->Parts)
                    {
                        blupiEffect_->setTextureProperty(part.Texture);
                        blupiEffect_->Apply();
                        device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                        device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                        device.DrawIndexedPrimitives(
                            Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                            part.Part->getVertexOffsetProperty(),
                            0,
                            part.Part->getNumVerticesProperty(),
                            part.Part->getStartIndexProperty(),
                            part.Part->getPrimitiveCountProperty());
                    }
                }
            }

            // Objects with a converted 3D model (objectModelTable_, plan.md
            // Phase 7B): drawn where their billboard would be (skipped in the
            // billboard passes), posed at their icon's frame, turned by its
            // yaw and flipped as the icon is (left-right or upside down,
            // about the sprite's centre line: kFlipShift). A model is in
            // blocks with its sprite's view toward +Z, as the parked vehicles
            // are. BigDecor cells with a model stand on their cell's floor at
            // the original's size: an explo.png icon is 64, 128 or 144 game
            // pixels (icons.txt), drawn 1:1 in the original (Pixmap.cpp),
            // where the billboard is 1 block.
            if (blupiEffect_ && !objectModels_.empty())
            {
                constexpr float kDegrees = 3.14159265358979f / 180.0f;
                std::vector<Microsoft::Xna::Framework::Matrix> boneTransforms;
                blupiEffect_->setViewProperty(camera_.View());
                blupiEffect_->setProjectionProperty(camera_.Projection());
                // The signs drawn this frame whose painted face is a decal (drawn after the models).
                auto& decalCells = signDecalCells_;
                decalCells.clear();
                electrodeCells_.clear();
                dynamiteSparkCells_.clear();
                // Models out of view are not drawn (objectModelRadii_): each one's sphere, its reach
                // widened by its depth scale and sprite offset, a block's margin for its animation.
                const Microsoft::Xna::Framework::BoundingFrustum viewFrustum(camera_.View() * camera_.Projection());
                bool tintModelCell = false;
                const auto drawModel = [&](const ObjectModelPose& pose, float x, float y, float z, float turnRadians = 0.0f,
                                           float rollRadians = 0.0f, float depthScale = 1.0f,
                                           bool boothFrontOnly = false, float topScale = 1.0f)
                {
                    const auto index = static_cast<std::size_t>(pose.model);
                    if (modelCulling_ && index < objectModelRadii_.size())
                    {
                        const float reach = objectModelRadii_[index] * std::max(1.0f, std::fabs(depthScale)) +
                                            static_cast<float>(std::abs(pose.offsetX) + std::abs(pose.offsetY)) / 64.0f +
                                            1.0f;
                        if (viewFrustum.Contains(Microsoft::Xna::Framework::BoundingSphere(
                                Microsoft::Xna::Framework::Vector3(x, y, z), reach)) ==
                            Microsoft::Xna::Framework::ContainmentType::Disjoint)
                        {
                            ++perfModelCulled_;
                            return;
                        }
                    }
                    auto& model = *objectModels_[index];
                    model.ComputeBoneTransformsEXT(
                        "Frames", System::TimeSpan::FromSeconds(pose.seconds), /*loop=*/false, boneTransforms);
                    // Mirroring turns the triangles' winding around (a negative depth scale too).
                    device.setRasterizerStateProperty(
                        (pose.mirror != 0) != (depthScale < 0.0f)
                            ? Microsoft::Xna::Framework::Graphics::RasterizerState::CullClockwise
                            : Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                    blupiEffect_->setWorldProperty(
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(pose.yawDegrees * kDegrees) *
                        Microsoft::Xna::Framework::Matrix::CreateScale(
                            pose.mirror == 1 ? -1.0f : 1.0f, pose.mirror == 2 ? -1.0f : 1.0f, 1.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(
                            (pose.mirror == 1 ? -kFlipShift : 0.0f) + static_cast<float>(pose.offsetX) / 64.0f,
                            (pose.mirror == 2 ? kFlipShift : 0.0f) - static_cast<float>(pose.offsetY) / 64.0f,
                            0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateScale(1.0f, 1.0f, depthScale) *
                        // Scaled about the cell's top (the drip's smallest drops, DripShrink).
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, -0.5f, 0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateScale(topScale) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(0.0f, 0.5f, 0.0f) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationZ(rollRadians) *
                        Microsoft::Xna::Framework::Matrix::CreateRotationY(turnRadians) *
                        Microsoft::Xna::Framework::Matrix::CreateTranslation(x, y, z));
                    blupiEffect_->SetBoneTransforms(boneTransforms);
                    for (const auto& part : model.Parts)
                    {
                        if (!GalaxyEggbert::Game::IsVisibleSawPart(part.Name,pose.seconds)) continue;
                        if (boothFrontOnly && GalaxyEggbert::Game::IsBoothBodyPart(part.Name))
                        {
                            continue; // a teleporter's other sides: its plate and lights only
                        }
                        blupiEffect_->setTextureProperty(part.Texture);
                        blupiEffect_->Apply();
                        device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                        device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                        device.DrawIndexedPrimitives(
                            Microsoft::Xna::Framework::Graphics::PrimitiveType::TriangleList,
                            part.Part->getVertexOffsetProperty(),
                            0,
                            part.Part->getNumVerticesProperty(),
                            part.Part->getStartIndexProperty(),
                            part.Part->getPrimitiveCountProperty());
                        perfModelTriangles_ += part.Part->getPrimitiveCountProperty();
                    }
                    drawEditorTint(model,tintModelCell,boothFrontOnly,pose.seconds);
                    ++perfModelDraws_;
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
                    tintModelCell = editorSelected(TargetKind::Object,obj.posStartX+WorldRuntime::kWorldCenterX,
                        obj.posStartY,obj.posStartZ+WorldRuntime::kWorldCenterZ);
                    ObjectTextureSource sheet{};
                    int icon = 0;
                    const ObjectModelPose* pose =
                        ObjectSprite(obj, sheet, icon) ? objectModelTable_.Find(sheet, icon) : nullptr;
                    if (pose != nullptr && sheet == ObjectTextureSource::ObjectM &&
                        GalaxyEggbert::Game::IsClayBridgeIcon(icon))
                    {
                        // ObjectType52 writes every visible frame to the grid, including 367-372.
                        // Its terrain model now owns the drawing (also for the static gallery).
                        if (!terrainRenderer_)
                            drawBridgeRows(*pose, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY),
                                           obj.currentZ);
                    }
                    else if (const ObjectModelPose* conveyor = ConveyorLiftModel(obj))
                    {
                        // The conveyor lift (47/48): its tread on the frame table_chenille(i) shows,
                        // stretched across the cell as the terrain conveyor (ModelDepthScale); the
                        // plain lift (1) its round platform on a funnel and stem, as modelled.
                        const bool plainLift = obj.type == GalaxyEggbert::Def::ObjectType::ObjectType1;
                        const float depth = plainLift ? 1.0f : GalaxyEggbert::Game::ModelDepthScale(311);
                        drawModel(*conveyor, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY),
                                  obj.currentZ, 0.0f, 0.0f, depth);
                        if (!plainLift)
                        {
                            // The belt on both sides (BlockModels.hpp ModelMirroredInDepth).
                            drawModel(*conveyor, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY),
                                      obj.currentZ, 0.0f, 0.0f, -depth);
                        }
                    }
                    else if (pose != nullptr)
                    {
                        // A sprite only faces left or right, along X, by its travel along X. One
                        // patrolling mainly along Z is turned from that facing to its real one.
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
                            turn += 1.57079632f * static_cast<float>(
                                                      CellObjectQuarterTurns(obj.posStartX, obj.posStartY, obj.posStartZ));
                        }
                        drawModel(*pose, obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ,
                                  turn);
                        if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType56 &&
                            dynamiteSparks_.count(icon) != 0)
                        {
                            // Centre of the terminal ring in the shipped dynamite_rd_6e6e00
                            // mesh at Frames=0 (model space). Anchor the star to the fuse,
                            // so orbiting the camera or turning the object cannot detach it.
                            const auto tip = Microsoft::Xna::Framework::Vector3::Transform(
                                Microsoft::Xna::Framework::Vector3(0.22986f, -0.38538f, 0.51735f),
                                Microsoft::Xna::Framework::Matrix::CreateRotationY(turn));
                            dynamiteSparkCells_.push_back({static_cast<std::uint16_t>(icon), obj.currentX + tip.X,
                                ResolveObjectVisualCenterY(obj.type, obj.currentY) + tip.Y,
                                obj.currentZ + tip.Z, 0.0f});
                        }
                    }
                }
                // The pieces flying away with a model (BD-0049): enemies, the invert potion, a fan,
                // spinning about the view axis as the original's picture does.
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
                            drawModel(*pose, piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z,
                                      0.0f, spin);
                        }
                    }
                }
                // The Clear2 death's soul (OR-0047): element 230-241 as Blupi/blupi-ange.rd4, the
                // turn its icons show, rising 300 px from where he died (Decor::BlupiDead,
                // VoyageDraw), half transparent as the sheet's sprite (alpha 127).
                if (interaction_.VoyageActive() && interaction_.VoyageIsSoul())
                {
                    if (const ObjectModelPose* pose =
                            objectModelTable_.Find(ObjectTextureSource::Element, interaction_.VoyageIconId()))
                    {
                        constexpr float kSoulRise = 300.0f / 64.0f;
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                        device.setDepthStencilStateProperty(
                            Microsoft::Xna::Framework::Graphics::DepthStencilState::DepthRead);
                        blupiEffect_->setAlphaProperty(0.5f);
                        drawModel(*pose, interaction_.VoyageWorldAnchorX(),
                                  interaction_.VoyageWorldAnchorY() + interaction_.VoyageProgress() * kSoulRise,
                                  interaction_.VoyageWorldAnchorZ());
                        blupiEffect_->setAlphaProperty(1.0f);
                        device.setDepthStencilStateProperty(
                            Microsoft::Xna::Framework::Graphics::DepthStencilState::Default);
                        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
                        soulDrawnIn3D = true;
                    }
                }
                // The glue death (element 168-171; Q-006): Blupi/glu.rd4's struggling pose, its yellow
                // made the sprites' green at conversion (texrecolour), turned the way he faces, its
                // icon's centre 30 px over his feet; his own model lies flattened under it as the
                // puddle (BlupiGlueModel()).
                if (const ObjectModelPose* glued = BlupiGlueModel())
                {
                    constexpr float kPi = 3.14159265358979f;
                    drawModel(*glued, blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f, blupi_.GetZ(),
                              kPi - blupi_.GetYaw());
                    glueStrandsPending_ = true;
                }
                // The fan cells (VIS-0127): the original fan model turned to blow the cell's way, its
                // rotor on the tile animation's frame.
                if (terrainRenderer_)
                {
                    for (const auto& cell : terrainRenderer_->ModelCells())
                    {
                        tintModelCell = editorSelected(TargetKind::Block,cell.x+WorldRuntime::kWorldCenterX,
                            cell.y,cell.z+WorldRuntime::kWorldCenterZ);
                        const bool saw =
                            std::find(GalaxyEggbert::Game::kSawModelBases.begin(),
                                      GalaxyEggbert::Game::kSawModelBases.end(),
                                      cell.base) != GalaxyEggbert::Game::kSawModelBases.end();
                        if (saw)
                        {
                            // Independent blade in its own cell; openSide carries its axis.
                            const int icon =
                                GalaxyEggbert::ResolveBlockAnimationIconAt(
                                    cell.base, worldRuntime_.GetAnimPhase(), static_cast<int>(std::lround(cell.x)),
                                    static_cast<int>(std::lround(cell.y)), static_cast<int>(std::lround(cell.z)));
                            if (icon >= 0)
                            {
                                if (const ObjectModelPose* pose = objectModelTable_.Find(
                                        ObjectTextureSource::ObjectM, static_cast<std::uint16_t>(icon)))
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
                            const int icon =
                                GalaxyEggbert::ResolveBlockAnimationIconAt(
                                    cell.base, worldRuntime_.GetAnimPhase(), static_cast<int>(std::lround(cell.x)),
                                    static_cast<int>(std::lround(cell.y)), static_cast<int>(std::lround(cell.z)));
                            if (icon >= 0)
                            {
                                const auto shrink = GalaxyEggbert::Game::DripShrink(icon);
                                if (const ObjectModelPose* pose = objectModelTable_.Find(
                                        ObjectTextureSource::ObjectM, shrink.modelIcon))
                                {
                                    drawModel(*pose, cell.x, cell.y, cell.z, 0.0f, 0.0f, 1.0f, false, shrink.scale);
                                }
                            }
                            continue;
                        }
                        if (std::find(GalaxyEggbert::Game::kFanModelBases.begin(), GalaxyEggbert::Game::kFanModelBases.end(),
                                      cell.base) == GalaxyEggbert::Game::kFanModelBases.end())
                        {
                            // A still icon drawn as its model (a pillar), or a lattice of its frame.
                            // The spring bounces through its frames (BlockModels.hpp SpringFrameIcon).
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
                                    // (BarRows.hpp); the model runs along X, turned where the row runs
                                    // along Z.
                                    const bool alongZ = GalaxyEggbert::Game::ClassifyBarRow(
                                                            (cell.sameSides & 3) != 0, (cell.sameSides & 12) != 0) ==
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
                                    // Turned in the editor a quarter at a time from the side it faces
                                    // by itself (OR-0071), its turns kept as an object's (metadata type
                                    // 5): the plate-rotation bit the hub generators set on signs facing
                                    // X (an older plate convention) must not turn the model too (the
                                    // signs of hub 070 stood sideways, owner 2026-11-05).
                                    const float frontYaw = GalaxyEggbert::Game::FrontModelYaw(cell.openSide) +
                                                           1.57079632f * static_cast<float>(CellObjectQuarterTurns(cell.x, cell.y, cell.z));
                                    drawModel(*still, cell.x, cell.y, cell.z, frontYaw);
                                    if (GalaxyEggbert::Game::IsTeleporterIcon(cell.base))
                                    {
                                        // The letter plate and lights on its other three sides too
                                        // (BlockModels.hpp IsTeleporterIcon, OR-0056).
                                        for (int quarter = 1; quarter < 4; ++quarter)
                                        {
                                            const float yaw = frontYaw + 1.57079632f * static_cast<float>(quarter);
                                            const float out = GalaxyEggbert::Game::kBoothSideOffsets[
                                                static_cast<std::size_t>(quarter)];
                                            drawModel(*still, cell.x + out * std::sin(yaw), cell.y,
                                                      cell.z + out * std::cos(yaw), yaw, 0.0f, 1.0f,
                                                      /*boothFrontOnly=*/true);
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
                                    if (GalaxyEggbert::Game::IsElectrodeIcon(cell.base) &&
                                        electrodeSparks_.count(cell.base) != 0)
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
                        const int frame = GalaxyEggbert::ResolveBlockAnimationIcon(cell.base, worldRuntime_.GetAnimPhase()) -
                                          static_cast<int>(cell.base);
                        const ObjectModelPose* pose = objectModelTable_.Find(
                            ObjectTextureSource::ObjectM, GalaxyEggbert::Game::kFanModelIcon + std::max(0, frame));
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
                    tintModelCell = editorSelected(TargetKind::BigDecor,cell.worldX+WorldRuntime::kWorldCenterX,
                        cell.worldY,cell.worldZ+WorldRuntime::kWorldCenterZ);
                    const ObjectModelPose* pose = objectModelTable_.Find(ObjectTextureSource::Explo, cell.icon);
                    if (pose != nullptr)
                    {
                        const float halfBlocks = static_cast<float>(pose->iconPixels > 0 ? pose->iconPixels : 128) / 128.0f;
                        drawModel(*pose, cell.worldX, cell.worldY - 0.5f + halfBlocks, cell.worldZ,
                                  1.57079632f * static_cast<float>(CellObjectQuarterTurns(cell.worldX, cell.worldY, cell.worldZ)));
                    }
                }
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
            }

            // The signs' painted faces (MR-0006): each decal on the sign's front and, mirrored so it
            // reads the right way round, on its back, a hair off the faces.
            if (!signDecals_.empty() && terrainRenderer_)
            {
                if (!decalEffect_)
                {
                    decalEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
                    decalEffect_->VertexColorEnabled = false;
                    decalEffect_->setTextureEnabledProperty(true);
                }
                decalEffect_->View = camera_.View();
                decalEffect_->Projection = camera_.Projection();
                decalEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                // Each face only from its own side (AddBillboard winds a quad clockwise as seen from
                // the side its right x up points to): drawn from both, the plate on the far side of a
                // thin numbered post showed beside it mirrored, a 9 read as a 6 (play pass 2026-11-03).
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
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
                        return Microsoft::Xna::Framework::Vector3(cell.x + x * c + z * s, cell.y + y,
                                                                  cell.z - x * s + z * c);
                    };
                    const float midX = (decal.left + decal.right) * 0.5f;
                    const float midY = (decal.top + decal.bottom) * 0.5f;
                    const Microsoft::Xna::Framework::Vector2 size(decal.right - decal.left, decal.top - decal.bottom);
                    constexpr float kLift = 0.004f;
                    const Microsoft::Xna::Framework::Vector3 right(c, 0.0f, -s);
                    const Microsoft::Xna::Framework::Vector3 up(0.0f, 1.0f, 0.0f);
                    auto mesh = MakeTexturedMeshBuilder();
                    mesh.AddBillboard(toWorld(midX, midY, decal.depth + kLift), size, right, up,
                                      CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                    mesh.AddBillboard(toWorld(-midX, midY, -decal.depth - kLift), size,
                                      Microsoft::Xna::Framework::Vector3(-c, 0.0f, s), up,
                                      CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                    if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                    {
                        decalEffect_->setTextureProperty(decal.texture.get());
                        renderer->Draw(device, *decalEffect_);
                    }
                }
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
            }

            // The glue death's strands (owner 2026-11-04, Q-006): the painted strands from his hands to the
            // puddle and its peaks, as thin tubes in the glue model's plane (GlueStrands.hpp), in their
            // green from element.png, turned and placed as the glue model is.
            if (glueStrandsPending_ && objectEffect_)
            {
                glueStrandsPending_ = false;
                const int frame = std::clamp(blupi_.GetRawAnimIcon() - 168, 0, 3);
                const auto iconUv = GetElementIconUv(168 + frame);
                const auto& texel = GalaxyEggbert::Game::kGlueTexel[static_cast<std::size_t>(frame)];
                const Microsoft::Xna::Framework::Vector2 uv(iconUv.U0 + texel[0] * (iconUv.U1 - iconUv.U0),
                                                            iconUv.V0 + texel[1] * (iconUv.V1 - iconUv.V0));
                constexpr float kPi = 3.14159265358979f;
                const float turn = kPi - blupi_.GetYaw();
                const float c = std::cos(turn), s = std::sin(turn);
                const Microsoft::Xna::Framework::Vector3 origin(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                                blupi_.GetZ());
                auto mesh = MakeTexturedMeshBuilder();
                GalaxyEggbert::Game::AppendPolylineTubes(
                    mesh, GalaxyEggbert::Game::kGluePoints.data(), GalaxyEggbert::Game::kGluePolylines.data(),
                    GalaxyEggbert::Game::kGlueFrames[static_cast<std::size_t>(frame)], uv,
                    Microsoft::Xna::Framework::Vector3(0.0f, 1.0f, 0.0f),
                    [&](const Microsoft::Xna::Framework::Vector3& p)
                    {
                        return Microsoft::Xna::Framework::Vector3(origin.X + p.X * c + p.Z * s, origin.Y + p.Y,
                                                                  origin.Z - p.X * s + p.Z * c);
                    });
                if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                {
                    objectEffect_->View = camera_.View();
                    objectEffect_->Projection = camera_.Projection();
                    objectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                    renderer->Draw(device, *objectEffect_);
                }
            }

            // Painted sparks facing the camera over their models (Q-005).
            if (!electrodeCells_.empty() || !dynamiteSparkCells_.empty())
            {
                if (!decalEffect_)
                {
                    decalEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
                    decalEffect_->VertexColorEnabled = false;
                    decalEffect_->setTextureEnabledProperty(true);
                }
                const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
                const auto right = invView.getRightProperty();
                const auto up = invView.getUpProperty();
                decalEffect_->View = camera_.View();
                decalEffect_->Projection = camera_.Projection();
                decalEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);
                device.setRasterizerStateProperty(Microsoft::Xna::Framework::Graphics::RasterizerState::CullNone);
                const auto drawSparks = [&](const auto& cells, const auto& textures,
                                            const Microsoft::Xna::Framework::Vector2& size,
                                            const CnaEngine::Graphics::UvRect& uv)
                {
                    for (const auto& cell : cells)
                    {
                        auto mesh = MakeTexturedMeshBuilder();
                        mesh.AddBillboard(Microsoft::Xna::Framework::Vector3(cell.x, cell.y, cell.z),
                                          size, right, up, uv);
                        if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                        {
                            decalEffect_->setTextureProperty(textures.at(cell.icon).get());
                            renderer->Draw(device, *decalEffect_);
                        }
                    }
                };
                drawSparks(electrodeCells_, electrodeSparks_, Microsoft::Xna::Framework::Vector2(1.0f, 1.0f),
                           CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                // The three stars occupy [38,55) x [44,60) in their 60-pixel cells.
                // Crop the clear margin so only the star rotates about the 3D fuse tip;
                // preserve the sprite's scale (64 original pixels per world block).
                drawSparks(dynamiteSparkCells_, dynamiteSparks_,
                           Microsoft::Xna::Framework::Vector2(17.0f / 64.0f, 16.0f / 64.0f),
                           CnaEngine::Graphics::UvRect{38.0f / 60.0f, 44.0f / 60.0f, 55.0f / 60.0f, 1.0f});
                device.setRasterizerStateProperty(
                    Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
                device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);
            }

            // Blend a ghost/Hide over the completed opaque scene. DepthRead tests walls in
            // front without preventing objects behind him from showing through his 0.75 alpha.
            if (translucentBlupi) drawBlupi();
            DrawEditorSpawn(device);

            // The terrain's semi-transparent pass after Blupi and every model, so water covers
            // what lies in it (OR-0052: a chest under water looked dry from above).
            device.setRasterizerStateProperty(
                Microsoft::Xna::Framework::Graphics::RasterizerState::CullCounterClockwise);
            terrainRenderer_->DrawTransparent(device, *terrainEffect_);

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

        // Real alpha transparency for every billboard drawn below (2026-07-09,
        // found live: element.png/object-m.png/explo.png/blupi.png icon
        // sheets all have a real, correct RGBA alpha channel -- confirmed by
        // direct pixel inspection, element.png's background pixels are
        // (0,0,0,0) -- but nothing before this enabled blending, so the GPU
        // ignored alpha and drew each billboard's transparent background as
        // solid opaque black. AlphaBlend restored to Opaque right after the
        // last billboard batch below (BigDecor) so it doesn't affect the
        // unrelated 2D SpriteBatch HUD indicator or any future opaque draw
        // call after this point.
        // NonPremultiplied, not AlphaBlend: Texture2D(path) loads PNGs with
        // straight alpha, while AlphaBlend expects the premultiplied colours
        // CNA's (and XNA's) content pipeline produces, which Mobile Eggbert
        // draws. With AlphaBlend every translucent pixel came out too bright:
        // the menu's dark blue button panel showed whitish (user report,
        // 2026-10-25).
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::NonPremultiplied);

        // Billboard rendering for worldRuntime_'s parsed MoveObjects
        // (15-3d-render-mapping-design.md §5/§7, first pass 2026-07-06) —
        // now animated via each MobileObjSpec's real per-instance phase
        // (2026-07-09, WorldRuntime::Update()), with this pass selected by
        // ObjectDefinitionRegistry's Element source. Rebuilt
        // every frame since billboard vertex positions depend on the camera
        // (MeshBuilder::AddBillboard faces the camera's right/up) -- convenient,
        // since it also means the icon lookup naturally re-runs every frame
        // with the latest phase.
        if (objectEffect_ &&
            (!worldRuntime_.GetMobileObjects().empty() || BlupiDrawnAsDeathSprite() || BlupiCharredShape().smokes ||
             BlupiAuraShown() != GalaxyEggbert::Def::SecretPower::None))
        {
            // Camera-facing basis: the inverse view matrix's Right/Up rows
            // give the camera's world-space right/up vectors (standard
            // spherical-billboard technique).
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                // Platform lifts/crates render as solid cubes (see
                // objectCubeMeshRenderer_/NEXT.md §8 task 3), not billboards
                // -- skip here so they aren't drawn twice. object-m.png-,
                // explo.png-, and blupi.png/blupi1.png-sourced types (see
                // objectMPngMeshRenderer_/exploMeshRenderer_/
                // blupiObjectMeshRenderer_/blupi1ObjectMeshRenderer_ below)
                // are also skipped here -- GetElementIconUv() would compute
                // the wrong UV rect for them (element.png icon-index domain,
                // not theirs). ResolveObjectVisual() is phase-aware because
                // ObjectType38 spends part of its cycle on element.png --
                // this loop must pick it up during that window.
                const int objPhase = static_cast<int>(obj.phase);
                const auto visual =
                    ResolveObjectVisual(obj.type, objPhase, obj.visualIcon);
                if (!obj.active || !visual.visible ||
                    visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::Element ||
                    ParkedVehicleModel(obj.type).model != nullptr)
                {
                    continue;
                }
                // 7 patrol enemies with a real per-direction/turn-transition
                // icon table (ENEMY-013/016/018/024/026/041/042, bulldozer/
                // fish/bird/wasp/creature/blupih/blupit) get it here instead
                // of GetObjIcon()'s generic phase-indexed fallback -- this
                // is the one call site with real patrol state (posStart/
                // posEnd/patrolStep/patrolTime) available to feed it.
                const bool patrolGoesLeftFromStart = obj.posStartX > obj.posEndX;
                const int patrolTimeTicks = static_cast<int>(obj.patrolTime);
                int icon;
                if (!TryGetPatrolIcon(obj.type, patrolGoesLeftFromStart, obj.patrolStep, patrolTimeTicks, icon))
                {
                    icon = visual.icon;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Element, icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = GetElementIconUv(icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
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
                const auto uv = GetElementIconUv(icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        piece.x, ResolveObjectVisualCenterY(piece.look.type, piece.y), piece.z),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), right, up,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }
            // His own death sprite (BlupiDrawnAsDeathSprite()): the 60 px icon, its centre 30 px
            // over his feet, as the original draws it. His feet are half a cell under GetY(), where
            // the model's are (kPlaceholderModelYOffset); centred 30 px over GetY() it hung half a
            // cell over the floor.
            if (BlupiDrawnAsDeathSprite())
            {
                const auto uv = GetElementIconUv(blupi_.GetDisplayAnimIcon());
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                       blupi_.GetZ()),
                    Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }
            // The shield's orb (table_shield_blupi, 2 px higher) and sparkles (table_shieldloop), the
            // power's stars (table_magicloop), a frame every 2 ticks, on his sprite's place (its
            // centre 30 px over his feet), a little behind him from the camera: the original draws
            // them before him (Decor.cpp:782-806).
            if (const auto aura = BlupiAuraShown();
                aura == GalaxyEggbert::Def::SecretPower::Shield || aura == GalaxyEggbert::Def::SecretPower::Power)
            {
                static constexpr int kShieldBlupi[16] = {144, 145, 146, 147, 148, 149, 150, 151,
                                                         266, 267, 268, 269, 270, 271, 272, 273};
                static constexpr int kShieldLoop[5] = {274, 275, 276, 277, 278};
                static constexpr int kMagicLoop[5] = {152, 153, 154, 155, 156};
                const int frame = worldRuntime_.GetAnimPhase() / 2;
                const auto behind = invView.getForwardProperty() * 0.35f;
                const Microsoft::Xna::Framework::Vector3 centre(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                                blupi_.GetZ());
                const auto add = [&](int icon, float raise)
                {
                    const auto uv = GetElementIconUv(icon);
                    mesh.AddBillboard(centre + behind + Microsoft::Xna::Framework::Vector3(0.0f, raise, 0.0f),
                                      Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight,
                                      cameraUp, CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
                };
                if (aura == GalaxyEggbert::Def::SecretPower::Shield)
                {
                    add(kShieldBlupi[frame % 16], 2.0f / 64.0f);
                    add(kShieldLoop[frame % 5], 0.0f);
                }
                else
                {
                    add(kMagicLoop[frame % 5], 0.0f);
                }
            }
            // The smoke over his heap of ash (46/47, CharredBlupi.hpp): the icon's rows above the
            // heap, its bottom where the sprite's would be over the model's feet (GetY() - 0.5,
            // as the model's kPlaceholderModelYOffset).
            if (const auto charred = BlupiCharredShape(); charred.smokes)
            {
                const auto uv = GetElementIconUv(blupi_.GetRawAnimIcon());
                constexpr float kRows = static_cast<float>(GalaxyEggbert::Game::kCharredSmokeRows);
                const float feetY = blupi_.GetY() - 0.5f;
                const float bottom = feetY + GalaxyEggbert::Game::kCharredSmokeBottomPixels / 64.0f;
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(blupi_.GetX(), bottom + kRows / 128.0f, blupi_.GetZ()),
                    Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, kRows / 64.0f), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V0 + (uv.V1 - uv.V0) * kRows / 60.0f});
            }

            if (!mesh.Mesh().Empty())
            {
                objectMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                objectEffect_->View = camera_.View();
                objectEffect_->Projection = camera_.Projection();
                objectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                objectMeshRenderer_->Draw(device, *objectEffect_);
            }

            // The drink's bubbles (Decor.cpp:882-895): over each bottle three bubbles (element
            // 274 + n % 5) rise 3 px a tick for 50 ticks, a third apart (table_drinkoffset 0, 7, 22),
            // fading from whole to nothing. The fade in five steps: one mesh and alpha each.
            {
                static constexpr int kDrinkOffsets[3] = {0, 7, 22};
                constexpr int kFadeSteps = 5;
                std::vector<CnaEngine::Graphics::MeshBuilder> fades;
                for (int k = 0; k < kFadeSteps; ++k)
                {
                    fades.push_back(MakeTexturedMeshBuilder());
                }
                bool any = false;
                for (const auto& obj : worldRuntime_.GetMobileObjects())
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
                        const auto uv = GetElementIconUv(274 + n % 5);
                        fades[static_cast<std::size_t>(step)].AddBillboard(
                            Microsoft::Xna::Framework::Vector3(obj.currentX, centreY + static_cast<float>(n) * 3.0f / 64.0f,
                                                               obj.currentZ) +
                                cameraRight * (2.0f / 64.0f),
                            Microsoft::Xna::Framework::Vector2(60.0f / 64.0f, 60.0f / 64.0f), cameraRight, cameraUp,
                            CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
                        any = true;
                    }
                }
                if (any)
                {
                    objectEffect_->View = camera_.View();
                    objectEffect_->Projection = camera_.Projection();
                    objectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                    for (int k = 0; k < kFadeSteps; ++k)
                    {
                        if (fades[static_cast<std::size_t>(k)].Mesh().Empty())
                        {
                            continue;
                        }
                        // (50 - n) / 50 at the step's middle.
                        objectEffect_->setAlphaProperty(1.0f - (static_cast<float>(k) + 0.5f) / static_cast<float>(kFadeSteps));
                        drinkBubbleRenderers_[static_cast<std::size_t>(k)] =
                            UploadTexturedMesh(device, fades[static_cast<std::size_t>(k)].Mesh());
                        drinkBubbleRenderers_[static_cast<std::size_t>(k)]->Draw(device, *objectEffect_);
                    }
                    objectEffect_->setAlphaProperty(1.0f);
                }
            }
        }

        // Billboard rendering for object-m.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3, 2026-07-09) --
        // same camera-facing billboard technique as the element.png batch
        // above, but reuses terrainTexture_ (object-m.png) via
        // objectMPngEffect_, and looks up UVs through tileAtlas_ instead of
        // GetElementIconUv() (the same icon-index domain as terrain).
        if (objectMPngEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                const auto visual = ResolveObjectVisual(
                    obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                if (!obj.active || !visual.visible ||
                    visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::ObjectM)
                {
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::ObjectM, visual.icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = tileAtlas_.GetTileUv(visual.icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    uv);
            }

            if (!mesh.Mesh().Empty())
            {
                objectMPngMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                objectMPngEffect_->View = camera_.View();
                objectMPngEffect_->Projection = camera_.Projection();
                objectMPngEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                objectMPngMeshRenderer_->Draw(device, *objectMPngEffect_);
            }
        }

        // Billboard rendering for explo.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3, 2026-07-09) --
        // same camera-facing billboard technique as the batches above, but
        // exploTexture_ (a genuinely new texture) via exploEffect_, and
        // GetExploIconUv() instead of GetElementIconUv()/tileAtlas_.
        if (exploEffect_ &&
            (!worldRuntime_.GetMobileObjects().empty() || BlupiAuraShown() == GalaxyEggbert::Def::SecretPower::Cloud))
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            // The Cloud's three sparks round him (explo.png 48 + (t + k) % 6, 128 px centred on his
            // sprite: pos - 34, Decor.cpp:807-818), a little behind him as the original draws them first.
            if (BlupiAuraShown() == GalaxyEggbert::Def::SecretPower::Cloud)
            {
                const auto behind = invView.getForwardProperty() * 0.35f;
                const Microsoft::Xna::Framework::Vector3 centre(blupi_.GetX(), blupi_.GetY() - 0.5f + 30.0f / 64.0f,
                                                                blupi_.GetZ());
                for (int k = 0; k < 3; ++k)
                {
                    const auto uv = GetExploIconUv(48 + (worldRuntime_.GetAnimPhase() + k) % 6);
                    mesh.AddBillboard(centre + behind, Microsoft::Xna::Framework::Vector2(128.0f / 64.0f, 128.0f / 64.0f),
                                      cameraRight, cameraUp, CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
                }
            }
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                const auto visual = ResolveObjectVisual(
                    obj.type, static_cast<int>(obj.phase), obj.visualIcon);
                if (!obj.active || visual.renderMode != ObjectRenderMode::Billboard ||
                    visual.textureSource != ObjectTextureSource::Explo)
                {
                    continue;
                }
                // Real -1 "invisible frame" sentinel (plan.md VISUAL-009,
                // table_sploutch2/3's own leading delay ticks, fixed
                // 2026-07-14) -- skip drawing this object entirely this
                // tick, matching the real source's own blank-frame behavior.
                if (!visual.visible)
                {
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Explo, visual.icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = GetExploIconUv(visual.icon);
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }

            if (!mesh.Mesh().Empty())
            {
                exploMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                exploEffect_->View = camera_.View();
                exploEffect_->Projection = camera_.Projection();
                exploEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                exploMeshRenderer_->Draw(device, *exploEffect_);
            }
        }

        // Billboard rendering for Blupi-skin MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3, 2026-07-09) --
        // ObjectType200 via blupiObjectEffect_ (blupi.png), ObjectType201/
        // 202/203 via blupi1ObjectEffect_ (blupi1.png) -- two separate
        // batches since
        // BasicEffect only binds one texture at a time.
        if (blupiObjectEffect_ && blupi1ObjectEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto blupiMesh = MakeTexturedMeshBuilder();
            auto blupi1Mesh = MakeTexturedMeshBuilder();
            constexpr float kObjectSize = 1.0f;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                const int objPhase = static_cast<int>(obj.phase);
                const auto visual =
                    ResolveObjectVisual(obj.type, objPhase, obj.visualIcon);
                if (!obj.active || !visual.visible ||
                    visual.renderMode != ObjectRenderMode::Billboard ||
                    (visual.textureSource != ObjectTextureSource::Blupi &&
                     visual.textureSource != ObjectTextureSource::Blupi1))
                {
                    continue;
                }
                // The enemy Blupis' patrol tables (ObjectSprite), unless drawn as his model.
                ObjectTextureSource sheet{};
                int icon = visual.icon;
                if (EnemyBlupiModelDrawn(obj) || !ObjectSprite(obj, sheet, icon) ||
                    objectModelTable_.Find(visual.textureSource, icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto uv = GetBlupiIconUv(icon);
                auto& mesh = visual.textureSource == ObjectTextureSource::Blupi1
                    ? blupi1Mesh : blupiMesh;
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        obj.currentX, ResolveObjectVisualCenterY(obj.type, obj.currentY), obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }

            if (!blupiMesh.Mesh().Empty())
            {
                blupiObjectMeshRenderer_ = UploadTexturedMesh(device, blupiMesh.Mesh());
                blupiObjectEffect_->View = camera_.View();
                blupiObjectEffect_->Projection = camera_.Projection();
                blupiObjectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                blupiObjectMeshRenderer_->Draw(device, *blupiObjectEffect_);
            }

            if (!blupi1Mesh.Mesh().Empty())
            {
                blupi1ObjectMeshRenderer_ = UploadTexturedMesh(device, blupi1Mesh.Mesh());
                blupi1ObjectEffect_->View = camera_.View();
                blupi1ObjectEffect_->Projection = camera_.Projection();
                blupi1ObjectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                blupi1ObjectMeshRenderer_->Draw(device, *blupi1ObjectEffect_);
            }
        }

        // Billboard rendering for worldRuntime_'s parsed BigDecor: cells.
        // The source renderer uses CHEXPLO for this layer, so the same
        // explo.png atlas/UV convention as explosion-channel MoveObjects is
        // intentional. Cells may come from either a mobile BigDecor: grid
        // or sparse 3D `.vwr` BigDecorRecord metadata.
        if (bigDecorEffect_ && !bigDecorCells_.empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.View());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            auto mesh = MakeTexturedMeshBuilder();
            auto marineMesh = MakeTexturedMeshBuilder();
            constexpr float kBigDecorSize = 1.0f;
            for (const auto& cell : bigDecorCells_)
            {
                if (cell.icon == GalaxyEggbert::BlockTypes::Marine)
                {
                    // BigDecor icon 203 is not an explo.png icon: the original
                    // draws the object channel's marine animation there
                    // (Decor.cpp: `num2 = table_marine[m_time / ScaleDiv(3) % 11];
                    // channel = Object`), the Marine block's animation.
                    float u = 0.0f, v = 0.0f, us = 0.0f, vs = 0.0f;
                    GalaxyEggbert::BlockTypes::tileUV(
                        GalaxyEggbert::ResolveBlockAnimationIcon(GalaxyEggbert::BlockTypes::Marine,
                                                                 worldRuntime_.GetAnimPhase()),
                        u, v, us, vs);
                    marineMesh.AddBillboard(
                        Microsoft::Xna::Framework::Vector3(cell.worldX, cell.worldY, cell.worldZ),
                        Microsoft::Xna::Framework::Vector2(kBigDecorSize, kBigDecorSize), cameraRight, cameraUp,
                        CnaEngine::Graphics::UvRect{u, v, u + us, v + vs});
                    continue;
                }
                if (objectModelTable_.Find(ObjectTextureSource::Explo, cell.icon) != nullptr)
                {
                    continue;  // drawn as its 3D model
                }
                const auto iconUv = GetBigDecorIconUv(static_cast<int>(cell.icon));
                mesh.AddBillboard(
                    Microsoft::Xna::Framework::Vector3(
                        cell.worldX, cell.worldY, cell.worldZ),
                    Microsoft::Xna::Framework::Vector2(kBigDecorSize, kBigDecorSize), cameraRight, cameraUp,
                    CnaEngine::Graphics::UvRect{
                        iconUv.U0, iconUv.V0, iconUv.U1, iconUv.V1});
            }

            if (!mesh.Mesh().Empty())
            {
                bigDecorMeshRenderer_ = UploadTexturedMesh(device, mesh.Mesh());
                bigDecorEffect_->View = camera_.View();
                bigDecorEffect_->Projection = camera_.Projection();
                bigDecorEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                bigDecorMeshRenderer_->Draw(device, *bigDecorEffect_);
            }
            if (objectMPngEffect_ && !marineMesh.Mesh().Empty())
            {
                bigDecorMarineMeshRenderer_ = UploadTexturedMesh(device, marineMesh.Mesh());
                objectMPngEffect_->View = camera_.View();
                objectMPngEffect_->Projection = camera_.Projection();
                objectMPngEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                bigDecorMarineMeshRenderer_->Draw(device, *objectMPngEffect_);
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
                                    fadeOutPhase_, TerrainReliefWanted());
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
