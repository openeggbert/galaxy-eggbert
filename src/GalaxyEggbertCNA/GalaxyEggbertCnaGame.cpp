#include "GalaxyEggbertCnaGame.hpp"

#include "GalaxyEggbert/BlockTypes.hpp"

#include <Easy3D/BillboardBatch.hpp>
#include <Easy3D/BillboardMesh.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>

namespace GalaxyEggbert::CNA
{
    GalaxyEggbertCnaGame::GalaxyEggbertCnaGame()
    {
        Game::getWindowProperty().setTitleProperty("Galaxy Eggbert (CNA)");
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

        // Interim 2D Blupi animation-state indicator (no 3D model yet,
        // 2026-07-05) — same blupi.png already copied next to this binary
        // for the eventual billboard (E3D-MIG-061..063).
        blupiIconTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D("Content/icons/blupi.png", device);
        blupiIconBatch_ = std::make_unique<Microsoft::Xna::Framework::Graphics::SpriteBatch>(device);

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
        // Built once here, not per frame, since these are static-phase-only
        // cubes whose vertices don't depend on the camera (unlike the
        // billboards above). Center.Y uses the same "+kObjectGroundOffset"
        // convention as the MoveObject billboards (kObjectGroundOffset=1.0f
        // below in Draw()) so a cube object sits on the ground exactly like
        // an already-verified-correct billboard would at the same position.
        {
            objectCubeEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
            objectCubeEffect_->VertexColorEnabled = false;
            objectCubeEffect_->setTextureEnabledProperty(true);
            objectCubeEffect_->setTextureProperty(&terrainTexture_);

            constexpr float kObjectCubeGroundOffset = 1.0f; // matches kObjectGroundOffset in Draw()
            std::vector<Easy3D::CubeVertex> cubeVertices;
            std::vector<std::uint32_t> cubeIndices;
            int cubeObjectCount = 0;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                if (!IsUniformCubeObject(obj.type))
                {
                    continue;
                }
                ++cubeObjectCount;
                const int icon = GetObjIcon(obj.type, 0);
                Easy3D::CubeItem item;
                item.Center = Easy3D::CubeBatch::Vector3(
                    obj.posStartX, obj.posStartY + kObjectCubeGroundOffset, obj.posStartZ);
                item.Size = Easy3D::CubeBatch::Vector3(1.0f, 1.0f, 1.0f);
                item.Uv = tileAtlas_.GetTileUv(icon);
                Easy3D::AppendCubeMesh(item, cubeVertices, cubeIndices);
            }

            if (!cubeIndices.empty())
            {
                objectCubeMeshRenderer_ = std::make_unique<Easy3D::CubeMeshRenderer>(device, cubeVertices, cubeIndices);
            }
            std::cout << "GalaxyEggbertCNA: " << cubeObjectCount
                      << " platform-lift/crate cube object(s) built." << std::endl;
        }

        // Spawn Blupi on the ground floor (world (0,1,0) == grid (50,*,50),
        // inside worlds3d/world001.vwr's ground floor). The .vwr format
        // carries no spawn point itself (see LoadFromVwrFile()), so this is
        // a fixed known-good spot for this specific sample world. The
        // camera now follows blupi_ every frame (see Update()) instead of a
        // fixed terrain-centroid shot.
        blupi_.SetPosition(0.0f, 1.0f, 0.0f);

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
            [[maybe_unused]] const bool actionPressed = keys.IsKeyDown(Keys::Space);
            const bool crouchHeld = keys.IsKeyDown(Keys::LeftShift);
            const bool lookUpHeld = keys.IsKeyDown(Keys::RightShift);
            blupi_.Step(worldRuntime_.GetWorld(), turnInput, moveInput, jumpPressed,
                        crouchHeld, lookUpHeld, dt);

            // First-person/player-view camera (2026-07-05): no 3D Blupi
            // model exists yet, so there is nothing for a third-person
            // camera to show — look from Blupi's eye position in his
            // current facing direction instead (see GEBlupiController's
            // GetYaw() convention: 0 rad = facing -Z). Crouching lowers the
            // eye height; looking up tilts the look target upward — neither
            // is a real head/body pose (no 3D model yet), just a rough
            // camera-only stand-in for the animation-indicator state.
            constexpr float kEyeHeight = 0.75f;
            constexpr float kCrouchEyeHeight = 0.4f;
            constexpr float kLookDistance = 5.0f;
            constexpr float kLookUpTilt = 2.5f;
            const float yaw = blupi_.GetYaw();
            const float eyeHeight = crouchHeld ? kCrouchEyeHeight : kEyeHeight;
            const float lookYOffset = lookUpHeld ? kLookUpTilt : 0.0f;
            const Easy3D::Camera3D::Vector3 eye(blupi_.GetX(), blupi_.GetY() + eyeHeight, blupi_.GetZ());
            camera_.SetPosition(eye);
            camera_.SetTarget(Easy3D::Camera3D::Vector3(
                eye.X + std::sin(yaw) * kLookDistance,
                eye.Y + lookYOffset,
                eye.Z - std::cos(yaw) * kLookDistance));
        }
    }

    void GalaxyEggbertCnaGame::Draw(const Microsoft::Xna::Framework::GameTime& gameTime)
    {
        (void)gameTime;
        auto& device = getGraphicsDeviceProperty();
        device.Clear(0.392f, 0.584f, 0.929f, 1.0f);
        device.SetDepthTestEnabled(true);

        if (terrainRenderer_ && terrainEffect_)
        {
            terrainEffect_->View = camera_.GetViewMatrix();
            terrainEffect_->Projection = camera_.GetProjectionMatrix();
            terrainEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
            terrainRenderer_->Draw(device, *terrainEffect_);

            if (objectCubeMeshRenderer_ && objectCubeEffect_)
            {
                // Platform-lift/crate UniformCube objects (NEXT.md §8 task 3)
                // -- opaque, drawn alongside the main terrain pass, same
                // texture sheet (terrainTexture_) as terrainEffect_ but its
                // own effect instance since World/View/Projection are set
                // independently per draw call.
                objectCubeEffect_->View = camera_.GetViewMatrix();
                objectCubeEffect_->Projection = camera_.GetProjectionMatrix();
                objectCubeEffect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
                objectCubeMeshRenderer_->Draw(device, *objectCubeEffect_);
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
                        // clearly different is terrain.
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

        // Billboard rendering for worldRuntime_'s parsed MoveObjects
        // (15-3d-render-mapping-design.md §5/§7, first pass 2026-07-06) —
        // static phase only (no per-instance animation timers yet), element.png
        // only (see GEObjectIcons.hpp's known-limitation note re: DOC-007).
        // Rebuilt every frame since billboard vertex positions depend on the
        // camera (Easy3D::BillboardMeshRenderer's header comment).
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
                // -- skip here so they aren't drawn twice. object-m.png-
                // sourced types (see objectMPngMeshRenderer_ below) are also
                // skipped here -- GetElementIconUv() would compute the wrong
                // UV rect for them (element.png icon-index domain, not
                // object-m.png's).
                if (IsUniformCubeObject(obj.type) || IsObjectMPngSourced(obj.type))
                {
                    continue;
                }
                const int icon = GetObjIcon(obj.type, 0);
                const auto uv = GetElementIconUv(icon);
                batch.Add(
                    Microsoft::Xna::Framework::Vector3(obj.posStartX, obj.posStartY + kObjectGroundOffset, obj.posStartZ),
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
                if (!IsObjectMPngSourced(obj.type))
                {
                    continue;
                }
                const int icon = GetObjIcon(obj.type, 0);
                const auto uv = tileAtlas_.GetTileUv(icon);
                batch.Add(
                    Microsoft::Xna::Framework::Vector3(obj.posStartX, obj.posStartY + kObjectGroundOffset, obj.posStartZ),
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

        // Interim 2D Blupi animation-state indicator, bottom-right corner
        // (no 3D model yet, 2026-07-05 — see GalaxyEggbertCnaGame.hpp).
        // blupi.png: 60x60 px tiles, 10 columns per row (matches
        // GalaxyEggbertSimple3D::GEBlupiController's kTilePx/kCols).
        if (blupiIconBatch_)
        {
            constexpr int kTilePx = 60;
            constexpr int kCols = 10;
            constexpr int kOnScreenSize = 96;
            constexpr int kMargin = 8;

            const int icon = blupi_.GetAnimIcon();
            const int col = icon % kCols;
            const int row = icon / kCols;
            const Microsoft::Xna::Framework::Rectangle srcRect(col * kTilePx, row * kTilePx, kTilePx, kTilePx);

            const auto& viewport = device.getViewportProperty();
            const int screenW = viewport.getWidthProperty();
            const int screenH = viewport.getHeightProperty();
            const Microsoft::Xna::Framework::Rectangle destRect(
                screenW - kOnScreenSize - kMargin, screenH - kOnScreenSize - kMargin,
                kOnScreenSize, kOnScreenSize);

            blupiIconBatch_->Begin();
            blupiIconBatch_->Draw(blupiIconTexture_, destRect, srcRect,
                                   Microsoft::Xna::Framework::Color::White);
            blupiIconBatch_->End();
        }
    }

    GetTypeNameCPP(GalaxyEggbertCnaGame, "GalaxyEggbertCnaGame")
}
