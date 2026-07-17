#include "GalaxyEggbertCnaGame.hpp"

#include "GalaxyEggbert/BlockTypes.hpp"
#include "GalaxyEggbert/Worlds/Block.hpp"

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
        // correspond to GEBlupiController::AnimState's states at all (10 as
        // of 2026-07-11's Jump/Air split plus Ecrase/Balloon/Teleporting,
        // plan.md E3D-MIG-064), so this is a rough best-effort substitution,
        // not a faithful behavioral mapping -- see
        // avatars3d/blupi_placeholder/README.md's own mapping table for the
        // reasoning per state. Swap out entirely once a real Blupi model
        // with real matching clips exists.
        const std::string& BlupiAnimStateToPlaceholderClipName(GEBlupiController::AnimState state)
        {
            static const std::string kSurvey = "Survey";
            static const std::string kWalk = "Walk";
            static const std::string kRun = "Run";
            switch (state)
            {
                case GEBlupiController::AnimState::March:
                case GEBlupiController::AnimState::MarchEcrase: return kWalk;
                case GEBlupiController::AnimState::Jump:
                case GEBlupiController::AnimState::Air:   return kRun;
                case GEBlupiController::AnimState::Stop:
                case GEBlupiController::AnimState::Down:
                case GEBlupiController::AnimState::Up:
                case GEBlupiController::AnimState::StopEcrase:
                case GEBlupiController::AnimState::Balloon:
                case GEBlupiController::AnimState::Teleporting:
                default:
                    return kSurvey;
            }
        }

        // Real fixed 5.0s Wait-phase cosmetic timer (plan.md
        // MENU-001..005, confirmed via research: `Game1.cpp`'s real
        // `waitProgress = ticks/50,000,000`) -- must match
        // GEInputPad::DrawWait()'s own gauge-fill duration (kept as a
        // separately-owned constant there since that class doesn't
        // depend on this one).
        constexpr float kWaitDurationSeconds = 5.0f;

        // Real fade-out phase-transition commit timer (plan.md
        // MENU-088/089, confirmed via research: `Config::ScaleTime(20)` =
        // 1.0s at this build's pinned 20fps) -- must match
        // GEInputPad.cpp's own kFadeDurationSeconds (kept as a
        // separately-owned constant there, same cross-file convention as
        // kWaitDurationSeconds above).
        constexpr float kFadeCommitDurationSeconds = 1.0f;

        // Third-person chase-camera wall collision (plan.md CAM-005, found
        // 2026-07-17): keeps the camera from clipping through solid
        // geometry to end up outside a tunnel/room looking back in through
        // a wall. A simplified fixed-step raymarch from the look-at point
        // toward the desired eye position, stopping at the first solid
        // block -- not a literal cell-boundary DDA voxel traversal (the
        // task's own original wording), since the real max distance
        // involved is tiny (a few world units, the chase distance) and a
        // small fixed step is both simpler and plenty precise at that
        // scale; same "documented simplification, not the textbook
        // algorithm" category as several other collision adaptations this
        // session. Solidity uses a plain `!isAir()` check (this is a
        // visual-occlusion query, not Blupi's own movement-collision rule
        // set -- e.g. a real non-solid-for-Blupi Teleporter pillar still
        // visually blocks the camera's view through it, which is the
        // correct behavior here even though it would be wrong for
        // GEBlupiController's own `IsSolidAt`). Returns the safe distance
        // along `direction` (a unit vector), always `<= maxDistance`.
        float RaymarchWallDistance(const GalaxyEggbert::Worlds::World& world,
                                   const Easy3D::Camera3D::Vector3& origin,
                                   const Easy3D::Camera3D::Vector3& direction, float maxDistance)
        {
            constexpr float kStep = 0.1f;
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            float traveled = 0.0f;
            while (traveled < maxDistance)
            {
                traveled = std::min(traveled + kStep, maxDistance);
                const auto point = origin + direction * traveled;
                const int gx = static_cast<int>(std::lround(point.X)) + GEWorldRuntime::kWorldCenterX;
                const int gy = static_cast<int>(std::lround(point.Y));
                const int gz = static_cast<int>(std::lround(point.Z)) + GEWorldRuntime::kWorldCenterZ;
                if (gx < 0 || gz < 0 || gy < 0 || gx >= blocksPerAxis || gz >= blocksPerAxis || gy >= blocksPerAxis)
                {
                    continue; // out of bounds -- treat as open air
                }
                if (!world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                     static_cast<std::uint16_t>(gz))
                         .isAir())
                {
                    return std::max(0.0f, traveled - kStep);
                }
            }
            return maxDistance;
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
        RebuildWorldPresentation();

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

        // Real mobile-eggbert on-screen touch controls (2026-07-13, see
        // GEInputPad.hpp -- loads its own pad.png instance plus
        // Content/backgrounds/pause.png and blupiyoupie.png, which
        // nothing else loads).
        inputPad_.LoadContent(device);

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

        // Minimal settings persistence (2026-07-13, plan.md MENU-067, see
        // GESaveData.hpp) -- restores the one real, already-wired setting
        // (sound on/off) from the previous run, if any.
        saveData_.Load();
        sound_.SetEnabled(saveData_.GetSoundEnabled());

        // Real Wait->Resume-or-Init branch (plan.md MENU-001..020) now
        // decides this at the END of the real 5.0s Wait timer, in
        // Update(), not here at load time -- see phase_'s own default
        // member initializer (now Wait, not Play) and GamePhase's own
        // class comment for the full reasoning.

        std::cout << "GalaxyEggbertCNA: terrain mesh uploaded — "
                  << terrainRenderer_->BlockCount() << " blocks ("
                  << terrainRenderer_->AnimatedBlockCount() << " animated, "
                  << terrainRenderer_->WaterBlockCount() << " water/semi-transparent), "
                  << terrainRenderer_->VertexCount() << " vertices, "
                  << terrainRenderer_->PrimitiveCount() << " triangles." << std::endl;
    }

    void GalaxyEggbertCnaGame::RebuildWorldPresentation()
    {
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
                backgroundLoaded_ = false;
                std::cout << "GalaxyEggbertCNA: no background image for region "
                          << worldRuntime_.GetSkyRegion() << " (" << backgroundPath
                          << " not found) — falling back to flat clear color." << std::endl;
            }
        }

        terrainRenderer_ = std::make_unique<GETerrainRenderer>(device, worldRuntime_.GetWorld(), tileAtlas_);
    }

    void GalaxyEggbertCnaGame::LoadMission(int missionNumber)
    {
        char path[64];
        std::snprintf(path, sizeof(path), "worlds3d/world%03d.vwr", missionNumber);
        if (!worldRuntime_.LoadFromVwrFile(path))
        {
            std::cout << "GalaxyEggbertCNA: LoadMission(" << missionNumber << "): " << path
                       << " not found or failed to load — staying on the current mission." << std::endl;
            return;
        }

        RebuildWorldPresentation();

        // Real AdaptDoors() hub-level door gate (plan.md hub/mission-
        // progression system, found 2026-07-17, `Decor.cpp:11588-11621`):
        // applied at LOAD time, before the level is ever shown -- matches
        // real source exactly (`AdaptDoors()` runs inside `StartMission()`,
        // never live while already playing). Scans the freshly-loaded
        // world for any `ProgressDoor2..8` tile; each one gates sublevel-
        // select marker N in THIS hub (mission `missionNumber+N`), opened
        // once the PRECEDING sublevel has been won (`saveData_::
        // IsMissionDoorUnlocked(missionNumber+N)`, set by the win-exit
        // handler below -- mirrors real `m_doors[mission+1]=1`). No
        // animated slide-open needed here (the real one only plays live,
        // the instant a door's condition becomes newly true while
        // standing right there -- this unlock always happened in an
        // earlier visit/session, so the door is simply already-open by
        // the time this level is shown, matching the real `AdaptDoors()`
        // pre-load timing exactly).
        {
            auto& terrain = worldRuntime_.GetWorldMutable();
            const auto axis = terrain.blocksPerAxis();
            for (std::uint16_t x = 0; x < axis; ++x)
            {
                for (std::uint16_t y = 0; y < axis; ++y)
                {
                    for (std::uint16_t z = 0; z < axis; ++z)
                    {
                        const auto block = terrain.getBlock(x, y, z).type();
                        if (!GalaxyEggbert::BlockTypes::isProgressDoor(block))
                        {
                            continue;
                        }
                        const int doorIndex = GalaxyEggbert::BlockTypes::progressDoorIndex(block);
                        const int targetMission = missionNumber + doorIndex;
                        if (saveData_.IsMissionDoorUnlocked(targetMission))
                        {
                            terrain.setBlock(x, y, z, GalaxyEggbert::Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));
                        }
                    }
                }
            }
        }

        // Real PlayPrepare() (Decor.cpp:381-429): every mission load resets
        // vehicle mode, all 4 secret powers, Invert, Ghost, keys, dynamite
        // count, and recounts this world's own treasures from scratch --
        // ONLY lives (m_nbVies) survive. `interaction_`/`blupi_` are plain
        // value members (not pointers), so reassigning fresh defaults is a
        // real reset, not a partial one -- same idiom already used
        // elsewhere in this class for "start over" semantics.
        const int preservedLives = interaction_.Lives();
        interaction_ = GEInteractionSystem();
        interaction_.SetLives(preservedLives);
        blupi_ = GEBlupiController();

        // Same fixed spawn convention every hand-authored .vwr world in
        // this engine shares (see LoadContent()'s own comment) -- the
        // format itself still carries no real per-world spawn point.
        blupi_.SetPosition(0.0f, 1.0f, 0.0f);

        saveData_.SetMissionNumber(missionNumber);
        saveData_.Save();

        std::cout << "GalaxyEggbertCNA: LoadMission(" << missionNumber << "): loaded " << path << "."
                   << std::endl;
    }

    void GalaxyEggbertCnaGame::SetPhase(GalaxyEggbert::GamePhase next, bool bypassFade) noexcept
    {
        // Real Game1::SetPhase() (2026-07-13, plan.md MENU-088/089, see
        // this method's own declaration comment for the full real
        // deferred-transition mechanic): only defers when the CURRENT
        // phase is one of the 5 real deferring phases, no fade is already
        // pending, and bypassFade wasn't requested (the real `mission==
        // -2` sentinel). Every other phase commits instantly, matching
        // the real source exactly (confirmed via research: Play->Pause,
        // Play->Win/Lost, and Resume->Play-via-Continue are ALL genuinely
        // instant in the real game, not merely fast).
        const bool sourceDefers = phase_ == GalaxyEggbert::GamePhase::Init ||
                                   phase_ == GalaxyEggbert::GamePhase::MainSetup ||
                                   phase_ == GalaxyEggbert::GamePhase::PlaySetup ||
                                   phase_ == GalaxyEggbert::GamePhase::Pause ||
                                   phase_ == GalaxyEggbert::GamePhase::Resume;
        if (sourceDefers && fadeOutPhase_ == GalaxyEggbert::GamePhase::None && !bypassFade)
        {
            fadeOutPhase_ = next;
            phaseTimeSeconds_ = 0.0f;
            return;
        }

        // Real Game1::SetPhase() resets a per-phase timer (real `phaseTime`,
        // now ported as phaseTimeSeconds_ -- see its own member comment,
        // 2026-07-13) and clears input debounce state on every transition
        // (Game1.cpp:979-1058) -- this engine's own equivalent is just the
        // 2 key-debounce trackers below.
        phase_ = next;
        fadeOutPhase_ = GalaxyEggbert::GamePhase::None;
        pauseKeyWasDown_ = false;
        phaseReturnKeyWasDown_ = false;
        phaseTimeSeconds_ = 0.0f;
        inputPad_.ResetTouchState();
    }

    const char* GalaxyEggbertCnaGame::PhaseOverlayMessage() const noexcept
    {
        switch (phase_)
        {
            case GalaxyEggbert::GamePhase::Pause:
                // Handled by inputPad_.DrawPause() instead (2026-07-13,
                // plan.md MENU-028..039) -- the real background/character/
                // buttons, not a generic text overlay.
                return nullptr;
            case GalaxyEggbert::GamePhase::Win:
            case GalaxyEggbert::GamePhase::Lost:
                // Handled by inputPad_.DrawWinLost() instead (2026-07-13,
                // plan.md MENU-046..057) -- the real win.png/lost.png
                // background + blupiyoupie.png animation, not a generic
                // text overlay.
                return nullptr;
            default:
                return nullptr;
        }
    }

    void GalaxyEggbertCnaGame::ResolvePendingVoyage()
    {
        if (!interaction_.VoyagePendingThisFrame())
        {
            return;
        }
        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        const int viewportW = viewport.getWidthProperty();
        const int viewportH = viewport.getHeightProperty();

        float projectedX = 0.0f, projectedY = 0.0f;
        const bool worldIsStart = interaction_.VoyagePendingWorldIsStart();
        const bool projected = GEHud::ProjectWorldToHudSpace(
            Microsoft::Xna::Framework::Vector3(interaction_.VoyagePendingWorldX(), interaction_.VoyagePendingWorldY(),
                                                interaction_.VoyagePendingWorldZ()),
            camera_.GetViewMatrix(), camera_.GetProjectionMatrix(), viewportW, viewportH, projectedX, projectedY);
        // Clear2Ascend (real death-VFX coinflip, plan.md `158` follow-up):
        // BOTH endpoints derive from the SAME projected world point (start
        // = Blupi's projected position, end = straight up from there by a
        // fixed HUD-space offset) -- there is no independent "fixed" point
        // to fall back to if projection fails, so an un-projectable
        // ascend request is simply dropped (a dying Blupi behind the
        // camera is not a real scenario this needs to handle robustly).
        if (interaction_.VoyagePendingIsAscend())
        {
            if (projected)
            {
                interaction_.BeginVoyage(worldRuntime_, interaction_.VoyagePendingKind(),
                                         interaction_.VoyagePendingIconId(), false, projectedX, projectedY, projectedX,
                                         projectedY - interaction_.VoyagePendingAscendOffsetY(), sound_,
                                         interaction_.VoyagePendingWorldX(), interaction_.VoyagePendingWorldY(),
                                         interaction_.VoyagePendingWorldZ());
            }
            return;
        }

        if (!projected)
        {
            // Behind the camera (shouldn't happen for a just-touched
            // pickup) -- fall back to collapsing this endpoint onto the
            // OTHER (fixed) one, giving a real total=0 (the reward applies
            // on the very next tick) rather than leaving the voyage stuck.
            projectedX = interaction_.VoyagePendingFixedX();
            projectedY = interaction_.VoyagePendingFixedY();
        }

        const float startX = worldIsStart ? projectedX : interaction_.VoyagePendingFixedX();
        const float startY = worldIsStart ? projectedY : interaction_.VoyagePendingFixedY();
        const float endX = worldIsStart ? interaction_.VoyagePendingFixedX() : projectedX;
        const float endY = worldIsStart ? interaction_.VoyagePendingFixedY() : projectedY;
        interaction_.BeginVoyage(worldRuntime_, interaction_.VoyagePendingKind(), interaction_.VoyagePendingIconId(),
                                 interaction_.VoyagePendingIsButtonChannel(), startX, startY, endX, endY, sound_);
    }

    void GalaxyEggbertCnaGame::DismountAndDepositVehicle()
    {
        if (!blupi_.IsInVehicle())
        {
            return;
        }
        GalaxyEggbert::ObjectType depositType;
        switch (blupi_.GetVehicleMode())
        {
            case GEBlupiController::VehicleMode::Helicopter: depositType = GalaxyEggbert::ObjectType::ObjectType13; break;
            case GEBlupiController::VehicleMode::Jeep:        depositType = GalaxyEggbert::ObjectType::ObjectType19; break;
            case GEBlupiController::VehicleMode::Tank:        depositType = GalaxyEggbert::ObjectType::ObjectType28; break;
            case GEBlupiController::VehicleMode::Skateboard:  depositType = GalaxyEggbert::ObjectType::ObjectType24; break;
            default:                                          depositType = GalaxyEggbert::ObjectType::ObjectType46; break;
        }
        blupi_.TriggerDismount();
        auto& objects = worldRuntime_.GetMobileObjectsMutable();
        MobileObjSpec deposited;
        deposited.type = depositType;
        deposited.active = true;
        deposited.posStartX = deposited.posEndX = deposited.currentX = blupi_.GetX();
        deposited.posStartY = deposited.posEndY = deposited.currentY = blupi_.GetY();
        deposited.posStartZ = deposited.posEndZ = deposited.currentZ = blupi_.GetZ();
        bool placed = false;
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = deposited;
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            objects.push_back(deposited);
        }
    }

    void GalaxyEggbertCnaGame::UpdateVehicleMotorSound()
    {
        using GalaxyEggbert::SoundChannel;

        SoundChannel desiredLoop = SoundChannel::SoundChannel0;
        SoundChannel startSound = SoundChannel::SoundChannel0;
        SoundChannel stopSound = SoundChannel::SoundChannel0;
        switch (blupi_.GetVehicleMode())
        {
            case GEBlupiController::VehicleMode::Helicopter:
                desiredLoop = blupi_.IsVehicleMotorHigh() ? SoundChannel::SoundChannel16 : SoundChannel::SoundChannel18;
                startSound = SoundChannel::SoundChannel15;
                stopSound = SoundChannel::SoundChannel17;
                break;
            case GEBlupiController::VehicleMode::Jeep:
            case GEBlupiController::VehicleMode::Tank:
            case GEBlupiController::VehicleMode::Overcraft:
                desiredLoop = blupi_.IsVehicleMotorHigh() ? SoundChannel::SoundChannel29 : SoundChannel::SoundChannel31;
                startSound = SoundChannel::SoundChannel28;
                stopSound = SoundChannel::SoundChannel30;
                break;
            default:
                break; // None/Skateboard: no motor sound in real source either
        }

        if (activeMotorLoop_ == desiredLoop)
        {
            return;
        }
        if (activeMotorLoop_ == SoundChannel::SoundChannel0 && desiredLoop != SoundChannel::SoundChannel0)
        {
            sound_.Play(startSound);
        }
        if (activeMotorLoop_ != SoundChannel::SoundChannel0 && desiredLoop == SoundChannel::SoundChannel0)
        {
            sound_.Play(stopSound);
        }
        if (activeMotorLoop_ != SoundChannel::SoundChannel0)
        {
            sound_.Stop(activeMotorLoop_);
        }
        activeMotorLoop_ = desiredLoop;
        if (activeMotorLoop_ != SoundChannel::SoundChannel0)
        {
            sound_.Play(activeMotorLoop_, /*loop=*/true);
        }
    }

    void GalaxyEggbertCnaGame::ResolveDeathLock()
    {
        // Starts a NEW lock for the 4 real trigger sites living inside
        // interaction_.Update() itself (see GEInteractionSystem::
        // DeathLockRequestedThisFrame()'s own comment for the exact
        // shouldRespawn/PendingDeathKind->DeathCause mapping).
        if (interaction_.DeathLockRequestedThisFrame())
        {
            using GalaxyEggbert::CNA::GEBlupiController;
            using GalaxyEggbert::CNA::GEInteractionSystem;
            const GEInteractionSystem::PendingDeathKind pendingKind = interaction_.DeathLockPendingKind();
            const GEBlupiController::DeathCause cause = (pendingKind == GEInteractionSystem::PendingDeathKind::Clear1)
                                                             ? GEBlupiController::DeathCause::Clear1
                                                         : (pendingKind == GEInteractionSystem::PendingDeathKind::Clear2)
                                                             ? GEBlupiController::DeathCause::Clear2
                                                             : GEBlupiController::DeathCause::Glu;
            blupi_.TriggerDeathLock(cause, interaction_.DeathLockShouldRespawn());
        }

        // Resolves an ALREADY-active lock (possibly started a prior frame)
        // that just elapsed -- see TriggerDeathLock()'s own comment for the
        // real mechanic (respawn, then the life-loss Voyage; or game over
        // with no Voyage at all if this was Blupi's last life).
        bool shouldRespawn = false;
        if (!blupi_.ConsumeDeathLockResolved(shouldRespawn))
        {
            return;
        }
        if (shouldRespawn)
        {
            blupi_.SetPosition(blupi_.GetValidX(), blupi_.GetValidY(), blupi_.GetValidZ());
        }
        // Real `if (m_nbVies > 0) { ...Voyage... } else { ...DoorsLost(), no
        // Voyage... }` (Decor.cpp:6382-6392) -- predict which branch BEFORE
        // calling anything that decrements, since LoseLife() itself is what
        // performs the decrement (inside BeginVoyage()'s own LifeLoss case
        // below, matching real VoyageInit's inline `m_nbVies--`).
        if (interaction_.Lives() <= 1)
        {
            interaction_.LoseLife();
            return;
        }

        const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        float endX = 0.0f, endY = 0.0f;
        const bool projected = GEHud::ProjectWorldToHudSpace(
            Microsoft::Xna::Framework::Vector3(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ()), camera_.GetViewMatrix(),
            camera_.GetProjectionMatrix(), viewport.getWidthProperty(), viewport.getHeightProperty(), endX, endY);
        // Real `VoyageGetPosVie(m_nbVies)` uses the PRE-decrement life count (evaluated as a call
        // argument before VoyageInit's body runs) -- kLivesX/Y/Step match GEHud.cpp's own
        // constants exactly (already reused this way for Egg's own dynamic end point in `158`).
        constexpr float kLivesX = 210.0f, kLivesY = 417.0f, kLivesStep = 16.0f;
        const float startX = kLivesX + kLivesStep * static_cast<float>(interaction_.Lives());
        if (!projected)
        {
            // Behind the camera -- collapse onto the start point, same
            // fallback idiom as ResolvePendingVoyage() above.
            endX = startX;
            endY = kLivesY;
        }
        interaction_.BeginVoyage(worldRuntime_, GalaxyEggbert::CNA::GEInteractionSystem::VoyageKind::LifeLoss, 48,
                                 false, startX, kLivesY, endX, endY, sound_);
    }

    void GalaxyEggbertCnaGame::ResolvePickupFreeze()
    {
        using GalaxyEggbert::CNA::GEBlupiController;

        // Starts a new freeze for whichever of the 3 real 2-stage pickups was touched this frame
        // (real immediate "grab" sound: Sucette ch50, Drink ch57, Charge ch58, Decor.cpp:
        // 6025-6087) -- the world object was already destroyed by interaction_.Update() itself.
        if (interaction_.PowerGrantedThisFrame() && blupi_.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Sucette))
        {
            sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel50);
            pendingPickupX_ = interaction_.PowerPickupX();
            pendingPickupY_ = interaction_.PowerPickupY();
            pendingPickupZ_ = interaction_.PowerPickupZ();
            pendingPickupType_ = GalaxyEggbert::ObjectType::ObjectType26;
        }
        else if (interaction_.HideGrantedThisFrame() &&
                 blupi_.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Drink))
        {
            sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel57);
            pendingPickupX_ = interaction_.HidePickupX();
            pendingPickupY_ = interaction_.HidePickupY();
            pendingPickupZ_ = interaction_.HidePickupZ();
            pendingPickupType_ = GalaxyEggbert::ObjectType::ObjectType30;
        }
        else if (interaction_.CloudGrantedThisFrame() &&
                 blupi_.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Charge))
        {
            sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel58);
            // Real m_blupiCloud grants at CONTACT, not completion (confirmed via direct source
            // read, unlike Sucette/Drink) -- this engine's existing instant TriggerCloud() call
            // below (already correct) stays; only the freeze/grab-sound/completion-sound were
            // missing. Real source also redundantly re-arms the gauge again at the 64-tick
            // completion (same values) -- NOT re-applied here, a documented, minor simplification
            // (the gauge decays for those ~3.2s during the freeze instead of being refreshed).
            blupi_.TriggerCloud();
            pendingPickupX_ = interaction_.CloudPickupX();
            pendingPickupY_ = interaction_.CloudPickupY();
            pendingPickupZ_ = interaction_.CloudPickupZ();
            pendingPickupType_ = GalaxyEggbert::ObjectType::ObjectType31;
        }

        // Resolves an ALREADY-active freeze (possibly started a prior frame) that just elapsed --
        // grants the real deferred buff (Sucette/Drink only -- Cloud already granted above),
        // plays the real "complete" sound (ch44/ch62/ch55), and re-spawns the item.
        GEBlupiController::PickupFreezeKind kind{};
        if (!blupi_.ConsumePickupFreezeResolved(kind))
        {
            return;
        }
        switch (kind)
        {
            case GEBlupiController::PickupFreezeKind::Sucette:
                blupi_.TriggerPower();
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel44);
                // Real m_blupiPosMagic reset (plan.md VISUAL-011-adjacent) -- moved here from the
                // old instant-grant site since the Power buff (and thus the magic trail) only
                // actually starts now, at completion, not at contact.
                interaction_.ResetMagicTrail(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
                break;
            case GEBlupiController::PickupFreezeKind::Drink:
                blupi_.TriggerHide();
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel62);
                break;
            case GEBlupiController::PickupFreezeKind::Charge:
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel55);
                break;
        }
        interaction_.RespawnPickupItem(worldRuntime_, pendingPickupX_, pendingPickupY_, pendingPickupZ_,
                                        pendingPickupType_);
    }

    void GalaxyEggbertCnaGame::ApplyCheat(int cheatNumber)
    {
        switch (cheatNumber)
        {
            case 1: // OpenDoors
                interaction_.CheatOpenDoors(worldRuntime_, sound_);
                break;
            case 2: // SuperBlupi -- pure invincibility toggle, NOT "all abilities" (an earlier draft was wrong)
                blupi_.SetCheatSuperBlupi(!blupi_.GetCheatSuperBlupi());
                break;
            case 3: // ShowSecret -- NOT implemented, see this method's own header comment
                break;
            case 4: // "LayEgg" -- real effect sets lives to 9 (the enum name is misleading, no egg is spawned)
                interaction_.SetLives(9);
                break;
            case 5: // Reset gamer progress
                saveData_.Reset();
                saveData_.Save();
                break;
            case 6: // Trial mode simulate-toggle -- no observable effect here, Trial phase is unreachable
                break;
            case 7: // CleanAll
                if (interaction_.CheatCleanAll(worldRuntime_))
                {
                    cameraShake_.Trigger(CameraShakeType::Small);
                }
                break;
            case 8: // AllTreasure
                interaction_.CheatAllTreasure(worldRuntime_, sound_);
                break;
            case 9: // EndGoal -- teleports to the exit; the EXISTING exit-contact check
                    // re-evaluates the real win/no-win gate next frame, no forcing needed
            {
                float exitX = 0.0f, exitY = 0.0f, exitZ = 0.0f;
                if (interaction_.CheatFindExit(worldRuntime_, exitX, exitY, exitZ))
                {
                    blupi_.SetPosition(exitX, exitY, exitZ);
                }
                break;
            }
            default:
                break;
        }
    }

    void GalaxyEggbertCnaGame::Update(Microsoft::Xna::Framework::GameTime& gameTime)
    {
        Game::Update(gameTime);

        // Real `phaseTime` (see phaseTimeSeconds_'s own member comment) --
        // incremented unconditionally every Update() tick, regardless of
        // phase, matching the real source's own `phaseTime++` placement.
        const float dt = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        phaseTimeSeconds_ += dt;

        // Real fade-out phase-transition commit (plan.md MENU-088/089) --
        // freezes ALL input/simulation while a fade is pending (matching
        // the real source's own early-return before `inputPad.Update()`/
        // `decor.MoveStep()`), committing the deferred phase once the
        // real 1.0s window elapses. See SetPhase()'s own comment for the
        // full real deferred-transition mechanic this drives.
        if (fadeOutPhase_ != GalaxyEggbert::GamePhase::None)
        {
            if (phaseTimeSeconds_ >= kFadeCommitDurationSeconds)
            {
                // Real "fadeOutPhase != None is REQUIRED to defer" guard
                // (see SetPhase()'s own comment) -- calling SetPhase()
                // with fadeOutPhase_ STILL set to the pending target
                // (not pre-cleared) makes its own `sourceDefers &&
                // fadeOutPhase_==None` condition false, so it correctly
                // falls through to the commit branch instead of
                // re-deferring to the same target forever.
                SetPhase(fadeOutPhase_);
            }
            return;
        }

        // Play on-screen control input (2026-07-13, plan.md MENU-021..027)
        // -- computed here, in Update()'s own top-level scope, so the
        // movement block further below (still inside the Play-gated
        // section) can read it after the early return this block itself
        // may trigger. Left default-zero when phase_ isn't Play (that
        // branch below never runs inputPad_.UpdatePlay()). inputPadClaimedMouse
        // tells the pre-existing mouse drag-look code (further below) that
        // this frame's press landed on an on-screen control, so the two
        // features don't fight over the same left-mouse-button input.
        GEInputPad::PlayInput padPlayInput;
        bool inputPadClaimedMouse = false;

        // Phase transitions (plan.md HUD-023) -- read regardless of the
        // current phase, since the big Play-gated simulation block below
        // (and its own input reading) doesn't run at all outside Play.
        {
            using Microsoft::Xna::Framework::Input::ButtonState;
            using Microsoft::Xna::Framework::Input::Keyboard;
            using Microsoft::Xna::Framework::Input::Keys;
            using Microsoft::Xna::Framework::Input::Mouse;
            const auto phaseKeys = Keyboard::GetState();
            const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();

            bool mousePausePressed = false;
            bool mouseContinuePressed = false;
            bool mouseRestartPressed = false;
            bool mouseSetupPressed = false;
            bool mouseMenuPressed = false;
            bool mouseBackPressed = false;
            if (phase_ == GalaxyEggbert::GamePhase::Play)
            {
                const auto mouse = Mouse::GetState();
                inputPadClaimedMouse = inputPad_.UpdatePlay(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), padPlayInput);
                mousePausePressed = padPlayInput.pausePressed;

                // Hidden cheat menu (2026-07-13, plan.md CHEAT-001..009):
                // real gesture zones are checked only during real
                // Phase::Play, layered on top of (not instead of) the
                // normal Play controls above -- see GEInputPad::
                // UpdateCheatGesture()'s own class comment for the real-
                // behavior citation and the "layered, not exclusive"
                // input-sharing caveat.
                if (!cheatMenuShown_)
                {
                    if (inputPad_.UpdateCheatGesture(mouse, viewport.getWidthProperty(), viewport.getHeightProperty()))
                    {
                        cheatMenuShown_ = true;
                    }
                }
                else
                {
                    const int cheatPressed = inputPad_.UpdateCheatMenu(
                        mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                    if (cheatPressed != 0)
                    {
                        cheatMenuShown_ = false;
                        ApplyCheat(cheatPressed);
                    }
                }

                // Real SECOND, independent cheat-entry method (plan.md
                // BLUPI-111, see GEInputPad::UpdateTypedGhostCheat()'s own
                // comment) -- typing "ghost" toggles Ghost mode, layered
                // on top of (not instead of) the on-screen cheat menu
                // above, same "layered" idiom already established there.
                if (inputPad_.UpdateTypedGhostCheat(phaseKeys, /*isPlayPhase=*/true))
                {
                    blupi_.ToggleGhost(worldRuntime_.GetWorld());
                }
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Pause)
            {
                const auto mouse = Mouse::GetState();
                // Real conditional visibility: Back/Restart hidden on
                // mission 1 (can't go "back" from/replay the very first
                // level); Restart additionally hidden on decade-boundary
                // missions (real `mission % 10 != 0` gate).
                const int mission = worldRuntime_.GetMissionNumber();
                const bool showBack = mission != 1;
                const bool showRestart = mission != 1 && mission % 10 != 0;
                const auto pauseInput = inputPad_.UpdatePause(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), showBack, showRestart);
                mouseContinuePressed = pauseInput.continuePressed;
                mouseRestartPressed = pauseInput.restartPressed;
                mouseSetupPressed = pauseInput.setupPressed;
                mouseMenuPressed = pauseInput.menuPressed;
                mouseBackPressed = pauseInput.backPressed;
            }
            else if (phase_ == GalaxyEggbert::GamePhase::PlaySetup ||
                     phase_ == GalaxyEggbert::GamePhase::MainSetup)
            {
                // Real PlaySetup (2026-07-13, plan.md MENU-058..069),
                // reachable via Pause's real Setup button, and MainSetup
                // (2026-07-13, plan.md MENU-006..020), reachable via
                // Init's own InitSetup button -- sharing UpdateSetup()/
                // DrawSetup(), the real "MainSetup additionally shows
                // SetupReset" difference (see GEInputPad::UpdateSetup()'s
                // own class comment) is the only distinction made here.
                const bool isMainSetup = phase_ == GalaxyEggbert::GamePhase::MainSetup;
                const auto mouse = Mouse::GetState();
                const auto setupInput = inputPad_.UpdateSetup(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty(), isMainSetup);
                if (setupInput.soundsToggled)
                {
                    // Real SetupSounds toggle -- a genuinely meaningful
                    // desktop equivalent, wired to the pre-existing
                    // GESound::SetEnabled()/IsEnabled(). Persisted
                    // immediately (2026-07-13, plan.md MENU-067, see
                    // GESaveData.hpp), matching the real source's own
                    // `gameData.setSoundActiveProperty(...);
                    // gameData.Write();` -- write-on-toggle, not a timer
                    // or continuous autosave.
                    sound_.SetEnabled(!sound_.IsEnabled());
                    saveData_.SetSoundEnabled(sound_.IsEnabled());
                    saveData_.Save();
                }
                if (setupInput.resetPressed)
                {
                    // Real SetupReset: `gameData.Reset(); gameData.Write();`
                    // -- the SAME full reset as Cheat5 (confirmed
                    // 2026-07-13 via Game1.cpp), only reachable from
                    // MainSetup (isMainSetup gates its rect/visibility).
                    saveData_.Reset();
                    saveData_.Save();
                }
                if (setupInput.returnPressed ||
                    (phaseKeys.IsKeyDown(Keys::Escape) && !pauseKeyWasDown_))
                {
                    // Real SetupReturn: `if (playSetup) SetPhase(Play,-1);
                    // else SetPhase(Init);` -- both branches are now
                    // reachable here. Escape is this engine's own
                    // keyboard pick for the same action (real source has
                    // no separate Setup-phase keyboard binding confirmed).
                    SetPhase(isMainSetup ? GalaxyEggbert::GamePhase::Init : GalaxyEggbert::GamePhase::Play);
                }
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Wait)
            {
                // Real Wait (plan.md MENU-001..005): a fixed 5.0s
                // wall-clock cosmetic timer (confirmed via research,
                // decoupled from actual asset loading, which already
                // finished synchronously in LoadContent()). See
                // GEInputPad::DrawWait()'s own class comment for the real
                // non-linear waitTable gauge-fill curve this timer drives.
                if (phaseTimeSeconds_ >= kWaitDurationSeconds)
                {
                    // Real Wait->Resume-or-Init branch -- ADAPTED trigger
                    // (see GamePhase's own comment for the full
                    // real-vs-adapted reasoning, same one already
                    // established for Resume itself).
                    SetPhase(saveData_.GetHasProgress() ? GalaxyEggbert::GamePhase::Resume
                                                         : GalaxyEggbert::GamePhase::Init);
                }
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Init)
            {
                // Real Init / gamer-select menu (2026-07-13, plan.md
                // MENU-006..020) -- see GEInputPad::UpdateInit()/
                // DrawInit()'s own class comment for full detail.
                const auto mouse = Mouse::GetState();
                const auto initInput = inputPad_.UpdateInit(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                if (initInput.gamerSelected >= 0)
                {
                    // Real Game1::SetGamer(): a single tap immediately
                    // selects AND persists that slot, independent of
                    // entering Play (confirmed via research).
                    saveData_.SetSelectedGamer(initInput.gamerSelected);
                    saveData_.Save();
                }
                if (initInput.playPressed)
                {
                    // Real InitPlay: `SetPhase(Play, 1)` -- always
                    // (re-)enters mission 1, the global hub (2026-07-17,
                    // now that `LoadMission()` exists to make this a real
                    // world load instead of just restaying on whatever was
                    // already loaded). Restores the selected gamer's own
                    // checkpointed lives if that slot has previous progress
                    // (this engine's own adaptation, since the real
                    // per-gamer lastWorld/door state has no equivalent here
                    // beyond lives) -- read AFTER LoadMission(1), which
                    // would otherwise reset lives to the default 3.
                    LoadMission(1);
                    interaction_.SetLives(saveData_.GetHasProgress() ? saveData_.GetLives() : 3);
                    SetPhase(GalaxyEggbert::GamePhase::Play);
                }
                else if (initInput.setupPressed)
                {
                    SetPhase(GalaxyEggbert::GamePhase::MainSetup);
                }
                else if (phaseKeys.IsKeyDown(Keys::Escape) && !pauseKeyWasDown_)
                {
                    // Real hardware Back-button behavior from Init is
                    // Exit() (`Game1.cpp`'s real gamepad-Back handler).
                    // Research separately found that the real source's
                    // OWN Escape key unconditionally maps to Pause
                    // regardless of phase, including from Init -- flagged
                    // by that research as a likely-UNINTENDED quirk of the
                    // real source (Escape apparently meant only for
                    // in-Play pause) rather than deliberate menu-screen
                    // design. NOT replicated here: this engine's own
                    // Escape binding reuses the real hardware-Back->Exit()
                    // behavior instead, which reads as the clearly
                    // intentional one.
                    Exit();
                }
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Resume)
            {
                // Real Resume (2026-07-13, plan.md MENU-040..045) -- see
                // GEInputPad::UpdateResume()'s class comment for the
                // adapted-trigger reasoning; this is just the two real
                // buttons' behavior once already in the phase.
                const auto mouse = Mouse::GetState();
                const auto resumeInput = inputPad_.UpdateResume(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                if (resumeInput.continuePressed)
                {
                    // Real ResumeContinue -> ContinueMission() ->
                    // SetPhase(Play,-2): restores the checkpointed
                    // mission (2026-07-17, now that GESaveData's own
                    // already-persisted missionNumber -- written at every
                    // Win/Lost checkpoint -- is finally read back) and
                    // lives (no real mid-level position/treasure/key state
                    // exists to restore beyond that, same simplification
                    // as PauseRestart/WinLostReturn). The real `-2` mission
                    // sentinel BYPASSES the fade-defer mechanism entirely
                    // (plan.md MENU-088/089, confirmed via research) --
                    // Resume->Play is genuinely instant in the real game.
                    LoadMission(saveData_.GetMissionNumber());
                    interaction_.SetLives(saveData_.GetLives());
                    SetPhase(GalaxyEggbert::GamePhase::Play, /*bypassFade=*/true);
                }
                else if (resumeInput.menuPressed ||
                         (phaseKeys.IsKeyDown(Keys::Escape) && !pauseKeyWasDown_))
                {
                    // Real ResumeMenu/Back -> Init -- now wired
                    // (2026-07-13, now that Init exists; was previously
                    // adapted to an instant restart-without-lives-restore
                    // workaround). This DOES defer/animate (the real
                    // shrink+reverse-spin fade, plan.md MENU-088/089),
                    // since Resume IS one of the 5 real deferring phases
                    // and this path doesn't pass bypassFade.
                    SetPhase(GalaxyEggbert::GamePhase::Init);
                }
            }

            // Real Pause trigger is gamepad-Back/a touch PlayPause button
            // (see phase_'s own class-comment) -- Escape is this engine's
            // own keyboard binding, OR'd with the on-screen PlayPause
            // button click (edge-triggered already, see UpdatePlay()).
            const bool pausePressed = phaseKeys.IsKeyDown(Keys::Escape) || mousePausePressed;
            if (pausePressed && !pauseKeyWasDown_)
            {
                if (phase_ == GalaxyEggbert::GamePhase::Play)
                {
                    SetPhase(GalaxyEggbert::GamePhase::Pause);
                }
                else if (phase_ == GalaxyEggbert::GamePhase::Pause)
                {
                    SetPhase(GalaxyEggbert::GamePhase::Play);
                }
            }
            pauseKeyWasDown_ = pausePressed;

            if (mouseContinuePressed)
            {
                // Real PauseContinue: resume in place.
                SetPhase(GalaxyEggbert::GamePhase::Play);
            }
            else if (mouseRestartPressed)
            {
                // Real PauseRestart (plan.md MENU-036): reload the CURRENT
                // mission fresh -- upgraded 2026-07-17 from the previous
                // origin-respawn-in-place simplification, now that
                // LoadMission() exists to do a genuine level reload
                // (fresh vehicle/secret-power/key/dynamite/treasure state,
                // same as touching any other mission trigger).
                LoadMission(worldRuntime_.GetMissionNumber());
                SetPhase(GalaxyEggbert::GamePhase::Play);
            }
            else if (mouseSetupPressed)
            {
                // Real PauseSetup: SetPhase(PlaySetup).
                SetPhase(GalaxyEggbert::GamePhase::PlaySetup);
            }
            else if (mouseMenuPressed)
            {
                // Real PauseMenu: SetPhase(Init) -- now wired (2026-07-13,
                // now that Init exists).
                SetPhase(GalaxyEggbert::GamePhase::Init);
            }
            else if (mouseBackPressed)
            {
                // Real PauseBack/MissionBack (plan.md MENU-035): wired
                // 2026-07-17 now that the hub/mission-progression system
                // gives it a real destination -- the same
                // ComputeMissionBack() formula shared by exit-reached
                // (WinLostReturn below): a hub (mission%10==0) goes to the
                // global hub (1), a sublevel goes to its own world's hub.
                LoadMission(GEWorldRuntime::ComputeMissionBack(worldRuntime_.GetMissionNumber()));
                SetPhase(GalaxyEggbert::GamePhase::Play);
            }

            if (phase_ == GalaxyEggbert::GamePhase::Win || phase_ == GalaxyEggbert::GamePhase::Lost)
            {
                // Real WinLostReturn button (icon 3), OR'd with this
                // engine's own Space-key pick -- edge-triggered already
                // (see UpdateWinLost()), so no separate debounce needed
                // for the mouse path.
                const auto mouse = Mouse::GetState();
                const bool mouseReturnPressed = inputPad_.UpdateWinLost(
                    mouse, viewport.getWidthProperty(), viewport.getHeightProperty());
                const bool returnPressed = phaseKeys.IsKeyDown(Keys::Space) || mouseReturnPressed;
                if (returnPressed && !phaseReturnKeyWasDown_)
                {
                    if (phase_ == GalaxyEggbert::GamePhase::Win)
                    {
                        // Real WinLostReturn from Win (plan.md TILE-006/
                        // SCORE-013..019): upgraded 2026-07-17 from the
                        // previous origin-respawn simplification -- reaching
                        // the exit now actually advances via the real
                        // win-exit formula (`ComputeWinExitTarget()`, NOT
                        // the plain `ComputeMissionBack()` PauseBack above
                        // uses -- this one additionally special-cases
                        // mission 1's own exit -> 199 and 199's own exit,
                        // real destination is actually Init, not straight
                        // back into Play -- this engine's own already-
                        // established "skip Init, return to Play directly"
                        // simplification is kept as-is, only WHICH mission
                        // loads is now real). Also unlocks the NEXT
                        // sublevel's door in this world's own hub (real
                        // `Decor::OpenDoorsWin()`, `m_doors[mission+1]=1`)
                        // -- only for a genuine sublevel (mission%10!=0);
                        // real source's `else` branch (hub/global-hub wins)
                        // uses `OpenGoldsWin()` instead, a different,
                        // cosmetic-only flag this engine doesn't model.
                        const int currentMission = worldRuntime_.GetMissionNumber();
                        if (currentMission % 10 != 0)
                        {
                            saveData_.UnlockMissionDoor(currentMission + 1);
                        }
                        LoadMission(GEWorldRuntime::ComputeWinExitTarget(currentMission));
                    }
                    else
                    {
                        // Real WinLostReturn from Lost: verified directly
                        // against Decor.cpp:6395-6397/11716 (DoorsLost()) --
                        // Lost does NOT reload the level or reset treasure/
                        // key/dynamite progress, only lives (already reset
                        // to 3 the instant GameOverCount() incremented, see
                        // HUD-023). Repositioning to spawn (unchanged from
                        // before) is the correct, already-accepted behavior
                        // here -- do NOT call LoadMission() on this path,
                        // it would wrongly wipe the current mission's
                        // in-progress treasure/key state.
                        blupi_.SetPosition(0.0f, 1.0f, 0.0f);
                    }
                    SetPhase(GalaxyEggbert::GamePhase::Play);
                }
                phaseReturnKeyWasDown_ = returnPressed;
            }
        }

        if (phase_ != GalaxyEggbert::GamePhase::Play)
        {
            return;
        }

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
            // On-screen D-pad OR'd in (2026-07-13, plan.md MENU-021..024):
            // both axes are already the same discrete {-1,0,+1} shape as
            // the keyboard reads above, so a plain add+clamp combines them
            // without needing a separate "which source wins" rule.
            turnInput = std::clamp(turnInput + padPlayInput.turnInput, -1.0f, 1.0f);
            moveInput = std::clamp(moveInput + padPlayInput.moveInput, -1.0f, 1.0f);
            const bool jumpPressed = keys.IsKeyDown(Keys::LeftControl) || padPlayInput.jumpHeld;
            const bool actionPressed = keys.IsKeyDown(Keys::Space) || padPlayInput.actionPressed;
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
            // Water Surf/Nage detection (plan.md E3D-MIG-148) -- computed
            // from Blupi's PRE-step position (matching tempPassable's own
            // timing above), fed into Step() itself since Nage changes
            // gravity/jump behavior, not just a post-hoc hazard check.
            // GetBlockTypeAt() (the tile he currently occupies) and
            // GetBlockTypeAbove() (one cell above that) together reproduce
            // the real IsSurfWater ("water here, dry above") / IsDeepWater
            // ("water here AND above") distinction -- see
            // GEBlupiController::IsSurf()/IsNage()'s own comment.
            // Real gate (Decor.cpp:5284-5285, found 2026-07-16) also excludes every vehicle mode +
            // Balloon/Ecrase entirely -- while riding/ballooned/squashed, water tiles never
            // trigger Surf/Nage at all (a vehicle drives straight over/through them instead).
            // This engine had no such exclusion; water detection ran unconditionally.
            const bool canEnterWater = !blupi_.IsInVehicle() && !blupi_.IsBallooned() && !blupi_.IsEcrased();
            const auto waterBlockAt = blupi_.GetBlockTypeAt(worldRuntime_.GetWorld());
            const auto waterBlockAbove = blupi_.GetBlockTypeAbove(worldRuntime_.GetWorld());
            const bool atWaterTile = canEnterWater && GalaxyEggbert::BlockTypes::isWater(waterBlockAt);
            const bool aboveIsWaterTile = GalaxyEggbert::BlockTypes::isWater(waterBlockAbove);
            const bool inSurfWater = atWaterTile && !aboveIsWaterTile;
            const bool inDeepWater = atWaterTile && aboveIsWaterTile;
            const bool wasSurf = blupi_.IsSurf();
            const bool wasNage = blupi_.IsNage();
            blupi_.Step(worldRuntime_.GetWorld(), turnInput, moveInput, jumpPressed,
                        crouchHeld, lookUpHeld, dt, tempPassable, inSurfWater, inDeepWater);

            // Real mobile-eggbert jump/land/footstep sounds (2026-07-10).
            // jumpPressed is edge-detected the same way "C" is below, gated
            // on wasOnGround so holding the key while airborne doesn't
            // replay the jump sound.
            if (jumpPressed && !jumpKeyWasDown_ && wasOnGround)
            {
                sound_.PlayJump();
            }
            const auto groundIcon = blupi_.GetGroundBlockType(worldRuntime_.GetWorld());
            // Real mobile-eggbert also suppresses this generic landing-thud
            // sound specifically when the landing spot is a spring
            // (Decor.cpp ~2984, `if (!IsRessort(end))`) -- the bounce sound
            // below covers it instead, since Blupi is about to launch back
            // upward immediately rather than coming to rest.
            if (!wasOnGround && blupi_.IsOnGround() && groundIcon != GalaxyEggbert::BlockTypes::Spring)
            {
                sound_.PlayLand(groundIcon);
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
                    sound_.PlayStep(groundIcon);
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
            // subset; deferred-timing follow-up 2026-07-14) -- takes the
            // real death sound channel for the specific cause (channel 8:
            // fall-off-world/lava, the real shared "you died" sound per
            // 07-sounds.md; channel 51: spikes/drip's real Glu-death sound,
            // distinct from channel 8's) and starts the real death-lock
            // (`GEBlupiController::TriggerDeathLock()`, see its own
            // comment) instead of applying life loss/respawn instantly --
            // both now happen once the lock elapses, via
            // ConsumeDeathLockResolved() below. `cause` drives the real
            // per-type lock duration; `shouldRespawn` is the real
            // `m_blupiRestart` flag for this specific site (see each call
            // site's own comment for its citation).
            const auto triggerDeath = [this](GalaxyEggbert::SoundChannel channel,
                                              GalaxyEggbert::CNA::GEBlupiController::DeathCause cause,
                                              bool shouldRespawn, bool playChannel = true)
            {
                if (playChannel)
                {
                    sound_.Play(channel);
                }
                blupi_.TriggerDeathLock(cause, shouldRespawn);
            };

            // Real Clear2/Clear3 "soul ascends" death VFX (plan.md `158`
            // death-VFX follow-up, `Decor::BlupiDead`'s own Clear2/Clear3
            // branches, Decor.cpp:6589-6606) -- called directly here (not
            // via the pending-request round-trip pickups use) since these
            // 2 real trigger sites (fall-off-world, Lava) already have
            // `camera_`/viewport in scope, unlike the generic-hazard-
            // contact coinflip site inside GEInteractionSystem::Update()
            // (see RequestClear2Ascend()'s own comment). kind must be
            // Clear2Ascend (icon 230, offsetY 300) or Clear3Ascend (icon
            // 40, offsetY 2000).
            // Takes an EXPLICIT death position (not read live from blupi_)
            // -- harmless now that triggerDeath() no longer moves Blupi
            // synchronously (2026-07-14, deferred-timing follow-up), but
            // callers still capture blupi_.GetX/Y/Z() BEFORE calling
            // triggerDeath() for clarity/consistency with every call site.
            const auto triggerDeathAscend = [this](GalaxyEggbert::CNA::GEInteractionSystem::VoyageKind kind,
                                                    float offsetY, int icon, float deathX, float deathY, float deathZ)
            {
                const auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
                float hudX = 0.0f, hudY = 0.0f;
                if (GEHud::ProjectWorldToHudSpace(Microsoft::Xna::Framework::Vector3(deathX, deathY, deathZ),
                                                   camera_.GetViewMatrix(), camera_.GetProjectionMatrix(),
                                                   viewport.getWidthProperty(), viewport.getHeightProperty(), hudX,
                                                   hudY))
                {
                    interaction_.BeginVoyage(worldRuntime_, kind, icon, false, hudX, hudY, hudX, hudY - offsetY,
                                              sound_, deathX, deathY, deathZ);
                }
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
            // reproduced a comparable ~6.2s fall (verified live at 6.72s)
            // using this engine's own already-tuned kGravity=25/
            // kFallLimit=-10 (not the real tick-domain values, which
            // aren't cross-checked against this engine's own constants
            // per plan.md `065`) -- an equivalent NUMBER OF SECONDS, not
            // the same literal unit distance, since this world's own
            // terrain (Y 0-13) is far shorter than a real level's, so
            // matching real "feel" (a real fall you notice) makes more
            // sense here than matching the real absolute margin.
            //
            // **-27.0f, not -60.0f (user feedback, 2026-07-13): even the
            // "feel"-matched ~6.2-6.7s read as too long in practice.** User
            // asked for roughly half. Using this engine's own kGravity=25/
            // kFallLimit=-10: reaching terminal velocity takes a fixed
            // 0.4s (10/25) covering 2.0 units regardless of the threshold;
            // the rest is covered at the 10 units/s terminal rate. This is
            // a game-feel adjustment, not a faithfulness correction (the
            // real absolute margin was never matched here anyway, see
            // above) -- verified live at 3.417s (down from the previously
            // live-verified 6.72s), i.e. almost exactly half.
            constexpr float kFallDeathY = -27.0f;
            if (blupi_.GetY() < kFallDeathY)
            {
                const float deathX = blupi_.GetX();
                const float deathY = blupi_.GetY();
                const float deathZ = blupi_.GetZ();
                // Real m_blupiRestart=true at this site (Decor.cpp:2757).
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel8,
                             GalaxyEggbert::CNA::GEBlupiController::DeathCause::Clear2, true);
                // Real Clear2 ascend (plan.md `158` death-VFX follow-up,
                // Decor.cpp:2754-2761) -- deterministic, no coinflip.
                triggerDeathAscend(GalaxyEggbert::CNA::GEInteractionSystem::VoyageKind::Clear2Ascend, 300.0f, 230,
                                    deathX, deathY, deathZ);
            }

            // Lava hazard (plan.md E3D-MIG-140) -- deterministic death, no
            // vehicle immunity (mobile-eggbert-reference/
            // 12-hazards-and-interactables.md: "Lava(icon 68): deterministic
            // Clear3, no vehicle immunity, no focus requirement" -- Shield/
            // Hide immunity IS modeled now, plan.md E3D-MIG-170, confirmed
            // directly against Decor.cpp:5497) -- the
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
            if (!blupi_.IsInvincible() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Lava)
            {
                const float deathX = blupi_.GetX();
                const float deathY = blupi_.GetY();
                const float deathZ = blupi_.GetZ();
                // Real m_blupiRestart=true at this site (Decor.cpp:5500).
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel8,
                             GalaxyEggbert::CNA::GEBlupiController::DeathCause::Clear3, true);
                // Real Clear3 ascend (plan.md `158` death-VFX follow-up,
                // Decor.cpp:5497-5499) -- deterministic, no coinflip.
                triggerDeathAscend(GalaxyEggbert::CNA::GEInteractionSystem::VoyageKind::Clear3Ascend, 2000.0f, 40,
                                    deathX, deathY, deathZ);
            }

            // Spikes hazard (plan.md E3D-MIG-141) -- real channel 51 (the
            // Glu-death sound, distinct from lava/fall's channel 8, per
            // 07-sounds.md). Real behavior (12-hazards-and-interactables.md)
            // gates this on vehicle immunity (Over/Jeep/Tank protect,
            // unlike lava) and requires m_blupiFocus -- focus doesn't exist
            // in GalaxyEggbertCNA yet, same simplification as every hazard
            // here, but vehicle immunity now DOES exist (`HasVehicleHazard
            // Immunity()`, fixed 2026-07-16 -- this comment previously said
            // "neither vehicles nor a focus concept exist... yet", stale
            // since vehicles were implemented `171`). The real check also
            // restricts to a narrow central x-band within the tile
            // (`pos.X%64` roughly 15-49, touching the tile's edges doesn't
            // count) -- not modeled, since GEBlupiController's single-point
            // 3D collision has no sub-tile position within a cell to test
            // against; the whole tile is lethal here.
            if (!blupi_.IsInvincible() && !blupi_.HasVehicleHazardImmunity() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spike)
            {
                // Real BlupiAction::Glu, m_blupiRestart=true (Decor.cpp:5504-5510).
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel51,
                             GalaxyEggbert::CNA::GEBlupiController::DeathCause::Glu, true);
            }

            // Water drip hazard (plan.md TILE-032, real Decor::IsGoutte,
            // confirmed 2026-07-14 via direct Decor.cpp read -- despite the
            // "drip"/glue-sounding name, real behavior is a deterministic
            // kill, mechanically identical to Spike above: same
            // BlupiAction::Glu death, same gate shape, same real channel 51
            // sound (Decor.cpp:5513-5519), same real Over/Jeep/Tank vehicle
            // immunity (fixed 2026-07-16, same as Spike above).
            if (!blupi_.IsInvincible() && !blupi_.HasVehicleHazardImmunity() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Drip)
            {
                // Real BlupiAction::Glu, m_blupiRestart=true (Decor.cpp:5513-5519).
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel51,
                             GalaxyEggbert::CNA::GEBlupiController::DeathCause::Glu, true);
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
            if (!blupi_.IsInvincible() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Blitz &&
                GEWorldRuntime::IsBlitzActiveAtPhase(worldRuntime_.GetAnimPhase()))
            {
                // Real BlupiAction::Clear1, m_blupiRestart=true (Decor.cpp:5541-5547).
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel8,
                             GalaxyEggbert::CNA::GEBlupiController::DeathCause::Clear1, true);
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
            if (!blupi_.IsInvincible() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Crusher &&
                GEWorldRuntime::IsCrusherActiveAtPhase(worldRuntime_.GetAnimPhase()) &&
                blupi_.TriggerCrush())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel70);
            }

            // Saw hazard (plan.md E3D-MIG-142) -- real channel 75 (the
            // "cut apart" death cue, distinct from every other hazard's
            // sound so far), deterministic, real Over/Jeep/Tank vehicle
            // immunity (`HasVehicleHazardImmunity()`, fixed 2026-07-16,
            // identical to Spike/Drip above -- this comment previously said
            // "no immunity simplification beyond what every hazard here
            // already lacks (vehicles... don't exist yet)", stale since
            // vehicles were implemented `171`). Only the active icon
            // (`Saw`, 378) is lethal -- the stopped variant (`SawStopped`,
            // 379) is a separate BlockTypes value, so GetGroundBlockType()'s
            // exact match already excludes it with no extra check needed. A
            // saw starts active or stopped per however the world was
            // authored; TryActivateSwitch() below is what flips it between
            // the two.
            if (!blupi_.IsInvincible() && !blupi_.HasVehicleHazardImmunity() &&
                blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Saw)
            {
                const float deathX = blupi_.GetX();
                const float deathY = blupi_.GetY();
                const float deathZ = blupi_.GetZ();
                // Real source has NO separate contact-site sound for Saw
                // (unlike Lava/fall's own extra channel 8) -- channel 75
                // comes entirely from BlupiDead's own Clear4 branch, which
                // SpawnSawDeathBurst() plays itself (plan.md `158`
                // death-VFX follow-up, Decor.cpp:5521-5524/6608-6613), so
                // triggerDeath() must NOT also play it here. Real
                // m_blupiRestart=true at this site (Decor.cpp:5526).
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel75,
                             GalaxyEggbert::CNA::GEBlupiController::DeathCause::Clear4, true, false);
                interaction_.SpawnSawDeathBurst(worldRuntime_, deathX, deathY, deathZ, sound_);
            }

            // Spring / bounce tile (plan.md E3D-MIG-145, icon 211,
            // verified directly against Decor.cpp:2835-2911/7312-7320) --
            // NOT a hazard, launches Blupi upward instead of costing a
            // life. TriggerSpringBounce() is idempotent (a no-op while
            // already airborne, matching the real `!m_blupiAir` guard), so
            // only play the real bounce sound (channel 41) on an actual new
            // trigger. Real vehicle-dismount-first gate fixed 2026-07-16
            // (Decor.cpp:2837-2893): touching a spring while riding ANY
            // vehicle (Shield/Hide protect against this, matching every
            // other real forced-dismount-style gate) forcibly ejects
            // Blupi first (small shake + channel 10, depositing the
            // vehicle pickup back into the world via the same
            // `DismountAndDepositVehicle()` helper the voluntary
            // action-button dismount uses) -- the bounce itself still
            // applies on the same contact, real source does not treat
            // these as mutually exclusive.
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spring &&
                blupi_.IsInVehicle() && blupi_.GetSecretPower() != GEBlupiController::SecretPower::Shield &&
                blupi_.GetSecretPower() != GEBlupiController::SecretPower::Hide)
            {
                DismountAndDepositVehicle();
                cameraShake_.Trigger(CameraShakeType::Small);
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel10);
            }
            if (blupi_.GetGroundBlockType(worldRuntime_.GetWorld()) == GalaxyEggbert::BlockTypes::Spring &&
                blupi_.TriggerSpringBounce(jumpPressed))
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel41);
            }

            // Hub/mission-progression: world-select portal contact (plan.md
            // hub/mission system, found/implemented 2026-07-17, real
            // `Decor::IsWorld()`/the `Bye`-action mission-change handler).
            // Contact-triggered, no debounce needed -- unlike every other
            // ground-tile check here, `LoadMission()` itself repositions
            // Blupi to a DIFFERENT world's spawn point, so he's never still
            // standing on the same portal tile the frame after it fires (by
            // construction: no world in this engine places a portal marker
            // at its own spawn point). The real ~1.5s "turn and wave" `Bye`
            // animation delay is NOT modeled (documented simplification,
            // same category as this session's other timing simplifications).
            // Returns immediately after LoadMission() (same "freeze this
            // frame" precedent already used for the fade-transition window)
            // so nothing below operates on stale locals derived from the
            // world that just got replaced.
            {
                const auto groundBlock = blupi_.GetGroundBlockType(worldRuntime_.GetWorld());
                if (GalaxyEggbert::BlockTypes::isWorldSelect(groundBlock))
                {
                    const int target = GEWorldRuntime::ComputeWorldSelectTarget(
                        worldRuntime_.GetMissionNumber(), GalaxyEggbert::BlockTypes::worldSelectIndex(groundBlock));
                    LoadMission(target);
                    return;
                }
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
            // TriggerTeleport() itself (grounded, not ballooned/squashed,
            // and -- fixed 2026-07-16 -- not in any vehicle mode either;
            // focus alone still isn't modeled). Idempotent, same shape as
            // every other Trigger*() here, so channel 71 only plays on an
            // actual new trigger.
            const auto aboveIcon = blupi_.GetBlockTypeAbove(worldRuntime_.GetWorld());
            if ((aboveIcon == GalaxyEggbert::BlockTypes::Teleport1 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport2 ||
                 aboveIcon == GalaxyEggbert::BlockTypes::Teleport3 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport4) &&
                blupi_.TriggerTeleport(aboveIcon))
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel71);
                // Real teleporter arc (plan.md VISUAL-010, ObjectType92,
                // Decor.cpp:5606) -- despite ObjectType.hpp's own "charged
                // attack" doc comment, this is the real, only spawn site.
                interaction_.SpawnTeleportArc(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
            }

            // Fan hazard (plan.md E3D-MIG-149, see GEWorldRuntime::
            // TryConsumeFan()'s own comment for the real IsVentillo() source
            // this ports, and for why only the head-tile consumption is
            // implemented, not the real trail-walk). Real "kills only if
            // m_blupiFocus && unshielded/unhidden/not-SuperBlupi" -- Shield/
            // Hide immunity IS modeled now (plan.md E3D-MIG-170); focus/
            // superBlupi don't exist in this engine yet. The fan is still
            // consumed (cleared) regardless of immunity, matching the real
            // "a shielded Blupi walking through a fan still pops it but
            // survives" behavior -- only the death itself is gated. Real
            // channel 10 (the fan's own contact sound, distinct from lava/
            // spike/blitz's channel 8/51) plays via triggerDeath() itself,
            // same pattern as every hazard above. BigShake IS modeled
            // (plan.md CAM-009, 2026-07-14) -- verified directly against
            // Decor.cpp: the real trigger site is the SAME `if
            // (m_blupiFocus && !shield && !hide && !superBlupi)` block
            // that calls BlupiDead(), i.e. the shake fires exactly when
            // the fan actually kills Blupi, not on every fan contact --
            // matches this engine's own `!IsInvincible()` gate on
            // triggerDeath() below exactly. The real cosmetic ObjectType11
            // shockwave flash spawned at the SAME site is now also
            // modeled (plan.md VISUAL-008-adjacent, 2026-07-14, see
            // `GEInteractionSystem::SpawnFanHitFlash()`'s own comment).
            if (worldRuntime_.TryConsumeFan(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ()) && !blupi_.IsInvincible())
            {
                // Captured BEFORE triggerDeath() respawns Blupi -- the
                // flash/ascend must originate from the death position, not
                // the respawn position (fixes a pre-existing bug: this
                // call previously read blupi_.GetX/Y/Z() AFTER triggerDeath()
                // had already moved it).
                const float deathX = blupi_.GetX();
                const float deathY = blupi_.GetY();
                const float deathZ = blupi_.GetZ();
                cameraShake_.Trigger(CameraShakeType::Big);
                interaction_.SpawnFanHitFlash(worldRuntime_, deathX, deathY, deathZ);
                // Real 50/50 Clear1/Clear2 coinflip (plan.md `158`
                // death-VFX follow-up, Decor.cpp:5458-5463) -- unlike
                // fall-off-world/Lava, Fan's death is NOT deterministically
                // Clear2. Real m_blupiRestart=false at this site (no
                // `m_blupiRestart=true` anywhere near Decor.cpp:5458-5472).
                const bool isClear2 = interaction_.RollClear2Coinflip();
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel10,
                             isClear2 ? GalaxyEggbert::CNA::GEBlupiController::DeathCause::Clear2
                                      : GalaxyEggbert::CNA::GEBlupiController::DeathCause::Clear1,
                             false);
                if (isClear2)
                {
                    triggerDeathAscend(GalaxyEggbert::CNA::GEInteractionSystem::VoyageKind::Clear2Ascend, 300.0f, 230,
                                        deathX, deathY, deathZ);
                }
            }

            // Water Surf/Nage (plan.md E3D-MIG-148) -- transition sounds via
            // before/after comparison (wasSurf/wasNage captured before
            // Step() above), same idiom as every other status this session.
            // Real channel 25 (start-surfing) plays specifically on
            // Nage->Surf (resurfacing). Real vehicle exclusion fixed
            // 2026-07-16 -- this comment previously mis-described it as
            // "forced dismount on water entry"; verified directly against
            // Decor.cpp:5284-5285: it's not a dismount at all, water tiles
            // simply never register as Surf/Nage while riding/ballooned/
            // squashed (`canEnterWater` above, computed before Step()) -- a
            // vehicle just drives straight over/through water instead.
            //
            // **Corrected 2026-07-17** (water-splash research): channel 22
            // was wired backwards -- verified directly against every real
            // `PlaySound(SoundChannel22, ...)` call site (`Decor.cpp:5356/
            // 5378/5392/5405`): ALL four are in the water-EXIT paths, none
            // in entry. Real water ENTRY instead plays channel 23 ("Plouf",
            // `Decor::MoveObjectPlouf`, `ObjectType14`) via the two dry->
            // Surf/dry->Nage branches (`Decor.cpp:5298/5318`) -- previously
            // entirely unwired (no code/sound at all on entry). Also added:
            // the jump-triggered exit's extra "Tiplouf" splash (channel 64,
            // `ObjectType35`, `Decor.cpp:5355-5356`) -- real source
            // distinguishes a deliberate jump-out (Jump held near the
            // surface, `MoveObjectTiplouf`+ch22 together) from a passive
            // drift-out (`Decor.cpp:5359-5408`, ch22 alone, 3 sub-cases by
            // vertical velocity sign, cosmetically identical) -- this
            // engine's single-point collision has no equivalent drift-out
            // distinction, so `jumpPressed` at the exact exit frame is used
            // as a documented approximation for "this was a deliberate jump
            // exit" (matches the common case: players hold Jump through the
            // whole swim-up-and-surface action).
            const bool wasDry = !wasSurf && !wasNage;
            const bool nowDry = !blupi_.IsSurf() && !blupi_.IsNage();
            if (wasDry && !nowDry && !interaction_.HasActiveObjectOfType(worldRuntime_, GalaxyEggbert::ObjectType::ObjectType14))
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel23);
                interaction_.SpawnWaterSplash(worldRuntime_, GalaxyEggbert::ObjectType::ObjectType14, blupi_.GetX(),
                                               blupi_.GetY(), blupi_.GetZ());
            }
            if (!wasDry && nowDry)
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel22);
                if (jumpPressed && !interaction_.HasActiveObjectOfType(worldRuntime_, GalaxyEggbert::ObjectType::ObjectType35))
                {
                    sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel64);
                    interaction_.SpawnWaterSplash(worldRuntime_, GalaxyEggbert::ObjectType::ObjectType35, blupi_.GetX(),
                                                   blupi_.GetY(), blupi_.GetZ());
                }
            }
            if (wasNage && blupi_.IsSurf())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel25);
            }
            // Ambient rising bubbles while fully submerged (plan.md
            // PICKUP-079, ObjectType15/channel 24) -- real trigger
            // `m_time % ScaleTime(70) == 0 || == ScaleTime(28)` (twice per
            // ~3.5s cycle, `Decor.cpp:4611-4613`), reusing this engine's own
            // 1:1-tick `GetAnimPhase()` the same way every other real
            // `m_time %`-gated periodic effect already does.
            if (blupi_.IsNage())
            {
                const int animTick = worldRuntime_.GetAnimPhase();
                if (animTick % 70 == 0 || animTick % 70 == 28)
                {
                    sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel24);
                    interaction_.SpawnWaterBubble(worldRuntime_, worldRuntime_.GetWorld(), blupi_.GetX(), blupi_.GetY(),
                                                   blupi_.GetZ());
                }
            }
            // Drowning (real BlupiAction::Drown, channel 26 -- a dedicated
            // death sound distinct from every other cause's channel 8/51/75,
            // per 07-sounds.md's own note). Real Shield/Hide/SuperBlupi
            // immunity gates the death itself (plan.md E3D-MIG-170); the
            // gauge still depletes regardless (matches every other hazard's
            // "still triggers, only the death is gated" pattern) -- only
            // superBlupi isn't modeled.
            if (blupi_.JustDrowned() && !blupi_.IsInvincible())
            {
                // Real BlupiAction::Drown, m_blupiRestart=true (Decor.cpp:4640-4652).
                triggerDeath(GalaxyEggbert::SoundChannel::SoundChannel26,
                             GalaxyEggbert::CNA::GEBlupiController::DeathCause::Drown, true);
            }

            // Real 10-slot safe-position FIFO respawn (plan.md E3D-MIG-067,
            // see GEBlupiController::UpdateSafePosition()'s own comment) --
            // "safe" here additionally means not standing on any of the 6
            // real terrain hazard tiles (Lava/Spike/Saw/active-Blitz/Temp/
            // Drip), not currently under a teleporter trigger (`aboveIcon`, already
            // computed above), and not currently Nage (plan.md E3D-MIG-148 --
            // respawning mid-drown with an already-depleted gauge would be a
            // bad experience, same reasoning as excluding active hazards) --
            // the categories of "unsafe tile/status" this engine can
            // actually check for; real vehicle/shield/ledge-teeter/
            // transport-riding/projectile-path checks aren't modeled (see
            // UpdateSafePosition's own comment).
            {
                const auto safetyGroundBlock = blupi_.GetGroundBlockType(worldRuntime_.GetWorld());
                const bool onHazardTile =
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Lava ||
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Spike ||
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Saw ||
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Temp ||
                    safetyGroundBlock == GalaxyEggbert::BlockTypes::Drip ||
                    (safetyGroundBlock == GalaxyEggbert::BlockTypes::Blitz &&
                     GEWorldRuntime::IsBlitzActiveAtPhase(worldRuntime_.GetAnimPhase()));
                const bool underTeleporter =
                    aboveIcon == GalaxyEggbert::BlockTypes::Teleport1 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport2 ||
                    aboveIcon == GalaxyEggbert::BlockTypes::Teleport3 || aboveIcon == GalaxyEggbert::BlockTypes::Teleport4;
                blupi_.UpdateSafePosition(!onHazardTile && !underTeleporter && !blupi_.IsNage());
            }

            // Switches (plan.md E3D-MIG-142, see GEWorldRuntime::
            // TryActivateSwitch()'s own comment for the real 41-cell
            // switch-to-saw linking) -- Space ("Action"), edge-detected the
            // same way jumpPressed is above so holding it doesn't retoggle
            // every frame. `wasEcrased`/`wasOnGround`-style capture isn't
            // needed here: TryActivateSwitch() itself is the single
            // authoritative check (grounded + standing on a switch tile),
            // no separate before/after state to compare.
            // Captured into a local (not just the `if` condition below)
            // since Sucette/Drink's own real action-button gate (plan.md
            // `173`) needs this SAME edge-detected value passed into
            // interaction_.Update() further down, by which point
            // actionKeyWasDown_ has already been overwritten with this
            // frame's raw actionPressed (see its own assignment below).
            const bool actionPressedEdge = actionPressed && !actionKeyWasDown_;
            if (actionPressedEdge)
            {
                // Real switch-activation gate (found 2026-07-16, Decor.cpp:5529-5531) also
                // excludes Overcraft/Jeep/Tank/Skateboard and Balloon -- NOT Helicopter, which
                // real source allows (a hovering Helicopter can still reach down and press a
                // switch, unlike the other 4 ground-committed/no-hands vehicle states). Same
                // "predates vehicle modeling" gap already found/fixed this session elsewhere
                // (pickups/TriggerTeleport/ground-jump/hazard immunity) -- TryActivateSwitch()
                // itself has no GEBlupiController access, so this is gated here in the caller
                // instead, matching that established decoupling. Scoped to JUST this call, NOT
                // the whole actionPressedEdge block below -- Dynamite/Perso/vehicle mount-dismount
                // share the same action-button press but have their own independent real gates
                // (in particular, dismounting a vehicle must still work while riding one).
                const bool canActivateSwitch =
                    blupi_.GetVehicleMode() != GalaxyEggbert::CNA::GEBlupiController::VehicleMode::Overcraft &&
                    blupi_.GetVehicleMode() != GalaxyEggbert::CNA::GEBlupiController::VehicleMode::Jeep &&
                    blupi_.GetVehicleMode() != GalaxyEggbert::CNA::GEBlupiController::VehicleMode::Tank &&
                    blupi_.GetVehicleMode() != GalaxyEggbert::CNA::GEBlupiController::VehicleMode::Skateboard &&
                    !blupi_.IsBallooned();
                if (canActivateSwitch)
                {
                    if (const auto turnedOn = worldRuntime_.TryActivateSwitch(
                            blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(), blupi_.IsOnGround()))
                    {
                        sound_.Play(*turnedOn ? GalaxyEggbert::SoundChannel::SoundChannel77
                                              : GalaxyEggbert::SoundChannel::SoundChannel76);
                    }
                }

                // Dynamite/Perso PLACEMENT vehicle gate (found 2026-07-16, Decor.cpp:4792-4794):
                // real source excludes EVERY vehicle mode (unlike switches, Helicopter is NOT
                // exempt here) plus Balloon/Ecrase. Deliberately does NOT gate Perso PICKUP
                // (TryPerso()'s own internal pickup branch, real Decor.cpp:6088-6101 has no
                // vehicle clause at all) -- see PlaceDynamite()/TryPerso()'s own header comments.
                const bool blupiCanUseHands = !blupi_.IsInVehicle() && !blupi_.IsBallooned() && !blupi_.IsEcrased();

                // Dynamite placement (plan.md E3D-MIG-155) -- same action
                // button, independent gate (carrying one + grounded) from
                // the switch check above, so both can coexist on one press
                // without stepping on each other (real channel 61, shared
                // with skin-swap per 07-sounds.md, unrelated here).
                if (interaction_.PlaceDynamite(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                                blupi_.IsOnGround(), blupiCanUseHands))
                {
                    sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel61);
                }
                else
                {
                    // Perso decoy placement/pickup (plan.md HUD-017) --
                    // real `else if (m_blupiPerso > 0)`, mutually exclusive
                    // with dynamite above. Channel 61 is the real placement
                    // sound (per 07-sounds.md), unaffected by Voyage timing.
                    // The pickup case's own real touch-time sound (ch60)
                    // and deferred completion sound (ch3, plan.md `158`)
                    // are handled by ResolvePendingVoyage() below instead
                    // -- PersoCount() no longer increments immediately, so
                    // it can't be used to distinguish pickup from placement
                    // anymore; a placement always DECREASES the count
                    // immediately (unaffected by Voyage), so that's the
                    // real distinguishing signal now.
                    const int persoBefore = interaction_.PersoCount();
                    if (interaction_.TryPerso(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                               blupi_.IsOnGround(), blupiCanUseHands) &&
                        interaction_.PersoCount() < persoBefore)
                    {
                        sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel61);
                    }
                    ResolvePendingVoyage();
                }

                // Vehicle mount/dismount (plan.md E3D-MIG-171) -- same
                // action button. While already riding, dismounts (real:
                // voluntary dismount, no fixed duration otherwise) and
                // deposits the vehicle pickup back into the world at
                // Blupi's position, matching the real "deposits vehicle
                // pickup back into the world" behavior. While not riding,
                // scans for a nearby active vehicle pickup (real confirmed
                // mapping: ObjectType13->Helicopter, 19->Jeep, 28->Tank,
                // 24->Skateboard, 46->Overcraft) and mounts it -- done
                // directly here (not inside GEInteractionSystem::Update(),
                // which has no access to GEBlupiController::VehicleMode)
                // matching the same architecture as TryActivateSwitch/
                // PlaceDynamite above. Real "requires the action button
                // held/pressed at contact" gate is modeled via the same
                // proximity radius every other pickup here uses; the real
                // helicopter-only auto-mount-over-a-gap exception
                // (`Decor::IsFloatingObject`) is NOT modeled.
                if (blupi_.IsInVehicle())
                {
                    DismountAndDepositVehicle();
                }
                else
                {
                    constexpr float kVehicleMountRadius = 1.1f;
                    for (auto& obj : worldRuntime_.GetMobileObjectsMutable())
                    {
                        if (!obj.active) continue;
                        GEBlupiController::VehicleMode mode;
                        switch (obj.type)
                        {
                            case GalaxyEggbert::ObjectType::ObjectType13: mode = GEBlupiController::VehicleMode::Helicopter; break;
                            case GalaxyEggbert::ObjectType::ObjectType19: mode = GEBlupiController::VehicleMode::Jeep; break;
                            case GalaxyEggbert::ObjectType::ObjectType28: mode = GEBlupiController::VehicleMode::Tank; break;
                            case GalaxyEggbert::ObjectType::ObjectType24: mode = GEBlupiController::VehicleMode::Skateboard; break;
                            case GalaxyEggbert::ObjectType::ObjectType46: mode = GEBlupiController::VehicleMode::Overcraft; break;
                            default: continue;
                        }
                        const float mdx = obj.currentX - blupi_.GetX();
                        const float mdy = obj.currentY - blupi_.GetY();
                        const float mdz = obj.currentZ - blupi_.GetZ();
                        if (mdx * mdx + mdy * mdy + mdz * mdz < kVehicleMountRadius * kVehicleMountRadius &&
                            blupi_.TriggerMount(mode, blupi_.IsNage(), blupi_.IsSurf()))
                        {
                            obj.active = false;
                            break;
                        }
                    }
                }
            }
            actionKeyWasDown_ = actionPressed;

            // Interactive objects (2026-07-10, see GEInteractionSystem.hpp)
            // -- platform lift patrol + riding (plan.md E3D-MIG-152, 2026-
            // 07-12), crate push, pickup collection, generic hazard contact
            // (ObjectType2/3/4/16/17/20/96/97), and (2026-07-11) the wasp's
            // balloon status. Runs after blupi_.Step() so blupi_'s position
            // is this frame's final value; blupiXBeforeStep lets the
            // interaction system infer movement direction for crate push
            // without GEBlupiController needing a velocity accessor.
            // crouchHeld gates ObjectType3's real duck-immunity;
            // blupi_.IsBallooned() gates whether a 3/16/96/97 hazard pops
            // the balloon instead of killing. blupiFacingDX/DZ (plan.md
            // E3D-MIG-160) is Blupi's facing rounded to the nearest
            // cardinal grid direction (GetYaw()'s own 0=-Z/forward=
            // (sin,0,-cos) convention), used for the door-detection probe
            // one cell ahead of him.
            const int blupiFacingDX = static_cast<int>(std::lround(std::sin(blupi_.GetYaw())));
            const int blupiFacingDZ = static_cast<int>(std::lround(-std::cos(blupi_.GetYaw())));
            // Secret powers (plan.md E3D-MIG-170) -- blupiCanGrantX mirrors
            // each TriggerX()'s own internal gate exactly (see
            // GEBlupiController::TriggerShield()/Power()/Cloud()/Hide()'s
            // own comments), computed here from GetSecretPower() since
            // GEInteractionSystem has no access to GEBlupiController.
            const auto secretPower = blupi_.GetSecretPower();
            const bool canGrantShield = secretPower != GEBlupiController::SecretPower::Shield &&
                                        secretPower != GEBlupiController::SecretPower::Hide &&
                                        secretPower != GEBlupiController::SecretPower::Power;
            // Real Sucette(26)/Drink(30)/Charge(31) gates also exclude every
            // vehicle mount plus Balloon/Ecrase (Decor.cpp:6025-6087:
            // !m_blupiHelico/Over/Balloon/Ecrase/Jeep/Tank/Skate) -- Shield
            // and Invert have no such clause in real source (verified
            // directly, NEXT.md's listing of Shield here was wrong), so
            // canGrantShield/canGrantInvert deliberately don't get this.
            const bool blupiVehicleOrSquashed = blupi_.GetVehicleMode() != GEBlupiController::VehicleMode::None ||
                                                 blupi_.IsBallooned() || blupi_.IsEcrased();
            const bool canGrantPower = secretPower != GEBlupiController::SecretPower::Shield &&
                                        !blupiVehicleOrSquashed;
            const bool canGrantCloud = secretPower == GEBlupiController::SecretPower::None &&
                                        !blupiVehicleOrSquashed;
            const bool canGrantHide = secretPower != GEBlupiController::SecretPower::Shield &&
                                       secretPower != GEBlupiController::SecretPower::Cloud &&
                                       !blupiVehicleOrSquashed;
            // Invert/Mirror (plan.md PICKUP-011) -- mirrors
            // GEBlupiController::TriggerInvert()'s own gate exactly (not
            // already Invert, not Hide); independent of the 4 powers above.
            const bool canGrantInvert = !blupi_.IsInverted() && secretPower != GEBlupiController::SecretPower::Hide;
            // Real Tank "Fire" (2026-07-13, plan.md BULLET-001) -- a
            // dedicated key (real `KeyPressFlags::Fire`), NOT the Action
            // button used for dynamite/Perso/switches/vehicle mount above.
            // "F" is this engine's own keyboard pick (no real keyboard
            // binding exists to match -- WP7 touch-only). Level state, not
            // edge-triggered (see GEInteractionSystem::Update()'s own
            // comment for why). Gated to Tank only -- Helicopter's own
            // real firing branch was not independently confirmed this
            // session, a documented gap, not modeled here.
            const bool firePressed = keys.IsKeyDown(Keys::F);
            const bool canFire = blupi_.GetVehicleMode() == GEBlupiController::VehicleMode::Tank;
            // Captured before Update() so the Lost transition below can
            // detect the exact frame GameOverCount() increments (plan.md
            // HUD-023's own real trigger, Decor.cpp:6374-6435).
            const int gameOverCountBeforeUpdate = interaction_.GameOverCount();
            // Real BlupiElectro aura (plan.md `068`) -- active whenever
            // Cloud is the current secret power (this engine's own
            // SecretPower::Cloud, matching the real Power-Charge pickup).
            const bool cloudActive = secretPower == GEBlupiController::SecretPower::Cloud;

            // Ghost mode (plan.md BLUPI-111) real "no interactions"
            // behavior: real `MoveObjectDetect()` (the shared query nearly
            // every pickup/hazard/enemy-contact/lift-riding check in this
            // function is ultimately built on) unconditionally returns "no
            // object found" while ghosting -- every check in this
            // function is a proximity test against Blupi's own position
            // (see this class's own repeated "same simplification as
            // every other proximity test in this file" comments), so
            // substituting a position far outside the 100x100 world makes
            // every one of them correctly fail shut, with zero changes
            // needed inside GEInteractionSystem itself. Patrol/animation
            // logic (which never references Blupi's position at all)
            // continues normally, matching real behavior exactly.
            constexpr float kGhostSentinelPos = 100000.0f;
            const float interactionBlupiX = blupi_.IsGhost() ? kGhostSentinelPos : blupi_.GetX();
            const float interactionBlupiY = blupi_.IsGhost() ? kGhostSentinelPos : blupi_.GetY();
            const float interactionBlupiZ = blupi_.IsGhost() ? kGhostSentinelPos : blupi_.GetZ();
            // Crate-push gate (found 2026-07-16, Decor.cpp:6130-6132): real source also excludes
            // every vehicle mode + Ecrase (Balloon is covered separately by the existing
            // blupi_.IsBallooned() argument just below).
            const bool canPushCrate = !blupi_.IsInVehicle() && !blupi_.IsEcrased();
            interaction_.Update(dt, worldRuntime_, interactionBlupiX, interactionBlupiY, interactionBlupiZ,
                                 blupi_.GetX() - blupiXBeforeStep, sound_, crouchHeld,
                                 blupi_.IsBallooned(), blupiFacingDX, blupiFacingDZ, blupi_.IsInvincible(),
                                 canGrantShield, canGrantPower, canGrantCloud, canGrantHide,
                                 firePressed, canFire, cloudActive, canGrantInvert, actionPressedEdge,
                                 canPushCrate);

            // Voyage (plan.md `158`) -- a pickup touched above may have
            // recorded a this-frame voyage request (GEInteractionSystem has
            // no camera access, so it can't project the world<->HUD-space
            // endpoint itself). Resolve it here and start the real voyage.
            ResolvePendingVoyage();
            // Death-lock/life-loss Voyage follow-up -- see its own comment.
            ResolveDeathLock();
            // Sucette/Drink/Charge real 2-stage pickup delay -- see its own comment.
            ResolvePickupFreeze();

            // Pollution puff (plan.md VISUAL-013) -- called unconditionally
            // every frame, matching the real function's own internal gate
            // (see TickPollutionPuff()'s own comment). isMoving reuses the
            // same blupiXBeforeStep delta already computed above for the
            // interaction system's crate-push direction inference, instead
            // of adding a new horizontal-speed accessor to GEBlupiController.
            {
                const auto vehicleMode = blupi_.GetVehicleMode();
                interaction_.TickPollutionPuff(
                    worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                    vehicleMode == GEBlupiController::VehicleMode::Helicopter,
                    vehicleMode == GEBlupiController::VehicleMode::Overcraft,
                    vehicleMode == GEBlupiController::VehicleMode::Jeep,
                    vehicleMode == GEBlupiController::VehicleMode::Tank,
                    blupi_.GetX() != blupiXBeforeStep, blupi_.GetVelocityY() > 0.0f, blupiFacingDX);
            }

            // Shield/Power magic trail (plan.md VISUAL-011-adjacent) --
            // called unconditionally every frame; TickMagicTrail() itself
            // is the gate (a no-op unless Shield or Power is active).
            interaction_.TickMagicTrail(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                        secretPower == GEBlupiController::SecretPower::Shield,
                                        secretPower == GEBlupiController::SecretPower::Power);

            // Real camera shake (plan.md CAM-008/009) -- generic-hazard
            // contact-kill (most types SmallShake, fish/bird BigShake) and
            // the dynamite blast's own center-tile SmallShake, both
            // signaled from GEInteractionSystem::Update() above.
            if (interaction_.SmallShakeTriggeredThisFrame())
            {
                cameraShake_.Trigger(CameraShakeType::Small);
            }
            if (interaction_.BigShakeTriggeredThisFrame())
            {
                cameraShake_.Trigger(CameraShakeType::Big);
            }

            // Real crate-push loop sound (found 2026-07-16, Decor.cpp:6138/6147 start, `:3637`
            // stop on leaving `BlupiAction::Push`) -- ch38, not "electric arc (long)" as
            // plan.md's own SOUND-048 entry claimed. `CrateBeingPushedThisFrame()` is a plain
            // per-frame fact (true only while a push is actually moving a crate this frame, no
            // memory of its own); comparing it against the previous frame's value here is what
            // turns that into a real start-once/stop-once loop, matching the real source's own
            // one-shot `PlaySound()`-on-entry/`StopSound()`-on-exit shape (not a per-frame replay).
            const bool isPushingCrate = interaction_.CrateBeingPushedThisFrame();
            if (isPushingCrate && !wasPushingCrate_)
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel38, /*loop=*/true);
            }
            else if (!isPushingCrate && wasPushingCrate_)
            {
                sound_.Stop(GalaxyEggbert::SoundChannel::SoundChannel38);
            }
            wasPushingCrate_ = isPushingCrate;

            // Real vehicle motor sound crossfade (plan.md SOUND-007/008, found 2026-07-17) --
            // see UpdateVehicleMotorSound()'s own comment. Runs every frame regardless of vehicle
            // state; it's the real per-mode `if`/`else if` chain inside the method itself that
            // decides whether a motor should be playing at all.
            UpdateVehicleMotorSound();

            // Real Win/Lost phase transitions (plan.md HUD-023): Lost
            // fires the instant GameOverCount() increments (real
            // DoorsLost()); Win fires the instant ExitReached() becomes
            // true (real IsTerminated(), already gated on holding every
            // treasure -- see GEInteractionSystem::Update()'s own exit
            // handling).
            if (interaction_.GameOverCount() > gameOverCountBeforeUpdate)
            {
                // Real MemorizeGamerProgress() checkpoint (2026-07-13,
                // plan.md MENU-040..045) -- confirmed via research to
                // fire automatically at exactly this real transition.
                saveData_.SetLives(interaction_.Lives());
                saveData_.SetMissionNumber(worldRuntime_.GetMissionNumber());
                saveData_.SetHasProgress(true);
                saveData_.Save();
                SetPhase(GalaxyEggbert::GamePhase::Lost);
            }
            else if (interaction_.ExitReached())
            {
                saveData_.SetLives(interaction_.Lives());
                saveData_.SetMissionNumber(worldRuntime_.GetMissionNumber());
                saveData_.SetHasProgress(true);
                saveData_.Save();
                SetPhase(GalaxyEggbert::GamePhase::Win);
            }

            // Platform lift riding (plan.md E3D-MIG-152): IsRidingLift()
            // reflects whether Blupi was standing on an active lift BEFORE
            // interaction_.Update() just advanced its patrol step -- see
            // GEInteractionSystem::IsRidingLift()'s own comment for the
            // delta-vs-snap rationale.
            if (interaction_.IsRidingLift())
            {
                blupi_.RideLift(blupi_.GetX() + interaction_.RideDeltaX(), interaction_.RideStandY(),
                                 blupi_.GetZ() + interaction_.RideDeltaZ());
            }

            // Secret power grants (plan.md E3D-MIG-170) -- Shield is real,
            // single-stage, instant (TriggerX()'s own internal gate should
            // agree with what GEInteractionSystem just checked, both read
            // the same GetSecretPower() state). Power/Cloud/Hide (Sucette/
            // Charge/Drink) are real 2-stage pickups -- see
            // ResolvePickupFreeze() below for their own grab/freeze/
            // complete handling (plan.md `173`, 2026-07-14).
            if (interaction_.ShieldGrantedThisFrame() && blupi_.TriggerShield())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel42);
                // Real m_blupiPosMagic reset (plan.md VISUAL-011-adjacent,
                // Decor.cpp:6022) -- the magic trail's first marker only
                // appears after a further real 40px of movement from here.
                interaction_.ResetMagicTrail(blupi_.GetX(), blupi_.GetY(), blupi_.GetZ());
            }

            // Invert/Mirror grant + expiry (plan.md PICKUP-011) -- real
            // channels 66 (activate) / 67 (expire), confirmed against
            // mobile-eggbert-reference/13-object-pickups.md's "Mirror/
            // Invert" section. No warning-threshold sound (unlike the 4
            // powers above) -- real source has no warning stage for this
            // buff either.
            if (interaction_.InvertGrantedThisFrame() && blupi_.TriggerInvert())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel66);
                interaction_.SpawnInvertBurst(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                               /*isGrant=*/true);
            }
            if (blupi_.JustExpiredInvert())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel67);
                interaction_.SpawnInvertBurst(worldRuntime_, blupi_.GetX(), blupi_.GetY(), blupi_.GetZ(),
                                               /*isGrant=*/false);
            }

            // Secret power warning sound (plan.md E3D-MIG-170) -- fires
            // once at the real per-power remaining-level threshold
            // (Shield@10=channel43, Power@20=channel45, Cloud@25=channel56,
            // Hide@20=channel63).
            if (blupi_.JustCrossedSecretPowerWarning())
            {
                switch (blupi_.GetSecretPower())
                {
                    case GEBlupiController::SecretPower::Shield:
                        sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel43);
                        break;
                    case GEBlupiController::SecretPower::Power:
                        sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel45);
                        break;
                    case GEBlupiController::SecretPower::Cloud:
                        sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel56);
                        break;
                    case GEBlupiController::SecretPower::Hide:
                        sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel63);
                        break;
                    default:
                        break;
                }
            }

            // Real respawn/life-loss for hazard-contact deaths (DiedThisFrame()
            // still true instantly on contact, for tests/other consumers) is
            // now deferred to the death-lock/life-loss-Voyage system --
            // resolved via ResolveDeathLock() above, not here (death-VFX
            // follow-up, 2026-07-14; this used to respawn instantly).

            // Wasp balloon status (plan.md E3D-MIG-135) -- TriggerBalloon()
            // is idempotent (a no-op while already ballooned, matching the
            // real `!m_blupiBalloon` re-trigger guard), so only play the
            // real entry sound (channel 40) on an actual new trigger.
            // ElectricShake (plan.md CAM-010) fires at this exact same
            // real site (Decor.cpp:5864, confirmed via direct source
            // read) -- despite the enum's own generic "electric field"
            // doc comment, the real trigger IS specifically wasp-sting/
            // balloon entry, not a separate electric-field-tile hazard.
            if (interaction_.BalloonTouchedThisFrame() && blupi_.TriggerBalloon())
            {
                sound_.Play(GalaxyEggbert::SoundChannel::SoundChannel40);
                cameraShake_.Trigger(CameraShakeType::Electric);
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
            //
            // inputPadClaimedMouse (2026-07-13, plan.md MENU-021..027)
            // forces lookHeld false whenever this frame's press landed on
            // an on-screen D-pad/Jump/Action/Pause control (computed
            // earlier this same Update(), before the Play-gate) -- without
            // it, dragging the on-screen D-pad would also spin the camera,
            // since both features read the same left-mouse-button state.
            {
                using Microsoft::Xna::Framework::Input::ButtonState;
                using Microsoft::Xna::Framework::Input::Mouse;
                const auto mouse = Mouse::GetState();
                const int mouseX = mouse.getXProperty();
                const int mouseY = mouse.getYProperty();
                const bool lookHeld =
                    mouse.getLeftButtonProperty() == ButtonState::Pressed && !inputPadClaimedMouse;
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
                const Easy3D::Camera3D::Vector3 desiredOffset(
                    -std::sin(yaw) * kChaseDistance * std::cos(elevation),
                    kChaseDistance * std::sin(elevation),
                    std::cos(yaw) * kChaseDistance * std::cos(elevation));

                // Wall collision (plan.md CAM-005, found 2026-07-17): pull
                // the eye in along the same direction if a solid block
                // sits between Blupi and the desired chase position, so
                // the camera never clips through a wall/tunnel ceiling to
                // end up looking back in from outside.
                const float desiredDistance = desiredOffset.Length();
                float safeDistance = desiredDistance;
                if (desiredDistance > 0.001f)
                {
                    const auto direction = desiredOffset * (1.0f / desiredDistance);
                    safeDistance = RaymarchWallDistance(worldRuntime_.GetWorld(), rawTarget, direction, desiredDistance);
                }
                rawEye = rawTarget + desiredOffset * (desiredDistance > 0.001f ? safeDistance / desiredDistance : 1.0f);
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

            // Real screen-shake/forced-pan effect (plan.md CAM-008..013,
            // see GECameraShake.hpp's own comment) -- only ticks during
            // real Play (matches the real `m_bPause` freeze gate; no other
            // phase should animate it). Real (dx,dy) is a 2D screen-space
            // scroll-offset in pixels; the closest natural 3D adaptation
            // (CLAUDE.md's allowed "perspective camera" adaptations) is a
            // small bodily camera TRANSLATION (added equally to eye AND
            // target, preserving look direction) along world X/height,
            // using the same 64px-per-world-unit conversion already used
            // throughout this engine's own tile/atlas math.
            if (phase_ == GalaxyEggbert::GamePhase::Play)
            {
                cameraShake_.Update(dt);
            }
            constexpr float kShakePixelsToWorldUnits = 1.0f / 64.0f;
            const Easy3D::Camera3D::Vector3 shakeOffset(
                cameraShake_.GetOffsetX() * kShakePixelsToWorldUnits,
                -cameraShake_.GetOffsetY() * kShakePixelsToWorldUnits, // real +Y is screen-down; this engine's +Y is up
                0.0f);
            camera_.SetPosition(cameraEyeSmoothed_ + shakeOffset);
            camera_.SetTarget(cameraTargetSmoothed_ + shakeOffset);

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

        device.Clear(0.392f, 0.584f, 0.929f, 1.0f);
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
        // opaque quad.
        if (phase_ != GalaxyEggbert::GamePhase::Wait && phase_ != GalaxyEggbert::GamePhase::Init)
        {
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
            // Real `Hide` during the death-lock's own life-loss-Voyage window
            // (death-VFX follow-up, `IsDeathHidden()`'s own comment) -- first-
            // person mode already renders no Blupi model at all, so this is
            // the only render-side change needed.
            if (cameraMode_ == CameraMode::ThirdPersonModel && blupiModelLoaded_ && blupiAvatarRenderer_ &&
                !blupi_.IsDeathHidden())
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
                terrainPixelPrintedFrame_ = drawFrameIndex_;
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
                // Real -1 "invisible frame" sentinel (plan.md VISUAL-009,
                // table_sploutch2/3's own leading delay ticks, fixed
                // 2026-07-14) -- skip drawing this object entirely this
                // tick, matching the real source's own blank-frame behavior.
                if (icon < 0)
                {
                    continue;
                }
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
        } // phase_ != Wait && phase_ != Init (3D world render guard above)

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
            // Pause/Win/Lost (2026-07-13, plan.md MENU-028..039/046..057):
            // the real HUD is skipped entirely, replaced by
            // inputPad_.DrawPause()/DrawWinLost()'s real background/
            // character/buttons below -- fully hiding the normal HUD
            // outside Play via a dedicated draw call instead of a generic
            // text message.
            const bool phaseHasRealScreen = phase_ == GalaxyEggbert::GamePhase::Pause ||
                                             phase_ == GalaxyEggbert::GamePhase::Win ||
                                             phase_ == GalaxyEggbert::GamePhase::Lost ||
                                             phase_ == GalaxyEggbert::GamePhase::PlaySetup ||
                                             phase_ == GalaxyEggbert::GamePhase::MainSetup ||
                                             phase_ == GalaxyEggbert::GamePhase::Resume ||
                                             phase_ == GalaxyEggbert::GamePhase::Wait ||
                                             phase_ == GalaxyEggbert::GamePhase::Init;
            if (!phaseHasRealScreen)
            {
                // Training-hint lookup (plan.md HUD-024): real grid position
                // (not render-centered, GEWorldRuntime::kWorldCenterX/Z offset
                // reversed, matching every other grid<->render conversion this
                // session).
                const int hintGridX = static_cast<int>(std::lround(blupi_.GetX())) + GEWorldRuntime::kWorldCenterX;
                const int hintGridZ = static_cast<int>(std::lround(blupi_.GetZ())) + GEWorldRuntime::kWorldCenterZ;
                const char* trainingHint = FindTrainingHint(
                    worldRuntime_.GetMissionNumber(), hintGridX, hintGridZ,
                    interaction_.TreasuresCollected(), blupi_.IsInVehicle(), interaction_.DynamiteCount() > 0);
                hud_.Draw(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                          interaction_.Lives(),
                          interaction_.Key1Count() > 0, interaction_.Key2Count() > 0,
                          interaction_.Key3Count() > 0,
                          interaction_.TreasuresCollected(), interaction_.TotalTreasures(),
                          interaction_.BulletCount(), interaction_.DynamiteCount(), interaction_.PersoCount(),
                          blupi_.IsNage(), blupi_.GetWaterGaugeLevel(),
                          blupi_.GetSecretPower() != GEBlupiController::SecretPower::None,
                          blupi_.GetSecretPowerLevel(),
                          trainingHint,
                          PhaseOverlayMessage(),
                          blupi_.GetAnimIcon(),
                          interaction_.VoyageActive(), interaction_.VoyageIconId(),
                          interaction_.VoyageIsButtonChannel(),
                          interaction_.VoyageDrawX(), interaction_.VoyageDrawY());
            }

            // On-screen touch controls (2026-07-13, plan.md
            // MENU-021..027/028..039/046..057, see GEInputPad.hpp): the
            // real Pause screen (background/character/5 real buttons)
            // while paused, the real Win/Lost screen (background/
            // character animation/Return button) while won/lost, or the
            // real D-pad/Jump/Action/Pause overlay on top of the live 3D
            // scene + HUD while playing.
            if (phase_ == GalaxyEggbert::GamePhase::Pause)
            {
                const int mission = worldRuntime_.GetMissionNumber();
                const bool showBack = mission != 1;
                const bool showRestart = mission != 1 && mission % 10 != 0;
                inputPad_.DrawPause(
                    device, viewport.getWidthProperty(), viewport.getHeightProperty(), showBack, showRestart,
                    phaseTimeSeconds_, fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Win || phase_ == GalaxyEggbert::GamePhase::Lost)
            {
                inputPad_.DrawWinLost(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phase_ == GalaxyEggbert::GamePhase::Win, phaseTimeSeconds_);
            }
            else if (phase_ == GalaxyEggbert::GamePhase::PlaySetup ||
                     phase_ == GalaxyEggbert::GamePhase::MainSetup)
            {
                inputPad_.DrawSetup(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                    sound_.IsEnabled(), phase_ == GalaxyEggbert::GamePhase::MainSetup,
                                    saveData_.GetSelectedGamer(), phaseTimeSeconds_, fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Resume)
            {
                inputPad_.DrawResume(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                     phaseTimeSeconds_, fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Wait)
            {
                inputPad_.DrawWait(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                   phaseTimeSeconds_);
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Init)
            {
                inputPad_.DrawInit(device, viewport.getWidthProperty(), viewport.getHeightProperty(),
                                   phaseTimeSeconds_, saveData_.GetSelectedGamer(),
                                   saveData_.GetLivesForGamer(0), saveData_.GetLivesForGamer(1),
                                   saveData_.GetLivesForGamer(2), fadeOutPhase_);
            }
            else if (phase_ == GalaxyEggbert::GamePhase::Play)
            {
                inputPad_.DrawPlay(device, viewport.getWidthProperty(), viewport.getHeightProperty());
            }

            // Hidden cheat menu overlay (2026-07-13, plan.md
            // CHEAT-001..009) -- real source renders it as a transparent
            // overlay layered on top of whatever's already on screen, no
            // background swap and no phase change, so this draws LAST,
            // on top of the just-drawn Play controls above (only ever
            // shown during Play, see Update()'s own gating).
            if (cheatMenuShown_ && phase_ == GalaxyEggbert::GamePhase::Play)
            {
                inputPad_.DrawCheatMenu(device, viewport.getWidthProperty(), viewport.getHeightProperty());
            }
        }

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
