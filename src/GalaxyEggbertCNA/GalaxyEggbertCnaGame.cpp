#include "GalaxyEggbertCnaGame.hpp"

#include "GalaxyEggbert/BlockTypes.hpp"

#include <Easy3D/BillboardBatch.hpp>
#include <Easy3D/BillboardMesh.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <vector>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Placeholder-model animation mapping (2026-07-09, NEXT.md §3) --
        // avatars3d/blupi_placeholder/'s 3 clips (Survey/Walk/Run) don't
        // correspond to GEBlupiController::AnimState's states at all (6 as
        // of 2026-07-11's Jump/Air split, plan.md E3D-MIG-064), so
        // this is a rough best-effort substitution, not a faithful
        // behavioral mapping -- see avatars3d/blupi_placeholder/README.md's
        // own mapping table for the reasoning per state. Swap out entirely
        // once a real Blupi model with real matching clips exists.
        const std::string& BlupiAnimStateToPlaceholderClipName(GEBlupiController::AnimState state)
        {
            static const std::string kSurvey = "Survey";
            static const std::string kWalk = "Walk";
            static const std::string kRun = "Run";
            switch (state)
            {
                case GEBlupiController::AnimState::March: return kWalk;
                case GEBlupiController::AnimState::Jump:
                case GEBlupiController::AnimState::Air:   return kRun;
                case GEBlupiController::AnimState::Stop:
                case GEBlupiController::AnimState::Down:
                case GEBlupiController::AnimState::Up:
                default:
                    return kSurvey;
            }
        }
    }

    GalaxyEggbertCnaGame::GalaxyEggbertCnaGame()
    {
        Game::getWindowProperty().setTitleProperty("Galaxy Eggbert (CNA)");
        // Standard XNA pattern: the manager must be constructed in the game
        // constructor (before Run() initializes the device). Only used for
        // F11's ToggleFullScreen() so far.
        graphics_ = std::make_unique<Microsoft::Xna::Framework::GraphicsDeviceManager>(this);
    }

    void GalaxyEggbertCnaGame::LoadContent()
    {
        // Default world source: a genuinely 3D, hand-authored .vwr world
        // (plan.md E3D-MIG-058), not a flat mobile-eggbert .txt layout.
        // LoadFromMobileEggbertFile() stays available on GEWorldRuntime as a
        // secondary/reference path (e.g. for later faithful-remake level
        // porting) but is no longer the default load here.
        const bool loaded = worldRuntime_.LoadFromVwrFile("worlds3d/world001.vwr");
        if (!loaded)
        {
            std::cout << "GalaxyEggbertCNA: worlds3d/world001.vwr not found or failed to load "
                         "(run the binary from its own build directory)." << std::endl;
            return;
        }

        const auto& world = worldRuntime_.GetWorld();
        int nonAirBlocks = 0;
        int minY = -1;
        int maxY = -1;
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        for (int y = 0; y < blocksPerAxis; ++y)
        {
            for (int z = 0; z < blocksPerAxis; ++z)
            {
                for (int x = 0; x < blocksPerAxis; ++x)
                {
                    if (!world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                         static_cast<std::uint16_t>(z)).isAir())
                    {
                        ++nonAirBlocks;
                        if (minY < 0) minY = y;
                        maxY = y;
                    }
                }
            }
        }

        std::cout << "GalaxyEggbertCNA: loaded worlds3d/world001.vwr — "
                  << nonAirBlocks << " non-air blocks, Y range [" << minY << ", " << maxY
                  << "]." << std::endl;

        // Tile-atlas sanity check (plan.md E3D-MIG-053) — nothing queues a
        // CubeBatch item from this yet; this only proves GETileAtlas resolves
        // known block types to plausible UV rects.
        const auto printTileUv = [this](const char* name, int blockType)
        {
            const auto uv = tileAtlas_.GetTileUv(blockType);
            std::cout << "GalaxyEggbertCNA: tile " << name << " (" << blockType
                      << ") UV = (" << uv.U0 << ", " << uv.V0 << ") - ("
                      << uv.U1 << ", " << uv.V1 << ")" << std::endl;
        };
        printTileUv("Ground", GalaxyEggbert::BlockTypes::Ground);
        printTileUv("Lava", GalaxyEggbert::BlockTypes::Lava);
        printTileUv("GoldPillar", GalaxyEggbert::BlockTypes::GoldPillar);

        // Static terrain mesh for the loaded world (plan.md E3D-MIG-054) —
        // one CubeBatch item per non-air cell, textured via GETileAtlas.
        auto& device = getGraphicsDeviceProperty();

        // Real mobile-eggbert background image for this world's skyRegion
        // (NEXT.md §3, 2026-07-09) -- direct filename formula, same
        // convention as mobile-eggbert's own Decor::LoadImages()
        // (05-backgrounds.md): "decor" + 3-digit zero-padded region id.
        // Content/backgrounds/ is already copied next to this binary (see
        // the mobile-eggbert Content/ POST_BUILD copy, CMakeLists.txt). Only
        // 28 of the 32 possible region ids (0-31) have a real file -- the 4
        // missing ones are confirmed never used by any real level
        // (05-backgrounds.md), but a hand-authored .vwr world could still
        // reference one, so this must degrade gracefully, not throw.
        {
            char backgroundPath[64];
            std::snprintf(backgroundPath, sizeof(backgroundPath),
                          "Content/backgrounds/decor%03u.png", worldRuntime_.GetSkyRegion());
            if (std::filesystem::exists(backgroundPath))
            {
                backgroundTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(backgroundPath, device);
                backgroundEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
                backgroundEffect_->VertexColorEnabled = false;
                backgroundEffect_->setTextureEnabledProperty(true);
                backgroundEffect_->setTextureProperty(&backgroundTexture_);
                backgroundLoaded_ = true;
                std::cout << "GalaxyEggbertCNA: background loaded — " << backgroundPath << " ("
                          << backgroundTexture_.getWidthProperty() << "x"
                          << backgroundTexture_.getHeightProperty() << " px, region "
                          << worldRuntime_.GetSkyRegion() << ")." << std::endl;
            }
            else
            {
                std::cout << "GalaxyEggbertCNA: no background image for region "
                          << worldRuntime_.GetSkyRegion() << " (" << backgroundPath
                          << " not found) — falling back to flat clear color." << std::endl;
            }
        }

        terrainRenderer_ = std::make_unique<GETerrainRenderer>(device, world, tileAtlas_);

        // object-m.png was already copied next to this binary at build time
        // (plan.md E3D-MIG-030); loaded directly via CNA's own Texture2D —
        // no mobile-eggbert Pixmap reuse (SpriteBatch-coupled, not usable for
        // this 3D BasicEffect draw path).
        terrainTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/object-m.png", device);
        std::cout << "GalaxyEggbertCNA: terrain texture loaded — "
                  << terrainTexture_.getWidthProperty() << "x"
                  << terrainTexture_.getHeightProperty() << " px." << std::endl;

        terrainEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        terrainEffect_->VertexColorEnabled = false;
        terrainEffect_->setTextureEnabledProperty(true);
        terrainEffect_->setTextureProperty(&terrainTexture_);

        // Icon 107's grass-top overlay (NEXT.md §8 task 3) — genuinely
        // separate from object-m.png, copied next to this binary from
        // textures3d/ (like worlds3d/, see CMakeLists.txt's POST_BUILD
        // step), not from mobile-eggbert's Content/.
        grassTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("textures3d/grass_top.png", device);
        std::cout << "GalaxyEggbertCNA: grass texture loaded — "
                  << grassTexture_.getWidthProperty() << "x"
                  << grassTexture_.getHeightProperty() << " px." << std::endl;
        grassEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        grassEffect_->VertexColorEnabled = false;
        grassEffect_->setTextureEnabledProperty(true);
        grassEffect_->setTextureProperty(&grassTexture_);

        // Real mobile-eggbert bottom HUD + the interim animation-state
        // indicator (2026-07-10, see GEHud.hpp -- loads its own texture
        // instances, including text.png/pad.png which nothing else loads).
        hud_.LoadContent(device);

        // Billboard rendering for MoveObjects (15-3d-render-mapping-design.md
        // §5/§7) — element.png, same asset already used by GalaxyEggbertSimple3D.
        objectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/element.png", device);
        objectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        objectEffect_->VertexColorEnabled = false;
        objectEffect_->setTextureEnabledProperty(true);
        objectEffect_->setTextureProperty(&objectTexture_);
        std::cout << "GalaxyEggbertCNA: " << worldRuntime_.GetMobileObjects().size()
                  << " MoveObject(s) parsed for billboard rendering." << std::endl;

        // Billboard rendering for the 5 object-m.png-sourced MoveObjects
        // (GEObjectIcons::IsObjectMPngSourced, NEXT.md §3) -- reuses
        // terrainTexture_ (object-m.png), same reasoning as bigDecorEffect_.
        objectMPngEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        objectMPngEffect_->VertexColorEnabled = false;
        objectMPngEffect_->setTextureEnabledProperty(true);
        objectMPngEffect_->setTextureProperty(&terrainTexture_);

        // Billboard rendering for the 12 explo.png-sourced MoveObjects
        // (GEObjectIcons::IsExploPngSourced, NEXT.md §3) -- explo.png isn't
        // loaded anywhere else in GalaxyEggbertCNA, so this is a genuinely
        // new texture load (already copied next to this binary at build
        // time, same mechanism as element.png/object-m.png).
        exploTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/explo.png", device);
        exploEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        exploEffect_->VertexColorEnabled = false;
        exploEffect_->setTextureEnabledProperty(true);
        exploEffect_->setTextureProperty(&exploTexture_);

        // Billboard rendering for the 4 Blupi-skin MoveObjects
        // (GEObjectIcons::IsBlupiPngSourced, NEXT.md §3) -- separate
        // Texture2D instances from blupiIconTexture_ above even though
        // ObjectType200 loads the same blupi.png file, since that one is
        // owned by the 2D SpriteBatch HUD path, not this 3D BasicEffect path.
        blupiObjectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/blupi.png", device);
        blupiObjectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        blupiObjectEffect_->VertexColorEnabled = false;
        blupiObjectEffect_->setTextureEnabledProperty(true);
        blupiObjectEffect_->setTextureProperty(&blupiObjectTexture_);

        blupi1ObjectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/blupi1.png", device);
        blupi1ObjectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        blupi1ObjectEffect_->VertexColorEnabled = false;
        blupi1ObjectEffect_->setTextureEnabledProperty(true);
        blupi1ObjectEffect_->setTextureProperty(&blupi1ObjectTexture_);

        // Billboard rendering for BigDecor: cells (NEXT.md §8 task 3) —
        // reuses terrainTexture_ (object-m.png), same icon vocabulary as the
        // main terrain grid, via a dedicated effect (BasicEffect only binds
        // one texture at a time). Filtered once here into bigDecorCells_ so
        // Draw() doesn't re-scan the full 100x100 grid every frame.
        bigDecorEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        bigDecorEffect_->VertexColorEnabled = false;
        bigDecorEffect_->setTextureEnabledProperty(true);
        bigDecorEffect_->setTextureProperty(&terrainTexture_);

        const auto& bigDecor = worldRuntime_.GetBigDecor();
        constexpr int kBigDecorGridSize = 100;
        // LoadFromVwrFile() (the default world source) clears bigDecor_ to
        // empty -- only LoadFromMobileEggbertFile() populates the full
        // 100x100 grid. Guard against indexing an empty vector (segfault,
        // found live 2026-07-09): only scan when the grid is actually the
        // expected full size.
        if (bigDecor.size() == static_cast<std::size_t>(kBigDecorGridSize) * kBigDecorGridSize)
        {
            for (int row = 0; row < kBigDecorGridSize; ++row)
            {
                for (int col = 0; col < kBigDecorGridSize; ++col)
                {
                    const std::uint16_t icon = bigDecor[
                        static_cast<std::size_t>(row) * kBigDecorGridSize + static_cast<std::size_t>(col)];
                    if (icon == GalaxyEggbert::BlockTypes::Air)
                    {
                        continue;
                    }
                    bigDecorCells_.push_back({
                        static_cast<float>(col - GEWorldRuntime::kWorldCenterX),
                        static_cast<float>(row - GEWorldRuntime::kWorldCenterZ),
                        icon,
                    });
                }
            }
        }
        std::cout << "GalaxyEggbertCNA: " << bigDecorCells_.size()
                  << " BigDecor cell(s) parsed for billboard rendering." << std::endl;

        // Platform-lift/crate UniformCube object path (NEXT.md §8 task 3) --
        // reuses terrainTexture_ (object-m.png), the confirmed-correct sheet
        // for these ObjectTypes (see GEObjectIcons::IsUniformCubeObject).
        // Only the effect is set up here; the mesh itself is rebuilt every
        // frame in Draw() now (2026-07-09, NEXT.md §8 task 3 optional
        // follow-up), same reason as the billboards -- types 47/48
        // (chenille caterpillar tracks) have real phase-cycling icon
        // formulas that need a fresh icon/UV lookup each frame to actually
        // animate, unlike a build-once-and-cache mesh.
        {
            objectCubeEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
            objectCubeEffect_->VertexColorEnabled = false;
            objectCubeEffect_->setTextureEnabledProperty(true);
            objectCubeEffect_->setTextureProperty(&terrainTexture_);

            int cubeObjectCount = 0;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                cubeObjectCount += IsUniformCubeObject(obj.type) ? 1 : 0;
            }
            std::cout << "GalaxyEggbertCNA: " << cubeObjectCount
                      << " platform-lift/crate cube object(s) found." << std::endl;
        }

        // Third-person-mode 3D model (2026-07-09, NEXT.md §3) -- CNA's
        // AvatarRenderer real-rendering extension, see this class's own
        // header comment and ../cna/docs/avatar-real-rendering-ext.md.
        // ContentManager's RootDirectory is pointed at avatars3d/ (not the
        // default "Content", which is mobile-eggbert's own tree, fully
        // re-copied fresh on every build -- see the textures3d/ comment
        // above for the same reasoning) so Load<>'s asset name is relative
        // to avatars3d/ itself. Currently unused elsewhere in this class,
        // so repointing its RootDirectory here can't break anything else.
        {
            const std::string blupiModelManifest = "avatars3d/blupi_placeholder/avatar.skinnedmodel.json";
            if (std::filesystem::exists(blupiModelManifest))
            {
                auto& content = getContentProperty();
                content.setRootDirectoryProperty("avatars3d");
                blupiModel_ = content.Load<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>(
                    "blupi_placeholder/avatar");

                blupiAvatarRenderer_ = std::make_unique<Microsoft::Xna::Framework::GamerServices::AvatarRenderer>(nullptr);
                blupiAvatarRenderer_->EnableRealRenderingEXT(device, blupiModel_);
                // LightColor/AmbientLightColor/LightDirection default to
                // black (matching the real, never-drawing XNA Avatar API's
                // untouched value-type defaults) -- DrawRealEXT renders
                // nothing visible until these are set (see the doc above).
                blupiAvatarRenderer_->setAmbientLightColorProperty(
                    Microsoft::Xna::Framework::Vector3(0.35f, 0.35f, 0.35f));
                blupiAvatarRenderer_->setLightColorProperty(
                    Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f));
                blupiAvatarRenderer_->setLightDirectionProperty(
                    Microsoft::Xna::Framework::Vector3(-0.4f, -0.6f, -0.7f));
                blupiModelLoaded_ = true;
                std::cout << "GalaxyEggbertCNA: third-person placeholder model loaded — "
                             "avatars3d/blupi_placeholder/ ('C' toggles first-/third-person)."
                          << std::endl;
            }
            else
            {
                std::cout << "GalaxyEggbertCNA: no third-person model found ("
                          << blupiModelManifest
                          << " not found) — third-person camera mode unavailable, staying first-person."
                          << std::endl;
            }
        }

        // Spawn Blupi on the ground floor (world (0,1,0) == grid (50,*,50),
        // inside worlds3d/world001.vwr's ground floor). The .vwr format
        // carries no spawn point itself (see LoadFromVwrFile()), so this is
        // a fixed known-good spot for this specific sample world. The
        // camera now follows blupi_ every frame (see Update()) instead of a
        // fixed terrain-centroid shot.
        blupi_.SetPosition(0.0f, 1.0f, 0.0f);

        // Real mobile-eggbert sound playback (2026-07-10, see GESound.hpp) --
        // loads whichever of Content/sounds/sound000.wav..sound092.wav
        // actually exist (already copied next to the binary by the
        // mobile-eggbert Content/ POST_BUILD step, same as icons/
        // backgrounds).
        sound_.LoadContent();
        std::cout << "GalaxyEggbertCNA: sound loaded — " << sound_.LoadedCount() << "/"
                  << GESound::kNumChannels << " channel(s)." << std::endl;

        std::cout << "GalaxyEggbertCNA: terrain mesh uploaded — "
                  << terrainRenderer_->BlockCount() << " blocks ("
                  << terrainRenderer_->AnimatedBlockCount() << " animated, "
                  << terrainRenderer_->WaterBlockCount() << " water/semi-transparent), "
                  << terrainRenderer_->VertexCount() << " vertices, "
                  << terrainRenderer_->PrimitiveCount() << " triangles." << std::endl;
    }

    void GalaxyEggbertCnaGame::Update(Microsoft::Xna::Framework::GameTime& gameTime)
    {
        Game::Update(gameTime);

        const float dt = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        worldRuntime_.Update(dt);

        if (terrainRenderer_)
        {
            terrainRenderer_->Update(getGraphicsDeviceProperty(), worldRuntime_.GetAnimPhase());

            // Tank controls (2026-07-05, matches GalaxyEggbertSimple3D's
            // already-shipped "Move" axis scheme): Left/Right turn, Up/Down
            // move forward/back along the current facing — arrows are not
            // a strafe pad. LCtrl jumps and Space is reserved for "Action"
            // (matches mobile-eggbert's own InputPad.cpp key glyphs exactly:
            // LeftControl -> PlayJump, Space -> PlayAction) — Space is read
            // here but not wired to anything yet, since there is no
            // interactive-object system in GalaxyEggbertCNA yet. LShift
            // crouches, RShift looks up (mirrors Simple3D's Down/Up
            // BlupiState).
            using Microsoft::Xna::Framework::Input::Keyboard;
            using Microsoft::Xna::Framework::Input::Keys;
            const auto keys = Keyboard::GetState();
            float turnInput = 0.0f;
            float moveInput = 0.0f;
            if (keys.IsKeyDown(Keys::Left))  turnInput -= 1.0f;
            if (keys.IsKeyDown(Keys::Right)) turnInput += 1.0f;
            if (keys.IsKeyDown(Keys::Up))    moveInput += 1.0f;
            if (keys.IsKeyDown(Keys::Down))  moveInput -= 1.0f;
            const bool jumpPressed = keys.IsKeyDown(Keys::LeftControl);
            const bool actionPressed = keys.IsKeyDown(Keys::Space);
            const bool crouchHeld = keys.IsKeyDown(Keys::LeftShift);
            const bool lookUpHeld = keys.IsKeyDown(Keys::RightShift);
            const bool wasOnGround = blupi_.IsOnGround();
            const bool wasEcrased = blupi_.IsEcrased();
            const bool wasTeleporting = blupi_.IsTeleporting();
            // Compared at the very end of Update() (not right after Step(),
            // like wasEcrased above) since the balloon can also end via
            // interaction_.Update() -> PopBalloon() later this same frame,
            // not just Step()'s own natural-timeout countdown.
            const bool wasBallooned = blupi_.IsBallooned();
            const float blupiXBeforeStep = blupi_.GetX();
            // Vanishing/Temp tile (plan.md E3D-MIG-146) -- computed once
            // per frame from the same 20-ticks/sec animPhase_ every other
            // per-tile timing cycle here already uses (Blitz/Crusher), then
            // threaded into Step() itself (not a post-hoc GetGroundBlockType()
            // check like every other hazard) since this changes whether the
            // tile IS the ground at all, not just what Blupi is standing on.
            const bool tempPassable = GEWorldRuntime::IsTempPassableAtPhase(worldRuntime_.GetAnimPhase());
            blupi_.Step(worldRuntime_.GetWorld(), turnInput, moveInput, jumpPressed,
                        crouchHeld, lookUpHeld, dt, tempPassable);

            // Real mobile-eggbert jump/land/footstep sounds (2026-07-10).
            // jumpPressed is edge-detected the same way "C" is below, gated
            // on wasOnGround so holding the key while airborne doesn't
            // replay the jump sound.
            if (jumpPressed && !jumpKeyWasDown_ && wasOnGround)
            {
                sound_.PlayJump();
            }
            // Real mobile-eggbert also suppresses this generic landing-thud
            // sound specifically when the landing spot is a spring
            // (Decor.cpp ~2984, `if (!IsRessort(end))`) -- the bounce sound
            // below covers it instead, since Blupi is about to launch back
            // upward immediately rather than coming to rest.
            if (!wasOnGround && blupi_.IsOnGround() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) != GalaxyEggbert::BlockTypes::Spring)
            {
                sound_.PlayLand();
            }
            if (blupi_.GetAnimState() == GEBlupiController::AnimState::March)
            {
                // kStepSoundInterval is a reasonable-sounding approximation,
                // NOT sourced from mobile-eggbert's real march-cycle timing
                // (unverified against Decor.cpp/Tables.cpp) -- revisit once
                // that's checked, same as kMarchFrames' own animation timing.
                constexpr float kStepSoundInterval = 0.3f;
                stepSoundTimer_ += dt;
                if (stepSoundTimer_ >= kStepSoundInterval)
                {
                    stepSoundTimer_ -= kStepSoundInterval;
                    sound_.PlayStep();
                }
            }
            else
            {
                stepSoundTimer_ = 0.0f;
            }
            jumpKeyWasDown_ = jumpPressed;

            // Crusher squash recovery sound (real channel 41, per
            // Decor.cpp:5180-5197 -- the same "buff expired" channel other
            // timed states reuse on their own recovery, not a dedicated
            // crusher-only sound). Entry sound (channel 70) is played at the
            // trigger site below instead, since only that call site knows
            // whether this is a genuinely new trigger (TriggerCrush()
            // returns false, a no-op, if already squashed).
            if (wasEcrased && !blupi_.IsEcrased())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel41);
            }

            // Teleporter transit completion (plan.md E3D-MIG-147, real
            // Decor.cpp:6349-6358) -- fires the one frame the kTeleportDuration
            // countdown naturally elapses inside Step() (blupi_ was fully
            // frozen for the whole transit, so this is the only way it ends,
            // unlike balloon's PopBalloon() interrupt path). Looks up the
            // paired destination via the same icon TriggerTeleport() was
            // called with; relocates Blupi there and plays the real arrival
            // sound (channel 71, reused -- the real source's two
            // ObjectType27 arrival-particle bursts are cosmetic, not
            // modeled, same as every other hazard's real particle effects).
            // If no paired cell exists anywhere in the grid, real behavior
            // is "regains control in place" -- a silent no-op, which is
            // already true here since blupi_'s position was never touched.
            if (wasTeleporting && !blupi_.IsTeleporting())
            {
                float destX, destY, destZ;
                if (worldRuntime_.FindTeleportDestination(blupi_.GetTeleportIcon(), blupi_.GetX(), blupi_.GetY(),
                                                            blupi_.GetZ(), destX, destY, destZ))
                {
                    blupi_.SetPosition(destX, destY, destZ);
                    sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel71);
                }
            }

            // Shared death consequence (2026-07-11, plan.md E3D-MIG-067
            // subset) -- takes the real death sound channel for the
            // specific cause (channel 8: fall-off-world/lava, the real
            // shared "you died" sound per 07-sounds.md; channel 51: spikes/
            // drip/saw's real Glu-death sound, distinct from channel 8's).
            // Respawns at the real 10-slot "last safe position" FIFO
            // (GEBlupiController::GetValidX/Y/Z(), plan.md E3D-MIG-067,
            // done 2026-07-11) instead of the fixed spawn point. Does not
            // yet distinguish death *animations* (Clear1-8/Glu each have
            // their own real fixed duration and revival behavior,
            // 10-blupi-mechanics.md §8) -- every cause here is instant, no
            // animation state exists yet for any of them.
            const auto triggerDeath = [this](GalaxyEggbert::SoundChannel channel)
            {
                sound_.Play(channel);
                interaction_.LoseLife();
                blupi_.SetPosition(blupi_.GetValidX(), blupi_.GetValidY(), blupi_.GetValidZ());
            };

            // Fall-off-world death -- mobile-eggbert-reference/
            // 10-blupi-mechanics.md §8: "Walking off the world's bottom
            // row ... -> Clear2", checked before the rest of the frame
            // runs. kFallDeathY is a simplification of the real grid-row
            // check (`(end.Y+30)/64 >= 99`, Decor.cpp:2754) -- this world's
            // real floors/hazards all sit at Y>=0 (the sample world's
            // water/pit hazard is only 1 block deep), so any Y clearly
            // below that means Blupi fell through a hole with nothing
            // under it, not a legitimate low point in the level.
            //
            // **-60.0f, not -5.0f (user-reported, 2026-07-11): the real
            // fall is NOT near-instant.** Directly inspected
            // `../mobile-eggbert/worlds/world001.txt`: its real terrain
            // (`Decor:` grid) occupies only rows 0-21, leaving rows 22-98
            // (77 rows, ~4928 real px) of completely empty grid before the
            // real row-99 death check ever fires -- a deliberate, generous
            // "pit of doom" margin, not a tight threshold. Verified real
            // gravity directly against Decor.cpp:2966-2968 (`end.Y +=
            // (int)(m_blupiVitesseY * 2.0); if (m_blupiVitesseY < 20.0)
            // m_blupiVitesseY += 2.0;`, starting at 1.0): terminal velocity
            // is actually 21, not literally 20 (the uncapped +2 increment
            // overshoots the `<20` gate by one step) -- 840 real px/sec
            // (13.125 tiles/sec) at the real 20Hz reference rate. Falling
            // that observed ~4928px margin at these real constants takes
            // ~6.1s (the real ramp-up to terminal covers the first ~200px
            // in 0.5s, the remaining ~4728px at 840px/s takes ~5.6s) --
            // several seconds, matching the user's recollection of a real,
            // noticeable fall, not the sub-1-second death the old -5.0f
            // threshold gave with this engine's own gravity. -60.0f
            // reproduces a comparable ~6.2s fall using this engine's own
            // already-tuned kGravity=25/kFallLimit=-10 (not the real
            // tick-domain values, which aren't cross-checked against this
            // engine's own constants per plan.md `065`) -- an equivalent
            // NUMBER OF SECONDS, not the same literal unit distance, since
            // this world's own terrain (Y 0-13) is far shorter than a real
            // level's, so matching real "feel" (a real fall you notice)
            // makes more sense here than matching the real absolute margin.
            constexpr float kFallDeathY = -60.0f;
            if (blupi_.GetY() < kFallDeathY)
            {
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel8);
            }

            // Lava hazard (plan.md E3D-MIG-140) -- deterministic death, no
            // immunity of any kind (mobile-eggbert-reference/
            // 12-hazards-and-interactables.md: "Lava(icon 68): deterministic
            // Clear3, no vehicle immunity, no focus requirement" -- the
            // simplest of the 5 real isHazard() tile types to implement
            // faithfully for exactly that reason; Crusher/Saw/Blitz each
            // have real gating/timing conditions BlockTypes::isHazard() does
            // NOT encode (that helper is Simple3D-only, a uniform "any
            // hazard kills" shortcut -- not reused here since it would be
            // unfaithful for those types), so each gets its own dedicated
            // task (E3D-MIG-142..144) instead of one generic "hazard" check.
            // GetGroundBlockType() already gates on IsOnGround() -- Lava
            // tiles are solid/walkable-on in this engine (Blupi stands on
            // lava rather than falling through it, matching the real 2D
            // game -- see BlockTypes.hpp's isMobileTransparent() comment on
            // why Lava is deliberately kept solid despite being
            // quart-passable in the real source).
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Lava)
            {
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel8);
            }

            // Spikes hazard (plan.md E3D-MIG-141) -- real channel 51 (the
            // Glu-death sound, distinct from lava/fall's channel 8, per
            // 07-sounds.md). Real behavior (12-hazards-and-interactables.md)
            // gates this on vehicle immunity (Over/Jeep/Tank protect,
            // unlike lava) and requires m_blupiFocus -- neither vehicles
            // nor a focus concept exist in GalaxyEggbertCNA yet (Phase 17),
            // so this is currently unconditional, same simplification as
            // lava; revisit once vehicles exist. The real check also
            // restricts to a narrow central x-band within the tile
            // (`pos.X%64` roughly 15-49, touching the tile's edges doesn't
            // count) -- not modeled, since GEBlupiController's single-point
            // 3D collision has no sub-tile position within a cell to test
            // against; the whole tile is lethal here.
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spike)
            {
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel51);
            }

            // Blitz hazard (plan.md E3D-MIG-144) -- real channel 8, same
            // lethality profile as lava per mobile-eggbert-reference/
            // 12-hazards-and-interactables.md ("No vehicle immunity --
            // same lethality profile as lava; only Shield/Hide/SuperBlupi
            // protect, and there is no focus requirement") -- unlike
            // spikes/drip/saw, Blitz needed no immunity simplification to
            // implement faithfully. Real `BlitzActif()`: a 100-tick cycle
            // at the same 20-ticks/sec reference rate worldRuntime_'s
            // GetAnimPhase() already advances at (Config::ScaleTime(1),
            // matching the real per-tile animation-divisor rate), lethal
            // only on even ticks within the first half (`num%2==0 &&
            // num<50` -- a rapid flicker for ~2.5s, 25% duty cycle over the
            // full ~5s period, then fully inactive for the second half).
            // The real cosmetic emitter tile (icon 304, sits one cell
            // above 305, times a zap sound cue only) is NOT implemented --
            // audio-only polish, not the hazard itself.
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Blitz &&
                GEWorldRuntime::IsBlitzActiveAtPhase(worldRuntime_.GetAnimPhase()))
            {
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel8);
            }

            // Crusher hazard (plan.md E3D-MIG-143) -- unlike every hazard
            // above, this is NOT lethal: it squashes Blupi (reduced move
            // speed, no jump, ~10s auto-recovery) rather than costing a
            // life. Real `IsEcraseur()`'s 3-out-of-10 danger window is
            // approximated via `IsCrusherActiveAtPhase()` (see its own
            // comment for why the real unnormalized timer has no exact
            // equivalent here). `TriggerCrush()` itself is idempotent (a
            // no-op while already squashed, matching the real
            // `!m_blupiEcrase` re-trigger guard) -- only play the real
            // entry sound (channel 70) when it actually starts a NEW squash.
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Crusher &&
                GEWorldRuntime::IsCrusherActiveAtPhase(worldRuntime_.GetAnimPhase()) &&
                blupi_.TriggerCrush())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel70);
            }

            // Saw hazard (plan.md E3D-MIG-142) -- real channel 75 (the
            // "cut apart" death cue, distinct from every other hazard's
            // sound so far), deterministic, no immunity simplification
            // beyond what every hazard here already lacks (vehicles/focus
            // don't exist yet). Only the active icon (`Saw`, 378) is
            // lethal -- the stopped variant (`SawStopped`, 379) is a
            // separate BlockTypes value, so GetGroundBlockType()'s exact
            // match already excludes it with no extra check needed. A saw
            // starts active or stopped per however the world was authored;
            // TryActivateSwitch() below is what flips it between the two.
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Saw)
            {
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel75);
            }

            // Spring / bounce tile (plan.md E3D-MIG-145, icon 211,
            // verified directly against Decor.cpp:2835-2911/7312-7320) --
            // NOT a hazard, launches Blupi upward instead of costing a
            // life. TriggerSpringBounce() is idempotent (a no-op while
            // already airborne, matching the real `!m_blupiAir` guard), so
            // only play the real bounce sound (channel 41) on an actual new
            // trigger. Real vehicle-dismount-first branches (Helico/Over/
            // Jeep/Tank/Skate) are NOT modeled -- no vehicle concept exists
            // in this engine yet, same simplification as every hazard
            // above.
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spring &&
                blupi_.TriggerSpringBounce(jumpPressed))
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel41);
            }

            // Teleporter (plan.md E3D-MIG-147, icons 330-333, verified
            // directly against Decor.cpp:7378-7394/5593-5606) -- checked
            // one cell ABOVE Blupi (GetBlockTypeAbove()), matching the real
            // detection geometry exactly. Teleporter icons are always
            // non-solid for collision (GroundHeightAt's own
            // IsTeleporterIcon() skip), so Blupi can genuinely walk into
            // the open space beneath a floating pillar to trigger this,
            // the same way real mobile-eggbert's per-tile-independent 2D
            // collision lets him walk under one (see GetBlockTypeAbove()'s
            // own comment). The real narrow 7px-wide left-edge sub-tile
            // band is NOT modeled -- no sub-tile position exists in this
            // engine's single-point collision, same simplification already
            // applied to spikes' own real sub-tile band. Real gate
            // (`!m_blupiHelico/Over/Balloon/Ecrase/Jeep/Tank/Skate &&
            // !m_blupiAir && m_blupiFocus`) is checked inside
            // TriggerTeleport() itself (grounded, not ballooned/squashed --
            // vehicles/focus aren't modeled). Idempotent, same shape as
            // every other Trigger*() here, so channel 71 only plays on an
            // actual new trigger.
            const auto aboveIcon = blupi_.GetBlockTypeAbove(worldRuntime_.GetWorld());
            if ((aboveIcon == GalaxyEggbert::BlockTypes::Teleport1 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport2 ||
                 aboveIcon == GalaxyEggbert::BlockTypes::Teleport3 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport4) &&
                blupi_.TriggerTeleport(aboveIcon))
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel71);
            }

            // Real 10-slot safe-position FIFO respawn (plan.md E3D-MIG-067,
            // see GEBlupiController::UpdateSafePosition()'s own comment) --
            // "safe" here additionally means not standing on any of the 5
            // real terrain hazard tiles (Lava/Spike/Saw/active-Blitz/Temp)
            // and not currently under a teleporter trigger (`aboveIcon`,
            // already computed above) -- the two categories of "unsafe
            // tile" this engine can actually check for; real vehicle/
            // shield/ledge-teeter/transport-riding/projectile-path checks
            // aren't modeled (see UpdateSafePosition's own comment).
            {
                const auto safetyGroundBlock = blupi_.GetGroundBlockType(worldRuntime_.GetWorld());
                const bool onHazardTile =
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Lava ||
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Spike ||
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Saw ||
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Temp ||
                    (safetyGroundBlock == GalaxyEggbert::BlockTypes::Blitz &&
                     GEWorldRuntime::IsBlitzActiveAtPhase(worldRuntime_.GetAnimPhase()));
                const bool underTeleporter =
                    aboveIcon == GalaxyEggbert::BlockTypes::Teleport1 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport2 ||
                    aboveIcon == GalaxyEggbert::BlockTypes::Teleport3 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport4;
                blupi_.UpdateSafePosition(!onHazardTile && !underTeleporter);
            }

            // Switches (plan.md E3D-MIG-142, see GEWorldRuntime::
            // TryActivateSwitch()'s own comment for the real 41-cell
            // switch-to-saw linking) -- Space ("Action"), edge-detected the
            // same way jumpPressed is above so holding it doesn't retoggle
            // every frame. `wasEcrased`/`wasOnGround`-style capture isn't
            // needed here: TryActivateSwitch() itself is the single
            // authoritative check (grounded + standing on a switch tile),
            // no separate before/after state to compare.
            if (actionPressed && !actionKeyWasDown_)
            {
                if (const auto turnedOn = worldRuntime_.TryActivateSwitch(
                        blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(), blupi_.IsOnGround()))
                {
                    sound_.Play(*turnedOn ? GalaxyEggbert::SoundChannel::SoundChannel77
                                          : GalaxyEggbert::SoundChannel::SoundChannel76);
                }
            }
            actionKeyWasDown_ = actionPressed;

            // Interactive objects (2026-07-10, see GEInteractionSystem.hpp)
            // -- platform lift patrol, crate push, pickup collection,
            // generic hazard contact (ObjectType2/3/4/16/17/20/96/97), and
            // (2026-07-11) the wasp's balloon status. Runs after
            // blupi_.Step() so blupi_'s position is this frame's final
            // value; blupiXBeforeStep lets the interaction system infer
            // movement direction for crate push without GEBlupiController
            // needing a velocity accessor. crouchHeld gates ObjectType3's
            // real duck-immunity; blupi_.IsBallooned() gates whether a
            // 3/16/96/97 hazard pops the balloon instead of killing.
            interaction_.Update(dt, worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                 blupi_.GetX() - blupiXBeforeStep, sound_, crouchHeld,
                                 blupi_.IsBallooned());

            // GEInteractionSystem has no access to GEBlupiController, so it
            // can only report that a hazard-contact death happened this
            // frame (DiedThisFrame()) -- respawn is applied here, same real
            // last-safe-position FIFO as the terrain-hazard deaths above.
            if (interaction_.DiedThisFrame())
            {
                blupi_.SetPosition(blupi_.GetValidX(), blupi_.GetValidY(), blupi_.GetValidZ());
            }

            // Wasp balloon status (plan.md E3D-MIG-135) -- TriggerBalloon()
            // is idempotent (a no-op while already ballooned, matching the
            // real `!m_blupiBalloon` re-trigger guard), so only play the
            // real entry sound (channel 40) on an actual new trigger.
            if (interaction_.BalloonTouchedThisFrame() && blupi_.TriggerBalloon())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel40);
            }
            if (interaction_.BalloonPoppedThisFrame())
            {
                blupi_.PopBalloon();
            }
            // Real recovery sound (channel 41) covers BOTH a natural
            // Step()-driven timeout AND the hazard-triggered pop just above
            // -- both funnel through the same IsBallooned() true->false
            // transition, so one comparison at the end of Update() (not
            // right after Step(), unlike wasEcrased's own check) catches
            // either cause.
            if (wasBallooned && !blupi_.IsBallooned())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel41);
            }

            // Camera-mode toggle (2026-07-09, NEXT.md §3) -- "C", edge-
            // detected (same pattern as demo_avatar's Space-toggle) so a
            // held key doesn't flip modes every frame. Only actually
            // switches to third-person if a model loaded successfully
            // (blupiModelLoaded_) -- otherwise silently stays first-person,
            // there being nothing to show in third-person.
            const bool cameraModeKeyDown = keys.IsKeyDown(Keys::C);
            if (cameraModeKeyDown && !cameraModeKeyWasDown_ && blupiModelLoaded_)
            {
                cameraMode_ = (cameraMode_ == CameraMode::FirstPerson)
                                  ? CameraMode::ThirdPersonModel
                                  : CameraMode::FirstPerson;
            }
            cameraModeKeyWasDown_ = cameraModeKeyDown;

            // F11 fullscreen toggle (2026-07-10, user request), edge-
            // detected like "C" above.
            const bool fullscreenKeyDown = keys.IsKeyDown(Keys::F11);
            if (fullscreenKeyDown && !fullscreenKeyWasDown_ && graphics_)
            {
                graphics_->ToggleFullScreen();
            }
            fullscreenKeyWasDown_ = fullscreenKeyDown;

            // Mouse drag-look (2026-07-10, user request): holding the left
            // button and dragging rotates the camera around Blupi without
            // turning him (yaw offset + clamped pitch offset on top of his
            // facing). Any movement input decays the offsets smoothly back
            // to zero, returning the camera behind him -- "look around,
            // then it snaps back when you walk". Drag (not free mouselook)
            // so it maps 1:1 onto touch input too (SDL reports touch drags
            // as mouse drags).
            {
                using Microsoft::Xna::Framework::Input::ButtonState;
                using Microsoft::Xna::Framework::Input::Mouse;
                const auto mouse = Mouse::GetState();
                const int mouseX = mouse.getXProperty();
                const int mouseY = mouse.getYProperty();
                const bool lookHeld = mouse.getLeftButtonProperty() == ButtonState::Pressed;
                if (lookHeld && mouseLookActive_)
                {
                    constexpr float kLookRadiansPerPixel = 0.008f;
                    constexpr float kPitchLimit = 1.2f;
                    lookYawOffset_ += static_cast<float>(mouseX - lastMouseX_) * kLookRadiansPerPixel;
                    lookPitchOffset_ += static_cast<float>(mouseY - lastMouseY_) * kLookRadiansPerPixel;
                    lookPitchOffset_ = std::clamp(lookPitchOffset_, -kPitchLimit, kPitchLimit);
                }
                mouseLookActive_ = lookHeld;
                lastMouseX_ = mouseX;
                lastMouseY_ = mouseY;

                if ((moveInput != 0.0f || turnInput != 0.0f) && !lookHeld)
                {
                    // Same framerate-independent exponential decay shape as
                    // the camera damping below.
                    constexpr float kLookReturnPerSecond = 5.0f;
                    const float keep = std::exp(-kLookReturnPerSecond * dt);
                    lookYawOffset_ *= keep;
                    lookPitchOffset_ *= keep;
                    if (std::fabs(lookYawOffset_) < 0.001f) lookYawOffset_ = 0.0f;
                    if (std::fabs(lookPitchOffset_) < 0.001f) lookPitchOffset_ = 0.0f;
                }
            }

            const float yaw = blupi_.GetYaw() + lookYawOffset_;

            Easy3D::Camera3D::Vector3 rawEye;
            Easy3D::Camera3D::Vector3 rawTarget;

            if (cameraMode_ == CameraMode::FirstPerson)
            {
                // First-person/player-view camera (2026-07-05): look from
                // Blupi's eye position in his current facing direction (see
                // GEBlupiController's GetYaw() convention: 0 rad = facing
                // -Z). Crouching lowers the eye height; looking up tilts the
                // look target upward — neither is a real head/body pose
                // (still no first-person 3D model), just a rough
                // camera-only stand-in for the animation-indicator state.
                constexpr float kEyeHeight = 0.75f;
                constexpr float kCrouchEyeHeight = 0.4f;
                constexpr float kLookDistance = 5.0f;
                constexpr float kLookUpTilt = 2.5f;
                const float eyeHeight = crouchHeld ? kCrouchEyeHeight : kEyeHeight;
                const float lookYOffset = lookUpHeld ? kLookUpTilt : 0.0f;
                rawEye = Easy3D::Camera3D::Vector3(blupi_.GetX(), blupi_.GetY() + eyeHeight, blupi_.GetZ());
                // Mouse-look pitch: positive offset (drag down) tilts the
                // view down, standard non-inverted feel. Yaw offset is
                // already folded into `yaw` above.
                rawTarget = Easy3D::Camera3D::Vector3(
                    rawEye.X + std::sin(yaw) * kLookDistance,
                    rawEye.Y + lookYOffset - std::sin(lookPitchOffset_) * kLookDistance,
                    rawEye.Z - std::cos(yaw) * kLookDistance);
            }
            else
            {
                // Third-person chase camera (2026-07-09) -- sits behind and
                // above Blupi (opposite his facing direction, same sin/-cos
                // yaw convention as the first-person target above) looking
                // slightly down at him. A fixed offset, not a real
                // spring-arm/collision-aware orbit camera -- sufficient to
                // show the placeholder model, not a final camera design.
                constexpr float kChaseDistance = 4.0f;
                constexpr float kChaseLookHeight = 1.0f;
                // Base elevation reproduces the previous fixed offset
                // (height 1.2 over distance 4); mouse-look pitch orbits the
                // camera up/down around Blupi on top of it (drag down =
                // camera rises, view tilts down -- same direction sense as
                // the first-person branch).
                constexpr float kBaseElevation = 0.29f;
                const float elevation = std::clamp(kBaseElevation + lookPitchOffset_, -1.2f, 1.45f);
                rawTarget = Easy3D::Camera3D::Vector3(
                    blupi_.GetX(), blupi_.GetY() + kChaseLookHeight, blupi_.GetZ());
                rawEye = Easy3D::Camera3D::Vector3(
                    rawTarget.X - std::sin(yaw) * kChaseDistance * std::cos(elevation),
                    rawTarget.Y + kChaseDistance * std::sin(elevation),
                    rawTarget.Z + std::cos(yaw) * kChaseDistance * std::cos(elevation));
            }

            // Exponential damping toward the raw eye/target computed above
            // (2026-07-09) -- reported live: snapping the camera straight to
            // Blupi's position/facing every frame moves too fast/feels
            // jerky, especially on turns. kCameraDampingPerSecond is the
            // fraction of the remaining distance closed per second; framerate-
            // independent via the standard `1 - exp(-rate * dt)` alpha
            // (not a plain `rate * dt` lerp, which would vary with
            // framerate). Snaps instantly on the very first frame so
            // load-time doesn't start with a lerp-in from the origin.
            constexpr float kCameraDampingPerSecond = 8.0f;
            if (!cameraSmoothedInitialized_)
            {
                cameraEyeSmoothed_ = rawEye;
                cameraTargetSmoothed_ = rawTarget;
                cameraSmoothedInitialized_ = true;
            }
            else
            {
                const float alpha = 1.0f - std::exp(-kCameraDampingPerSecond * dt);
                cameraEyeSmoothed_ = Easy3D::Camera3D::Vector3::Lerp(cameraEyeSmoothed_, rawEye, alpha);
                cameraTargetSmoothed_ = Easy3D::Camera3D::Vector3::Lerp(cameraTargetSmoothed_, rawTarget, alpha);
            }
            camera_.SetPosition(cameraEyeSmoothed_);
            camera_.SetTarget(cameraTargetSmoothed_);

            // Third-person placeholder model animation clip (2026-07-09) --
            // advanced regardless of camera mode so switching into
            // third-person mid-animation starts at a sensible position, not
            // always frame 0. Resets on a clip change since clips have very
            // different durations (Walk ~0.7s vs Survey ~3.4s) and
            // continuing from an old position could already be past a
            // shorter new clip's own end.
            if (blupiModelLoaded_)
            {
                const std::string& clipName = BlupiAnimStateToPlaceholderClipName(blupi_.GetAnimState());
                if (clipName != blupiActiveClipName_)
                {
                    blupiActiveClipName_ = clipName;
                    blupiClipTimeSeconds_ = 0.0;
                }
                blupiClipTimeSeconds_ += static_cast<double>(dt);
            }
        }
    }

    void GalaxyEggbertCnaGame::Draw(const Microsoft::Xna::Framework::GameTime& gameTime)
    {
        (void)gameTime;
        auto& device = getGraphicsDeviceProperty();
        device.Clear(0.392f, 0.584f, 0.929f, 1.0f);
        device.SetDepthTestEnabled(true);

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
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.GetViewMatrix());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            constexpr float kBackgroundDistance = 900.0f; // < camera_'s 1000.0f far plane
            constexpr float kBackgroundMargin = 1.3f;
            const float halfHeight = kBackgroundDistance *
                std::tan(camera_.GetFieldOfView() * 0.5f) * kBackgroundMargin;
            const float halfWidth = halfHeight * camera_.GetAspectRatio();

            const auto& camPos = camera_.GetPosition();
            const auto& camTarget = camera_.GetTarget();
            Microsoft::Xna::Framework::Vector3 forward(
                camTarget.X - camPos.X, camTarget.Y - camPos.Y, camTarget.Z - camPos.Z);
            forward.Normalize();
            const Microsoft::Xna::Framework::Vector3 quadCenter(
                camPos.X + forward.X * kBackgroundDistance,
                camPos.Y + forward.Y * kBackgroundDistance,
                camPos.Z + forward.Z * kBackgroundDistance);

            Easy3D::BillboardBatch batch;
            batch.Add(quadCenter,
                      Microsoft::Xna::Framework::Vector2(halfWidth * 2.0f, halfHeight * 2.0f),
                      Easy3D::UvRect{0.0f, 0.0f, 1.0f, 1.0f});

            std::vector<Easy3D::BillboardVertex> vertices;
            std::vector<std::uint32_t> indices;
            Easy3D::BuildBillboardMesh(batch, cameraRight, cameraUp, vertices, indices);

            if (!indices.empty())
            {
                backgroundMeshRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
                backgroundEffect_->View = camera_.GetViewMatrix();
                backgroundEffect_->Projection = camera_.GetProjectionMatrix();
                backgroundEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                // No CullNone override needed here anymore -- fixed at the
                // source (2026-07-09, NEXT.md §8 task 0): Easy3D::
                // AppendBillboardMesh's winding was back-facing under this
                // engine's default CullCounterClockwise state; reversed
                // there (../easy-3d/src/BillboardMesh.cpp) so every
                // billboard consumer, not just this one, renders correctly
                // under the default cull state.
                backgroundMeshRenderer_->Draw(device, *backgroundEffect_);
            }
        }

        if (terrainRenderer_ && terrainEffect_)
        {
            terrainEffect_->View = camera_.GetViewMatrix();
            terrainEffect_->Projection = camera_.GetProjectionMatrix();
            terrainEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            terrainRenderer_->Draw(device, *terrainEffect_);

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
                // CubeMeshRenderer has no in-place update API (only
                // construction from vertex/index data), so a fresh mesh is
                // the only way to reflect a new icon/UV per frame -- cheap
                // here since there are only a handful of cube objects.
                constexpr float kObjectCubeGroundOffset = 1.0f; // matches kObjectGroundOffset below
                std::vector<Easy3D::CubeVertex> cubeVertices;
                std::vector<std::uint32_t> cubeIndices;
                for (const auto& obj : worldRuntime_.GetMobileObjects())
                {
                    if (!IsUniformCubeObject(obj.type) || !obj.active)
                    {
                        continue;
                    }
                    const int icon = GetObjIcon(obj.type, static_cast<int>(obj.phase));
                    Easy3D::CubeItem item;
                    item.Center = Easy3D::CubeBatch::Vector3(
                        obj.currentX, obj.currentY + kObjectCubeGroundOffset, obj.currentZ);
                    item.Size = Easy3D::CubeBatch::Vector3(1.0f, 1.0f, 1.0f);
                    item.Uv = tileAtlas_.GetTileUv(icon);
                    Easy3D::AppendCubeMesh(item, cubeVertices, cubeIndices);
                }

                if (!cubeIndices.empty())
                {
                    objectCubeMeshRenderer_ = std::make_unique<Easy3D::CubeMeshRenderer>(device, cubeVertices, cubeIndices);
                    objectCubeEffect_->View = camera_.GetViewMatrix();
                    objectCubeEffect_->Projection = camera_.GetProjectionMatrix();
                    objectCubeEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                    objectCubeMeshRenderer_->Draw(device, *objectCubeEffect_);
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
                grassEffect_->View = camera_.GetViewMatrix();
                grassEffect_->Projection = camera_.GetProjectionMatrix();
                grassEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                terrainRenderer_->DrawGrass(device, *grassEffect_);
            }

            // Third-person placeholder 3D model (2026-07-09, NEXT.md §3) --
            // only drawn in third-person mode (first-person mode has
            // nothing to show, same reasoning as the pre-2026-07-09 comment
            // this replaced). World transform bakes in a coarse scale (the
            // placeholder fox mesh is ~79 units tall in its own space,
            // scaled down to roughly Blupi's ~1.6-unit eye-height scale --
            // see avatars3d/blupi_placeholder/README.md; not verified against
            // a real Blupi model yet, since none exists) and a yaw rotation
            // to face Blupi's movement direction -- exact forward-facing
            // alignment for this specific placeholder asset hasn't been
            // visually verified, a real Blupi model may need a different
            // constant rotation offset here.
            if (cameraMode_ == CameraMode::ThirdPersonModel && blupiModelLoaded_ && blupiAvatarRenderer_)
            {
                constexpr float kPlaceholderModelScale = 0.02f;
                const auto world =
                    Microsoft::Xna::Framework::Matrix::CreateScale(kPlaceholderModelScale) *
                    Microsoft::Xna::Framework::Matrix::CreateRotationY(blupi_.GetYaw()) *
                    Microsoft::Xna::Framework::Matrix::CreateTranslation(
                        blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
                blupiAvatarRenderer_->setWorldProperty(world);
                blupiAvatarRenderer_->setViewProperty(camera_.GetViewMatrix());
                blupiAvatarRenderer_->setProjectionProperty(camera_.GetProjectionMatrix());
                blupiAvatarRenderer_->DrawRealEXT(
                    blupiActiveClipName_,
                    System::TimeSpan::FromSeconds(blupiClipTimeSeconds_),
                    /*loop=*/true);
            }

            static bool terrainPixelPrinted = false;
            if (!terrainPixelPrinted)
            {
                terrainPixelPrinted = true;
                const auto& viewport = device.getViewportProperty();
                const int w = viewport.getWidthProperty();
                const int h = viewport.getHeightProperty();

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
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::AlphaBlend);

        // Billboard rendering for worldRuntime_'s parsed MoveObjects
        // (15-3d-render-mapping-design.md §5/§7, first pass 2026-07-06) —
        // now animated via each MobileObjSpec's real per-instance phase
        // (2026-07-09, GEWorldRuntime::Update()), element.png only (see
        // GEObjectIcons.hpp's known-limitation note re: DOC-007). Rebuilt
        // every frame since billboard vertex positions depend on the camera
        // (Easy3D::BillboardMeshRenderer's header comment) -- convenient,
        // since it also means the icon lookup naturally re-runs every frame
        // with the latest phase.
        if (objectEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            // Camera-facing basis: the inverse view matrix's Right/Up rows
            // give the camera's world-space right/up vectors (standard
            // spherical-billboard technique).
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.GetViewMatrix());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            Easy3D::BillboardBatch batch;
            constexpr float kObjectSize = 1.0f;
            constexpr float kObjectGroundOffset = 1.0f; // matches blupi_'s own ground-standing height
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
                // not theirs). IsBlupiPngSourcedAtPhase (not the plain,
                // phase-blind IsBlupiPngSourced) since ObjectType38 spends
                // part of its cycle on element.png -- this loop must pick it
                // up during that window, not skip it forever.
                const int objPhase = static_cast<int>(obj.phase);
                if (!obj.active || IsUniformCubeObject(obj.type) || IsObjectMPngSourced(obj.type) ||
                    IsExploPngSourced(obj.type) || IsBlupiPngSourcedAtPhase(obj.type, objPhase))
                {
                    continue;
                }
                const int icon = GetObjIcon(obj.type, objPhase);
                const auto uv = GetElementIconUv(icon);
                batch.Add(
                    Microsoft::Xna::Framework::Vector3(obj.currentX, obj.currentY + kObjectGroundOffset, obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize),
                    Easy3D::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }

            std::vector<Easy3D::BillboardVertex> vertices;
            std::vector<std::uint32_t> indices;
            Easy3D::BuildBillboardMesh(batch, cameraRight, cameraUp, vertices, indices);

            if (!indices.empty())
            {
                objectMeshRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
                objectEffect_->View = camera_.GetViewMatrix();
                objectEffect_->Projection = camera_.GetProjectionMatrix();
                objectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                objectMeshRenderer_->Draw(device, *objectEffect_);
            }
        }

        // Billboard rendering for the 5 object-m.png-sourced MoveObjects
        // (GEObjectIcons::IsObjectMPngSourced, NEXT.md §3, 2026-07-09) --
        // same camera-facing billboard technique as the element.png batch
        // above, but reuses terrainTexture_ (object-m.png) via
        // objectMPngEffect_, and looks up UVs through tileAtlas_ instead of
        // GetElementIconUv() (same icon-index domain as terrain/BigDecor).
        if (objectMPngEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.GetViewMatrix());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            Easy3D::BillboardBatch batch;
            constexpr float kObjectSize = 1.0f;
            constexpr float kObjectGroundOffset = 1.0f; // matches the element.png batch above
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                if (!obj.active || !IsObjectMPngSourced(obj.type))
                {
                    continue;
                }
                const int icon = GetObjIcon(obj.type, static_cast<int>(obj.phase));
                const auto uv = tileAtlas_.GetTileUv(icon);
                batch.Add(
                    Microsoft::Xna::Framework::Vector3(obj.currentX, obj.currentY + kObjectGroundOffset, obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize),
                    uv);
            }

            std::vector<Easy3D::BillboardVertex> vertices;
            std::vector<std::uint32_t> indices;
            Easy3D::BuildBillboardMesh(batch, cameraRight, cameraUp, vertices, indices);

            if (!indices.empty())
            {
                objectMPngMeshRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
                objectMPngEffect_->View = camera_.GetViewMatrix();
                objectMPngEffect_->Projection = camera_.GetProjectionMatrix();
                objectMPngEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                objectMPngMeshRenderer_->Draw(device, *objectMPngEffect_);
            }
        }

        // Billboard rendering for the 12 explo.png-sourced MoveObjects
        // (GEObjectIcons::IsExploPngSourced, NEXT.md §3, 2026-07-09) --
        // same camera-facing billboard technique as the batches above, but
        // exploTexture_ (a genuinely new texture) via exploEffect_, and
        // GetExploIconUv() instead of GetElementIconUv()/tileAtlas_.
        if (exploEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.GetViewMatrix());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            Easy3D::BillboardBatch batch;
            constexpr float kObjectSize = 1.0f;
            constexpr float kObjectGroundOffset = 1.0f; // matches the batches above
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                if (!obj.active || !IsExploPngSourced(obj.type))
                {
                    continue;
                }
                const int icon = GetObjIcon(obj.type, static_cast<int>(obj.phase));
                const auto uv = GetExploIconUv(icon);
                batch.Add(
                    Microsoft::Xna::Framework::Vector3(obj.currentX, obj.currentY + kObjectGroundOffset, obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize),
                    Easy3D::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }

            std::vector<Easy3D::BillboardVertex> vertices;
            std::vector<std::uint32_t> indices;
            Easy3D::BuildBillboardMesh(batch, cameraRight, cameraUp, vertices, indices);

            if (!indices.empty())
            {
                exploMeshRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
                exploEffect_->View = camera_.GetViewMatrix();
                exploEffect_->Projection = camera_.GetProjectionMatrix();
                exploEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                exploMeshRenderer_->Draw(device, *exploEffect_);
            }
        }

        // Billboard rendering for the 4 Blupi-skin MoveObjects
        // (GEObjectIcons::IsBlupiPngSourced, NEXT.md §3, 2026-07-09) --
        // ObjectType200 via blupiObjectEffect_ (blupi.png), ObjectType201/
        // 202/203 via blupi1ObjectEffect_ (blupi1.png,
        // GEObjectIcons::UsesBlupi1Texture) -- two separate batches since
        // BasicEffect only binds one texture at a time.
        if (blupiObjectEffect_ && blupi1ObjectEffect_ && !worldRuntime_.GetMobileObjects().empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.GetViewMatrix());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            Easy3D::BillboardBatch blupiBatch;
            Easy3D::BillboardBatch blupi1Batch;
            constexpr float kObjectSize = 1.0f;
            constexpr float kObjectGroundOffset = 1.0f; // matches the batches above
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                const int objPhase = static_cast<int>(obj.phase);
                if (!obj.active || !IsBlupiPngSourcedAtPhase(obj.type, objPhase))
                {
                    continue;
                }
                const int icon = GetObjIcon(obj.type, objPhase);
                const auto uv = GetBlupiIconUv(icon);
                auto& batch = UsesBlupi1Texture(obj.type) ? blupi1Batch : blupiBatch;
                batch.Add(
                    Microsoft::Xna::Framework::Vector3(obj.currentX, obj.currentY + kObjectGroundOffset, obj.currentZ),
                    Microsoft::Xna::Framework::Vector2(kObjectSize, kObjectSize),
                    Easy3D::UvRect{uv.U0, uv.V0, uv.U1, uv.V1});
            }

            std::vector<Easy3D::BillboardVertex> blupiVertices;
            std::vector<std::uint32_t> blupiIndices;
            Easy3D::BuildBillboardMesh(blupiBatch, cameraRight, cameraUp, blupiVertices, blupiIndices);
            if (!blupiIndices.empty())
            {
                blupiObjectMeshRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, blupiVertices, blupiIndices);
                blupiObjectEffect_->View = camera_.GetViewMatrix();
                blupiObjectEffect_->Projection = camera_.GetProjectionMatrix();
                blupiObjectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                blupiObjectMeshRenderer_->Draw(device, *blupiObjectEffect_);
            }

            std::vector<Easy3D::BillboardVertex> blupi1Vertices;
            std::vector<std::uint32_t> blupi1Indices;
            Easy3D::BuildBillboardMesh(blupi1Batch, cameraRight, cameraUp, blupi1Vertices, blupi1Indices);
            if (!blupi1Indices.empty())
            {
                blupi1ObjectMeshRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, blupi1Vertices, blupi1Indices);
                blupi1ObjectEffect_->View = camera_.GetViewMatrix();
                blupi1ObjectEffect_->Projection = camera_.GetProjectionMatrix();
                blupi1ObjectEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                blupi1ObjectMeshRenderer_->Draw(device, *blupi1ObjectEffect_);
            }
        }

        // Billboard rendering for worldRuntime_'s parsed BigDecor: cells
        // (NEXT.md §8 task 3) — same camera-facing billboard technique as
        // MoveObjects above, but object-m.png (via tileAtlas_) instead of
        // element.png, since BigDecor shares the main terrain grid's icon
        // vocabulary. Only ever non-empty when a world was loaded via
        // LoadFromMobileEggbertFile() — the default .vwr world has no
        // BigDecor concept, so this is a no-op in that case.
        if (bigDecorEffect_ && !bigDecorCells_.empty())
        {
            const auto invView = Microsoft::Xna::Framework::Matrix::Invert(camera_.GetViewMatrix());
            const auto cameraRight = invView.getRightProperty();
            const auto cameraUp = invView.getUpProperty();

            Easy3D::BillboardBatch batch;
            constexpr float kBigDecorSize = 1.0f;
            constexpr float kBigDecorGroundOffset = 1.0f; // matches kObjectGroundOffset above
            for (const auto& cell : bigDecorCells_)
            {
                const auto uv = tileAtlas_.GetTileUv(static_cast<int>(cell.icon));
                batch.Add(
                    Microsoft::Xna::Framework::Vector3(cell.worldX, kBigDecorGroundOffset, cell.worldZ),
                    Microsoft::Xna::Framework::Vector2(kBigDecorSize, kBigDecorSize),
                    uv);
            }

            std::vector<Easy3D::BillboardVertex> vertices;
            std::vector<std::uint32_t> indices;
            Easy3D::BuildBillboardMesh(batch, cameraRight, cameraUp, vertices, indices);

            if (!indices.empty())
            {
                bigDecorMeshRenderer_ = std::make_unique<Easy3D::BillboardMeshRenderer>(device, vertices, indices);
                bigDecorEffect_->View = camera_.GetViewMatrix();
                bigDecorEffect_->Projection = camera_.GetProjectionMatrix();
                bigDecorEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                bigDecorMeshRenderer_->Draw(device, *bigDecorEffect_);
            }
        }

        // Restore opaque state after the AlphaBlend block above (billboards
        // only) -- GEHud manages its own blend state, but this keeps device
        // state predictable for anything drawn after this point.
        device.setBlendStateProperty(Microsoft::Xna::Framework::Graphics::BlendState::Opaque);

        // HUD quads must not be depth-tested against the 3D scene (Draw()
        // re-enables depth at the top of every frame regardless).
        device.SetDepthTestEnabled(false);

        // Real mobile-eggbert bottom HUD + interim animation-state
        // indicator (2026-07-10, see GEHud.hpp for the full layout AND for
        // why this is real 3D quads instead of SpriteBatch -- CNA's Vulkan
        // backend records every SpriteBatch batch before every 3D draw, so
        // a sprite HUD is always painted over by the scene; that was the
        // real cause of the twice-reported "icon visible for a second,
        // then gone" bug, which the earlier depth-test fix addressed only
        // for EasyGL). Drawn LAST so it wins submission order on both
        // backends.
        {
            const auto& viewport = device.getViewportProperty();
            hud_.Draw(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                      interaction_.Lives(),
                      interaction_.Key1Count() > 0, interaction_.Key2Count() > 0,
                      interaction_.Key3Count() > 0,
                      interaction_.TreasuresCollected(), interaction_.TotalTreasures(),
                      blupi_.GetAnimIcon());
        }

        // One-shot full-frame screenshot, taken here (2026-07-11) rather
        // than reusing the terrain-only screenshot.png above (captured
        // right after the opaque terrain pass, before billboards/HUD) --
        // that one is a deliberately earlier terrain-geometry diagnostic,
        // not meant to also cover the 2D HUD added above. This one runs
        // after every draw call in the frame, including the new HUD, so it
        // can be inspected for visual HUD verification the same way
        // screenshot.png already is for terrain.
        static bool hudScreenshotWritten = false;
        if (!hudScreenshotWritten)
        {
            hudScreenshotWritten = true;
            const auto& viewport = device.getViewportProperty();
            const int w = viewport.getWidthProperty();
            const int h = viewport.getHeightProperty();
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
    }

    GetTypeNameCPP(GalaxyEggbertCnaGame, "GalaxyEggbertCnaGame")
}
