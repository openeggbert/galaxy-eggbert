// GalaxyEggbertGame: its content loaded (LoadContent) and the world presentation built from it (terrain, models, reliefs). Split out of GalaxyEggbertGame.cpp.
#include "GalaxyEggbertGame.hpp"
#include "GameInternal.hpp"

#include <GalaxyEggbert/Game/TerrainMaps.hpp>
#include <GalaxyEggbert/Game/TextureScale.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsAdapter.hpp>
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

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "GameHelpers.hpp"

namespace GalaxyEggbert::CNA
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::CNA::detail;

    void GalaxyEggbertGame::LoadContent()
    {
        // Default world source: a genuinely 3D, hand-authored .vwr world
        // (plan.md E3D-MIG-058), not a flat mobile-eggbert .txt layout.
        // LoadFromMobileEggbertFile() stays available on WorldRuntime as a
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

        // Tile-atlas sanity check (plan.md E3D-MIG-053) — nothing builds a
        // cube from this yet; this only proves TileAtlas resolves
        // known block types to plausible UV rects.
        const auto printTileUv = [this](const char* name, int blockType)
        {
            const auto uv = tileAtlas_.GetTileUv(blockType);
            std::cout << "GalaxyEggbertCNA: tile " << name << " (" << blockType
                      << ") UV = (" << uv.u0 << ", " << uv.v0 << ") - ("
                      << uv.u1 << ", " << uv.v1 << ")" << std::endl;
        };
        printTileUv("Ground", GalaxyEggbert::BlockTypes::Ground);
        printTileUv("Lava", GalaxyEggbert::BlockTypes::Lava);
        printTileUv("GoldPillar", GalaxyEggbert::BlockTypes::GoldPillar);

        // Static terrain mesh for the loaded world (plan.md E3D-MIG-054) —
        // one box per non-air cell, textured via TileAtlas.
        auto& device = getGraphicsDeviceProperty();

        // The textures' size for this run (TextureScale.hpp: the Setup's "Textures", GALAXY_TEXTURES, or
        // auto: 2x with Ultra on a display at least 1000 pixels tall), decided once before any texture loads.
        // The golden captures are 1x whatever the folder's savedata.txt says (its textureScale=2 made
        // effects-high's frame differ, 2026-11-10); GALAXY_TEXTURES still overrides.
        GalaxyEggbert::Game::SetTextureScaleSetting(goldenCaptureMode_ || goldenTraceMode_ ? 1
                                                                                          : saveData_.GetTextureScale());
        const int displayHeight = Microsoft::Xna::Framework::Graphics::GraphicsAdapter::getDefaultAdapterProperty()
                                      .getCurrentDisplayModeProperty()
                                      .getHeightProperty();
        const int wantedTextureScale = GalaxyEggbert::Game::ResolveTextureScale(
            displayHeight, EffectsLevel() >= GalaxyEggbert::Game::SaveData::kEffectsLevelUltra);
        std::cout << "GalaxyEggbertCNA: textures " << wantedTextureScale << "x (setting "
                  << saveData_.GetTextureScale() << ", display " << displayHeight << " px tall, effects level "
                  << EffectsLevel() << ")." << std::endl;

        // object-m.png was already copied next to this binary at build time
        // (plan.md E3D-MIG-030); loaded directly via CNA's own Texture2D —
        // no mobile-eggbert Pixmap reuse (SpriteBatch-coupled, not usable for
        // this 3D BasicEffect draw path). Content/icons2x/object-m.png when 2x is wanted: twice the size,
        // every tile and gap twice as large, so every texel step below takes the scale.
        int terrainScale = 1;
        terrainTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
            GalaxyEggbert::Game::ScaledContentPath("Content/icons/object-m.png", &terrainScale), device);
        GalaxyEggbert::Game::SetTextureScale(&terrainTexture_, terrainScale);
        terrainTileAtlas_ = GalaxyEggbert::Game::TileAtlas(GalaxyEggbert::Game::TileAtlas::Layout::Padded, terrainScale);
        std::cout << "GalaxyEggbertCNA: terrain texture loaded — "
                  << terrainTexture_.getWidthProperty() << "x"
                  << terrainTexture_.getHeightProperty() << " px." << std::endl;
        {
            // Which tiles are sprites on clear texels, for the terrain's passes (TileCoverage.hpp).
            const int width = terrainTexture_.getWidthProperty();
            const int height = terrainTexture_.getHeightProperty();
            std::vector<Microsoft::Xna::Framework::Color> texels(static_cast<std::size_t>(width) *
                                                                 static_cast<std::size_t>(height));
            terrainTexture_.GetData(texels.data(), static_cast<int>(texels.size()));
            std::vector<std::uint8_t> alpha(texels.size());
            for (std::size_t i = 0; i < texels.size(); ++i)
            {
                alpha[i] = texels[i].getAProperty();
            }
            tileCoverage_ = GalaxyEggbert::Game::ClassifyTileSheet(alpha.data(), width, height, terrainScale);
            GalaxyEggbert::Game::MarkSeeThroughTiles(tileCoverage_); // the cheese's holes (Q-010)
            // The brick edge pieces' ragged ends (Q-011) are a table measured from this sheet: say so if
            // the sheet loaded tells otherwise (BrickEdges.hpp).
            // (Measured in the 1x sheet's texel rows: a 2x sheet is the same picture, the 1x one is checked
            // whenever it loads.)
            for (const auto& tile : GalaxyEggbert::Game::BrickEdges::kTiles)
            {
                const int tileTexels = GalaxyEggbert::BlockTypes::kTileSize * terrainScale;
                const int gap = GalaxyEggbert::BlockTypes::kSheetGap * terrainScale;
                const int kPitch = tileTexels + gap;
                const int columns = width / tileTexels;
                const int x0 = gap + tile.icon % columns * kPitch;
                const int y0 = gap + tile.icon / columns * kPitch;
                if (terrainScale != 1 || y0 + tileTexels > height || x0 + tileTexels > width)
                    continue;
                const std::uint8_t* at = alpha.data() + static_cast<std::size_t>(y0) * width + x0;
                using GalaxyEggbert::Game::BrickEdges::MeasureCourseInsets;
                const auto left = MeasureCourseInsets(at, width, true, GalaxyEggbert::Game::kCutoutReferenceAlpha);
                const auto right = MeasureCourseInsets(at, width, false, GalaxyEggbert::Game::kCutoutReferenceAlpha);
                const auto ragged = [](const std::array<int, 4>& measured, const std::array<int, 4>& table) {
                    for (std::size_t i = 0; i < 4; ++i)
                        if (table[i] != 0 && measured[i] != table[i]) return false;
                    return true;
                };
                if (!ragged(left, tile.left) || !ragged(right, tile.right))
                    std::cout << "GalaxyEggbertCNA: warning: brick edge " << tile.icon
                              << "'s courses differ from BrickEdges::kTiles in this object-m.png." << std::endl;
            }
            // No black where an opaque tile's sprite is clear (TileCoverage.hpp).
            std::vector<std::uint8_t> rgba(texels.size() * 4);
            for (std::size_t i = 0; i < texels.size(); ++i)
            {
                rgba[i * 4 + 0] = texels[i].getRProperty();
                rgba[i * 4 + 1] = texels[i].getGProperty();
                rgba[i * 4 + 2] = texels[i].getBProperty();
                rgba[i * 4 + 3] = texels[i].getAProperty();
            }
            GalaxyEggbert::Game::FillBrickEdgesFromWholeBrick(rgba.data(), width, height, terrainScale); // Q-011
            GalaxyEggbert::Game::FillOpaqueTilesClearTexels(rgba.data(), width, height, tileCoverage_, terrainScale);
            for (std::size_t i = 0; i < texels.size(); ++i)
            {
                texels[i] = Microsoft::Xna::Framework::Color(rgba[i * 4 + 0], rgba[i * 4 + 1], rgba[i * 4 + 2],
                                                             rgba[i * 4 + 3]);
            }
            terrainTexture_.SetData(texels.data(), static_cast<int>(texels.size()));
            // The terrain's padded copy with its mip levels (TileAtlas.hpp Layout::Padded): distant
            // floors no longer shimmer into rings.
            const auto padded = terrainTileAtlas_.Padded();
            const int kPadW = padded.width;
            const int kPadH = padded.height;
            auto level = GalaxyEggbert::Game::BuildPaddedSheet(rgba.data(), width, height, terrainScale);
            // Each block type's colour for Ultra's light from around (TerrainGi.hpp): its padded tile's
            // mean (the inner tile, its border being copies of its edge).
            terrainGi_.albedo.assign(1024, Microsoft::Xna::Framework::Vector3(0.2f, 0.2f, 0.2f));
            for (int type = 1; type < static_cast<int>(terrainGi_.albedo.size()); ++type)
            {
                const auto uv = terrainTileAtlas_.GetTileUv(type);
                if (uv.u1 <= uv.u0 || uv.v1 <= uv.v0)
                {
                    continue;
                }
                terrainGi_.albedo[static_cast<std::size_t>(type)] = GalaxyEggbert::Game::TileAlbedo(
                    level.data(), kPadW, static_cast<int>(uv.u0 * kPadW), static_cast<int>(uv.v0 * kPadH),
                    static_cast<int>(uv.u1 * kPadW), static_cast<int>(uv.v1 * kPadH));
            }
            // The effects' water: each water's body, its frames' tints averaged (Water1 92-95, Water2 91,
            // 96-98).
            {
                using GalaxyEggbert::Game::TileAtlas;
                const auto tint = [&](std::initializer_list<int> icons)
                {
                    Microsoft::Xna::Framework::Vector4 sum(0.0f, 0.0f, 0.0f, 0.0f);
                    int frames = 0;
                    for (const int icon : icons)
                    {
                        const int x0 = padded.border + (icon % TileAtlas::kPaddedColumns) * padded.pitch;
                        const int y0 = padded.border + (icon / TileAtlas::kPaddedColumns) * padded.pitch;
                        const auto t = GalaxyEggbert::Game::TileTint(level.data(), kPadW, x0, y0, x0 + padded.tile,
                                                                     y0 + padded.tile);
                        if (t.W <= 0.0f) continue;
                        sum = sum + t;
                        ++frames;
                    }
                    return frames > 0 ? sum / static_cast<float>(frames) : sum;
                };
                waterBodies_ = {tint({92, 93, 94, 95}), tint({91, 96, 97, 98})};
            }
            terrainMipTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
                device, kPadW, kPadH, /*mipMap=*/true, Microsoft::Xna::Framework::Graphics::SurfaceFormat::Color);
            int levelW = kPadW, levelH = kPadH;
            const int levels = terrainMipTexture_.getLevelCountProperty();
            for (int mip = 0; mip < levels; ++mip)
            {
                std::vector<Microsoft::Xna::Framework::Color> colors(static_cast<std::size_t>(levelW) * levelH);
                for (std::size_t i = 0; i < colors.size(); ++i)
                {
                    colors[i] = Microsoft::Xna::Framework::Color(level[i * 4 + 0], level[i * 4 + 1], level[i * 4 + 2],
                                                                 level[i * 4 + 3]);
                }
                terrainMipTexture_.SetData(mip, nullptr, colors.data(), 0, static_cast<int>(colors.size()));
                if (mip + 1 < levels)
                {
                    level = GalaxyEggbert::Game::DownsampleHalf(level, levelW, levelH);
                    levelW = std::max(1, levelW / 2);
                    levelH = std::max(1, levelH / 2);
                }
            }
        }
        RebuildWorldPresentation();

        terrainEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        terrainEffect_->VertexColorEnabled = false;
        terrainEffect_->setTextureEnabledProperty(true);
        ApplyTerrainLighting(*terrainEffect_);
        terrainEffect_->setTextureProperty(&terrainMipTexture_);

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
        ApplyTerrainLighting(*grassEffect_);
        grassEffect_->setTextureProperty(&grassTexture_);

        // Real mobile-eggbert bottom HUD + the interim animation-state
        // indicator (2026-07-10, see Hud.hpp -- loads its own texture
        // instances, including text.png/pad.png which nothing else loads).
        hud_.LoadContent(device);

        // Real mobile-eggbert on-screen touch controls (2026-07-13, see
        // InputPad.hpp -- loads its own pad.png instance plus
        // Content/backgrounds/pause.png and blupiyoupie.png, which
        // nothing else loads).
        inputPad_.LoadContent(device);

        // Billboard rendering for MoveObjects (15-3d-render-mapping-design.md
        // §5/§7) — element.png from the approved mobile-eggbert assets.
        {
            int scale = 1;
            objectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
                GalaxyEggbert::Game::ScaledContentPath("Content/icons/element.png", &scale), device);
            GalaxyEggbert::Game::SetTextureScale(&objectTexture_, scale);
        }
        objectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        objectEffect_->VertexColorEnabled = false;
        objectEffect_->setTextureEnabledProperty(true);
        objectEffect_->setTextureProperty(&objectTexture_);
        std::cout << "GalaxyEggbertCNA: " << worldRuntime_.GetMobileObjects().size()
                  << " MoveObject(s) parsed for billboard rendering." << std::endl;

        // Billboard rendering for object-m.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3) -- reuses
        // terrainTexture_ (object-m.png), same reasoning as bigDecorEffect_.
        objectMPngEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        objectMPngEffect_->VertexColorEnabled = false;
        objectMPngEffect_->setTextureEnabledProperty(true);
        objectMPngEffect_->setTextureProperty(&terrainTexture_);

        // Billboard rendering for explo.png-sourced MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3) -- explo.png isn't
        // loaded anywhere else in GalaxyEggbertCNA, so this is a genuinely
        // new texture load (already copied next to this binary at build
        // time, same mechanism as element.png/object-m.png).
        {
            int scale = 1;
            exploTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
                GalaxyEggbert::Game::ScaledContentPath("Content/icons/explo.png", &scale), device);
            GalaxyEggbert::Game::SetTextureScale(&exploTexture_, scale);
        }
        exploEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        exploEffect_->VertexColorEnabled = false;
        exploEffect_->setTextureEnabledProperty(true);
        exploEffect_->setTextureProperty(&exploTexture_);

        // Billboard rendering for Blupi-skin MoveObjects
        // (ObjectDefinitionRegistry, NEXT.md §3) -- separate
        // Texture2D instances from blupiIconTexture_ above even though
        // ObjectType200 loads the same blupi.png file, since that one is
        // owned by the 2D SpriteBatch HUD path, not this 3D BasicEffect path.
        {
            int scale = 1;
            blupiObjectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
                GalaxyEggbert::Game::ScaledContentPath("Content/icons/blupi.png", &scale), device);
            GalaxyEggbert::Game::SetTextureScale(&blupiObjectTexture_, scale);
        }
        blupiObjectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        blupiObjectEffect_->VertexColorEnabled = false;
        blupiObjectEffect_->setTextureEnabledProperty(true);
        blupiObjectEffect_->setTextureProperty(&blupiObjectTexture_);

        {
            int scale = 1;
            blupi1ObjectTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
                GalaxyEggbert::Game::ScaledContentPath("Content/icons/blupi1.png", &scale), device);
            GalaxyEggbert::Game::SetTextureScale(&blupi1ObjectTexture_, scale);
        }
        blupi1ObjectEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        blupi1ObjectEffect_->VertexColorEnabled = false;
        blupi1ObjectEffect_->setTextureEnabledProperty(true);
        blupi1ObjectEffect_->setTextureProperty(&blupi1ObjectTexture_);

        // Billboard rendering for BigDecor: cells. Eggbert 2 renders this
        // layer through CHEXPLO (Decor.cpp's QuickIcon(channel=9,...)), so
        // ids such as Palmtree=16 must sample explo.png, not object-m.png
        // where icon 16 is an unrelated blue triangular mechanical tile.
        bigDecorEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
        bigDecorEffect_->VertexColorEnabled = false;
        bigDecorEffect_->setTextureEnabledProperty(true);
        bigDecorEffect_->setTextureProperty(&exploTexture_);

        std::cout << "GalaxyEggbertCNA: " << bigDecorCells_.size()
                  << " BigDecor cell(s) parsed for billboard rendering." << std::endl;

        // Platform-lift/crate UniformCube object path (NEXT.md §8 task 3) --
        // reuses terrainTexture_ (object-m.png), the confirmed-correct sheet
        // for these ObjectTypes (see ObjectDefinitionRegistry's SolidCube
        // render mode).
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
            ApplyTerrainLighting(*objectCubeEffect_);
            objectCubeEffect_->setTextureProperty(&terrainTexture_);

            int cubeObjectCount = 0;
            for (const auto& obj : worldRuntime_.GetMobileObjects())
            {
                cubeObjectCount +=
                    GetObjectDefinition(obj.type).renderMode == ObjectRenderMode::SolidCube ? 1 : 0;
            }
            std::cout << "GalaxyEggbertCNA: " << cubeObjectCount
                      << " platform-lift/crate cube object(s) found." << std::endl;
        }

        // Third-person-mode 3D model (2026-07-09, NEXT.md §3) -- CNA's
        // generic SkinnedModelEXT/SkinnedEffect path, see this class's own
        // header comment and ../cna/docs/skinned-model-ext.md.
        // ContentManager's RootDirectory is pointed at avatars3d/ (not the
        // default "Content", which is mobile-eggbert's own tree, fully
        // re-copied fresh on every build -- see the textures3d/ comment
        // above for the same reasoning) so Load<>'s asset name is relative
        // to avatars3d/ itself. Currently unused elsewhere in this class,
        // so repointing its RootDirectory here can't break anything else.
        {
            const std::string realModelManifest = "avatars3d/blupi/avatar.skinnedmodel.json";
            const std::string blupiModelManifest = "avatars3d/blupi_placeholder/avatar.skinnedmodel.json";
            blupiModelIsReal_ = std::filesystem::exists(realModelManifest);
            if (blupiModelIsReal_ || std::filesystem::exists(blupiModelManifest))
            {
                auto& content = getContentProperty();
                // Relative on every platform: on Android CNA reads the packaged assets, and since cna
                // 0b8424e9b lists each APK directory once (UB-006; the absolute root into internal
                // storage that worked round it is gone).
                content.setRootDirectoryProperty("avatars3d");
                blupiModel_ = content.Load<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>(
                    blupiModelIsReal_ ? "blupi/avatar" : "blupi_placeholder/avatar");

                blupiEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::SkinnedEffect>(device);
                blupiEffect_->setAmbientLightColorProperty(
                    Microsoft::Xna::Framework::Vector3(0.35f, 0.35f, 0.35f));
                auto& keyLight = blupiEffect_->getDirectionalLight0Property();
                keyLight.setEnabledProperty(true);
                keyLight.setDiffuseColorProperty(
                    Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f));
                keyLight.setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                keyLight.setDirectionProperty(
                    Microsoft::Xna::Framework::Vector3(-0.4f, -0.6f, -0.7f));
                blupiEffect_->getDirectionalLight1Property().setEnabledProperty(false);
                blupiEffect_->getDirectionalLight2Property().setEnabledProperty(false);
                blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                blupiEffect_->setEmissiveColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                // Keep the placeholder's established warm fallback tint.
                blupiEffect_->setDiffuseColorProperty(
                    Microsoft::Xna::Framework::Color::NavajoWhite.ToVector3());
                if (blupiModelIsReal_)
                {
                    // The real model's part textures carry its colours
                    // (yellow body with the face, black eyelashes), so no
                    // tint. Ray Dream lit the sprites with an ambient 0.5
                    // grey plus one 0.5 light and a 0.5 highlight. The chase
                    // camera mostly sees his back, which that key light
                    // leaves at ambient only, so a weaker opposite light fills
                    // it in.
                    blupiEffect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3::One);
                    blupiEffect_->setAmbientLightColorProperty(
                        Microsoft::Xna::Framework::Vector3(0.45f, 0.45f, 0.45f));
                    keyLight.setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(0.55f, 0.55f, 0.55f));
                    keyLight.setSpecularColorProperty(Microsoft::Xna::Framework::Vector3(0.3f, 0.3f, 0.3f));
                    auto& fillLight = blupiEffect_->getDirectionalLight1Property();
                    fillLight.setEnabledProperty(true);
                    fillLight.setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(0.35f, 0.35f, 0.35f));
                    fillLight.setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::Zero);
                    fillLight.setDirectionProperty(Microsoft::Xna::Framework::Vector3(0.4f, -0.6f, 0.7f));
                    blupiEffect_->setSpecularColorProperty(Microsoft::Xna::Framework::Vector3::One);
                    blupiEffect_->setSpecularPowerProperty(24.0f);
                }
                if (blupiModelIsReal_)
                {
                    // Expressions per clip frame and the clips facing the
                    // viewer (see blupiFaces_' declaration).
                    std::ifstream clipsFile("avatars3d/blupi/clips.txt");
                    std::string line;
                    while (std::getline(clipsFile, line))
                    {
                        std::istringstream in(line);
                        std::string kind;
                        in >> kind;
                        if (kind == "rate")
                        {
                            in >> blupiFaceRate_;
                        }
                        else if (kind == "part")
                        {
                            in >> blupiFacePart_;
                        }
                        else if (kind == "face")
                        {
                            std::string file;
                            in >> file;
                            blupiFaces_.emplace_back("avatars3d/blupi/" + file, device);
                        }
                        else if (kind == "clip")
                        {
                            std::string name;
                            in >> name;
                            std::vector<int> frames;
                            for (int face = 0; in >> face;)
                            {
                                frames.push_back(face);
                            }
                            blupiClipFaces_[name] = std::move(frames);
                        }
                        else if (kind == "viewer")
                        {
                            std::string name;
                            in >> name;
                            blupiViewerClips_.insert(name);
                        }
                        else if (kind == "vehicle")
                        {
                            std::string name;
                            in >> name;
                            blupiVehicles_.emplace_back(
                                name,
                                content.Load<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>(
                                    "blupi/vehicles/" + name + "/avatar"));
                        }
                    }

                    // The objects converted to 3D models (objectModelTable_):
                    // lit like him, so only with the real model.
                    std::ifstream objectIndex("avatars3d/objects/index.txt");
                    for (std::string name; std::getline(objectIndex, name);)
                    {
                        if (name.empty() || name[0] == '#')
                        {
                            continue;
                        }
                        std::ifstream icons("avatars3d/objects/" + name + "/icons.txt");
                        std::string error;
                        if (!objectModelTable_.AddModel(static_cast<int>(objectModels_.size()), icons, &error))
                        {
                            std::cerr << "GalaxyEggbertCNA: avatars3d/objects/" << name
                                      << "/icons.txt: bad line '" << error << "'" << std::endl;
                        }
                        objectModels_.push_back(
                            content.Load<std::shared_ptr<Microsoft::Xna::Framework::Graphics::SkinnedModelEXT>>(
                                "objects/" + name + "/avatar"));
                        objectModelRadii_.push_back(ModelFileReach("avatars3d/objects/" + name));
                        // Its parts' Ray Dream materials (tools/rd4_to_gltf/procedural_materials.py), for the
                        // 3D effects: one per part, empty when it has none.
                        {
                            std::vector<GalaxyEggbert::Game::ProceduralMaterial> parts;
                            std::ifstream file("avatars3d/objects/" + name + "/materials.txt");
                            if (file)
                            {
                                const auto materials = GalaxyEggbert::Game::ReadProceduralMaterials(file);
                                const auto& model = *objectModels_.back();
                                parts.resize(model.Parts.size());
                                bool any = false;
                                for (std::size_t i = 0; i < model.Parts.size(); ++i)
                                {
                                    if (const auto* material =
                                            GalaxyEggbert::Game::FindPartMaterial(materials, model.Parts[i].Name))
                                    {
                                        parts[i] = *material;
                                        any = true;
                                    }
                                }
                                if (!any)
                                {
                                    parts.clear();
                                }
                            }
                            objectPartMaterials_.push_back(std::move(parts));
                        }
                        // The solid BigDecor's shapes (Q-014), as this model draws their icons.
                        for (const auto icon : GalaxyEggbert::kSolidBigDecorIcons)
                        {
                            const ObjectModelPose* pose = objectModelTable_.Find(ObjectTextureSource::Explo, icon);
                            if (pose != nullptr && pose->model == static_cast<int>(objectModels_.size()) - 1)
                            {
                                const float halfBlocks =
                                    static_cast<float>(pose->iconPixels > 0 ? pose->iconPixels : 128) / 128.0f;
                                bigDecorShapes_[icon] = ModelFileShape("avatars3d/objects/" + name, *pose, halfBlocks);
                            }
                        }
                        // The electrode's sparks (Q-005; tools/rd4_to_gltf/electro_sparks.py).
                        for (int icon = 238; icon <= 242; ++icon)
                        {
                            const std::string spark =
                                "avatars3d/objects/" + name + "/sparks/" + std::to_string(icon) + ".png";
                            if (std::filesystem::exists(spark))
                            {
                                electrodeSparks_[icon] =
                                    std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(spark, device);
                            }
                        }
                        if (name == "dynamite_dynamite")
                        {
                            for (int icon = 253; icon <= 255; ++icon)
                            {
                                const std::string spark =
                                    "avatars3d/objects/" + name + "/sparks/" + std::to_string(icon) + ".png";
                                if (std::filesystem::exists(spark))
                                {
                                    dynamiteSparks_[icon] =
                                        std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(spark, device);
                                }
                            }
                        }
                        // A sign's painted face (sign_decals.py): decal <icon> <png> <x0> <y0> <x1> <y1> <z>.
                        std::ifstream decals("avatars3d/objects/" + name + "/decals.txt");
                        for (std::string line; std::getline(decals, line);)
                        {
                            std::istringstream fields(line);
                            std::string word, png;
                            int icon = 0;
                            float x0 = 0, y0 = 0, x1 = 0, y1 = 0, z = 0;
                            if (!(fields >> word >> icon >> png >> x0 >> y0 >> x1 >> y1 >> z) || word != "decal")
                            {
                                continue;
                            }
                            const std::string path = "avatars3d/objects/" + name + "/" + png;
                            if (!std::filesystem::exists(path))
                            {
                                continue;
                            }
                            SignDecal decal;
                            decal.texture = std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(path, device);
                            // Model space: 64 sprite pixels a block, the icon's centre the origin, +Y up.
                            decal.left = (x0 - 32.0f) / 64.0f;
                            decal.right = (x1 - 32.0f) / 64.0f;
                            decal.top = (32.0f - y0) / 64.0f;
                            decal.bottom = (32.0f - y1) / 64.0f;
                            decal.depth = z;
                            signDecals_[icon] = std::move(decal);
                        }
                    }
                    std::cout << "GalaxyEggbertCNA: " << objectModels_.size() << " object model(s) draw "
                              << objectModelTable_.Size() << " sprite icon(s) in 3D." << std::endl;
                    ApplyBlockModels();
                    // A terrain built before the models came leaves the still model icons in its
                    // mesh: build it again (the next frame).
                    --terrainRevision_;
                    BuildEnemyBlupiTextures(device);
                }
                blupiModelLoaded_ = true;
                std::cout << "GalaxyEggbertCNA: third-person model loaded — "
                          << (blupiModelIsReal_ ? "avatars3d/blupi/" : "avatars3d/blupi_placeholder/")
                          << " (F3 toggles third-/first-person)." << std::endl;
            }
            else
            {
                cameraMode_ = CameraMode::FirstPerson;
                std::cout << "GalaxyEggbertCNA: no third-person model found ("
                          << blupiModelManifest
                          << " not found) — third-person camera mode unavailable, staying first-person."
                          << std::endl;
            }
        }

        // Explicit .vwr start cells use raw-grid coordinates and are shifted
        // by WorldRuntime for rendering. Old worlds retain the historical
        // fixed sample-world default.
        if (worldRuntime_.HasExplicitSpawnPoint())
        {
            PlaceBlupi(
                worldRuntime_.GetSpawnRenderX(), worldRuntime_.GetSpawnRenderY(),
                worldRuntime_.GetSpawnRenderZ());
            blupi_.SetYaw(worldRuntime_.GetSpawnYaw());
        }
        else
        {
            PlaceBlupi(0.0f, 1.0f, 0.0f);
        }

        // Real mobile-eggbert sound playback (2026-07-10, see Sound.hpp) --
        // loads whichever of Content/sounds/sound000.wav..sound092.wav
        // actually exist (already copied next to the binary by the
        // mobile-eggbert Content/ POST_BUILD step, same as icons/
        // backgrounds).
        sound_.LoadContent();
        std::cout << "GalaxyEggbertCNA: sound loaded — " << sound_.LoadedCount() << "/"
                  << Sound::kNumChannels << " channel(s)." << std::endl;

        // Minimal settings persistence (2026-07-13, plan.md MENU-067, see
        // SaveData.hpp) -- restores the one real, already-wired setting
        // (sound on/off) from the previous run, if any.
        sound_.SetEnabled(saveData_.GetSoundEnabled());
        sound_.SetEffectsVolume(static_cast<float>(saveData_.GetEffectsVolume()) / 100.0f);
        inputPad_.SetJumpRight(saveData_.GetJumpRight());

        // Real Wait->Resume-or-Init branch (plan.md MENU-001..020) now
        // decides this at the END of the real 5.0s Wait timer, in
        // Update(), not here at load time -- see phase_'s own default
        // member initializer (now Wait, not Play) and GalaxyEggbert::Def::GamePhase's own
        // class comment for the full reasoning.

        std::cout << "GalaxyEggbertCNA: terrain mesh uploaded — "
                  << terrainRenderer_->BlockCount() << " blocks ("
                  << terrainRenderer_->AnimatedBlockCount() << " animated, "
                  << terrainRenderer_->WaterBlockCount() << " water/semi-transparent), "
                  << terrainRenderer_->VertexCount() << " vertices, "
                  << terrainRenderer_->PrimitiveCount() << " triangles." << std::endl;
    }

    void GalaxyEggbertGame::RebuildWorldPresentation(bool editorEdit)
    {
        xraySecretRevision_.reset(); // including a new world with the same block revision
        xraySecretBoxes_.clear();
        const auto editStarted = std::chrono::steady_clock::now();
        // Re-derive skyRegion/missionNumber/mobileObjects_ from the World
        // itself first (plan.md EDITOR-109): the world editor mutates the
        // live World in place, with no disk round-trip, so LoadFromVwrFile()
        // -- which used to be the only thing that populated these -- never
        // runs for an in-editor edit. Without this, an object placed in the
        // editor stays invisible until the world is saved and reloaded.
        // A no-op for every other caller, whose World hasn't changed since
        // it was loaded.
        worldRuntime_.ResyncFromWorld();
        bigDecorCells_.clear();
        for (const auto& cell : worldRuntime_.GetBigDecorCells())
        {
            bigDecorCells_.push_back({
                cell.worldX, cell.worldY, cell.worldZ, cell.icon});
        }
        worldRuntime_.SetBigDecorShapes(bigDecorShapes_);
        blupi_.SetSolidBoxes(worldRuntime_.SolidBigDecorBoxes());

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
        // An editor's edit keeps the picture of the same region (decoding it and Ultra's sky light took
        // most of an edit's time).
        const bool sameBackground = editorEdit && backgroundRegion_ == static_cast<int>(worldRuntime_.GetSkyRegion());
        backgroundRegion_ = static_cast<int>(worldRuntime_.GetSkyRegion());
        if (!sameBackground)
        {
            char backgroundPath[64];
            std::snprintf(backgroundPath, sizeof(backgroundPath),
                          "Content/backgrounds/decor%03u.png", worldRuntime_.GetSkyRegion());
            if (std::filesystem::exists(backgroundPath))
            {
                int backgroundScale = 1;
                backgroundTexture_ = Microsoft::Xna::Framework::Graphics::Texture2D(
                    GalaxyEggbert::Game::ScaledContentPath(backgroundPath, &backgroundScale), device);
                GalaxyEggbert::Game::SetTextureScale(&backgroundTexture_, backgroundScale);
                backgroundEffect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
                backgroundEffect_->VertexColorEnabled = false;
                backgroundEffect_->setTextureEnabledProperty(true);
                backgroundEffect_->setTextureProperty(&backgroundTexture_);
                backgroundLoaded_ = true;
                {
                    // The 3D effects' distance haze takes the picture's own colour: the average of its
                    // middle rows, where its horizon is.
                    const int width = backgroundTexture_.getWidthProperty();
                    const int height = backgroundTexture_.getHeightProperty();
                    std::vector<Microsoft::Xna::Framework::Color> texels(static_cast<std::size_t>(width) *
                                                                         static_cast<std::size_t>(height));
                    backgroundTexture_.GetData(texels.data(), static_cast<int>(texels.size()));
                    double r = 0.0, g = 0.0, b = 0.0;
                    long count = 0;
                    for (int y = height * 35 / 100; y < height * 75 / 100; ++y)
                    {
                        for (int x = 0; x < width; ++x)
                        {
                            const auto& c = texels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                                   static_cast<std::size_t>(x)];
                            r += c.getRProperty();
                            g += c.getGProperty();
                            b += c.getBProperty();
                            ++count;
                        }
                    }
                    if (count > 0)
                    {
                        backgroundHaze_ = Microsoft::Xna::Framework::Vector3(
                            static_cast<float>(r / count / 255.0), static_cast<float>(g / count / 255.0),
                            static_cast<float>(b / count / 255.0));
                    }
                    // Ultra's sky light: the same picture.
                    std::vector<std::uint8_t> rgba(texels.size() * 4);
                    for (std::size_t i = 0; i < texels.size(); ++i)
                    {
                        rgba[i * 4] = texels[i].getRProperty();
                        rgba[i * 4 + 1] = texels[i].getGProperty();
                        rgba[i * 4 + 2] = texels[i].getBProperty();
                        rgba[i * 4 + 3] = 255;
                    }
                    BuildUltraSky(device, rgba.data(), width, height);
                }
                std::cout << "GalaxyEggbertCNA: background loaded — " << backgroundPath << " ("
                          << backgroundTexture_.getWidthProperty() << "x"
                          << backgroundTexture_.getHeightProperty() << " px, region "
                          << worldRuntime_.GetSkyRegion() << ")." << std::endl;
            }
            else
            {
                backgroundLoaded_ = false;
                BuildUltraSky(device, nullptr, 0, 0);
                std::cout << "GalaxyEggbertCNA: no background image for region "
                          << worldRuntime_.GetSkyRegion() << " (" << backgroundPath
                          << " not found) — falling back to flat clear color." << std::endl;
            }
        }

        if (editorEdit && terrainRenderer_ && terrainWorld_ == &worldRuntime_.GetWorld())
        {
            // The same World edited: its changes patched or the chunks they reach rebuilt, as in play
            // (a full build when the history is unknown: a turned plate, more than 256 cells at once).
            RefreshTerrainMeshIfChanged();
            if (perfEnabled_)
            {
                std::cout << "GalaxyEggbertCNA perf: editor edit presented in "
                          << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - editStarted).count()
                          << " ms" << std::endl;
            }
            return;
        }
        terrainDrawSecret_ = TerrainDrawSecret();
        terrainRenderer_ = std::make_unique<TerrainRenderer>(device, worldRuntime_.GetWorld(), terrainTileAtlas_, tileCoverage_,
                                                             terrainDrawSecret_, StillModelIcons(), TerrainReliefs(),
                                                             TerrainOcclusionWanted(), TerrainGiWanted());
        terrainWorld_ = &worldRuntime_.GetWorld();
        terrainRenderer_->SetReliefOverhangs(terrainReliefOverhangs_);
        terrainReliefWarm_ = false;
        ApplyBlockModels();
        terrainRevision_ = worldRuntime_.GetWorld().blockRevision();
        std::cout << "GalaxyEggbertCNA: world terrain built — " << terrainRenderer_->BlockCount() << " blocks, "
                  << terrainRenderer_->VertexCount() << " vertices";
        if (terrainRenderer_->RaggedBrickCount() > 0)
            std::cout << ", " << terrainRenderer_->RaggedBrickCount() << " brick end(s) ragged";
        std::cout << ", " << terrainRenderer_->StaticChunkCount() << " chunk(s)";
        std::cout << "." << std::endl;
        if (perfEnabled_)
        {
            std::cout << "GalaxyEggbertCNA perf: presentation built in "
                      << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - editStarted).count()
                      << " ms" << (editorEdit ? " (an editor edit)" : "") << std::endl;
        }
    }

    void GalaxyEggbertGame::RefreshTerrainMeshIfChanged()
    {
        if (replayingResume_) return;
        // Patch changes confined to non-occluding models, else rebuild the chunks the changes reach.
        // Missing journal history, model availability changes and ShowSecret rebuild the terrain.
        if (terrainRenderer_ && (worldRuntime_.GetWorld().blockRevision() != terrainRevision_ ||
                                 TerrainDrawSecret() != terrainDrawSecret_))
        {
            const auto rebuildStart = std::chrono::steady_clock::now();
            const auto& world = worldRuntime_.GetWorld();
            const auto changes = world.blockChangesSince(terrainRevision_);
            if (TerrainDrawSecret() == terrainDrawSecret_ &&
                terrainRenderer_->PatchModelCells(world, changes, StillModelIcons(), terrainDrawSecret_, tileCoverage_))
            {
                terrainRevision_ = world.blockRevision();
                if (perfEnabled_)
                {
                    std::cout << "GalaxyEggbertCNA perf: terrain models patched " << changes.cells.size() << " changes in "
                              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - rebuildStart).count()
                              << " ms" << std::endl;
                }
                return;
            }
            // Else the chunks the changes reach (a wall blown away, a saw cut): a few of the world's.
            if (terrainRenderer_->RebuildChangedChunks(getGraphicsDeviceProperty(), world, changes, StillModelIcons(),
                                                       TerrainDrawSecret(), tileCoverage_))
            {
                terrainRevision_ = world.blockRevision();
                if (perfEnabled_)
                {
                    std::cout << "GalaxyEggbertCNA perf: terrain chunks rebuilt for " << changes.cells.size()
                              << " changes in "
                              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - rebuildStart).count()
                              << " ms" << std::endl;
                }
                return;
            }
            if (perfEnabled_)
            {
                std::cout << "GalaxyEggbertCNA perf: terrain full rebuild causes: known=" << changes.known
                          << " secretChanged=" << (TerrainDrawSecret() != terrainDrawSecret_) << std::endl;
                for (const auto& c : changes.cells)
                    std::cout << "  terrain change (" << c.x << ',' << c.y << ',' << c.z << ") "
                              << Worlds::Block(c.oldRaw).type() << "->" << Worlds::Block(c.newRaw).type()
                              << " raw " << c.oldRaw << "->" << c.newRaw << std::endl;
            }
            terrainDrawSecret_ = TerrainDrawSecret();
            auto rebuilt =
                std::make_unique<TerrainRenderer>(getGraphicsDeviceProperty(), worldRuntime_.GetWorld(), terrainTileAtlas_,
                                                  tileCoverage_, terrainDrawSecret_, StillModelIcons(), TerrainReliefs(),
                                                             TerrainOcclusionWanted(), TerrainGiWanted());
            rebuilt->SetReliefOverhangs(terrainReliefOverhangs_);
            // The unchanged chunks keep their reliefs; the changed ones are built in the background.
            rebuilt->AdoptReliefs(*terrainRenderer_);
            terrainRenderer_ = std::move(rebuilt);
            ApplyBlockModels();
            terrainRevision_ = worldRuntime_.GetWorld().blockRevision();
            if (perfEnabled_)
            {
                std::cout << "GalaxyEggbertCNA perf: terrain rebuilt in "
                          << std::chrono::duration<double>(std::chrono::steady_clock::now() - rebuildStart).count() * 1000.0
                          << " ms" << std::endl;
            }
        }
    }

    const GalaxyEggbert::Game::TerrainReliefTable* GalaxyEggbertGame::TerrainReliefs()
    {
        if (!terrainReliefsLoaded_)
        {
            terrainReliefsLoaded_ = true;
            if (TerrainReliefWanted())
            {
                const char* dir = std::getenv("GALAXY_TERRAIN_RELIEF_DIR");
                const int icons = terrainReliefs_.Load(dir != nullptr && *dir != '\0' ? dir : "avatars3d/terrain");
                std::cout << "GalaxyEggbertCNA: " << icons << " terrain tile(s) drawn as their relief near the camera."
                          << std::endl;
            }
            if (const char* radius = std::getenv("GALAXY_TERRAIN_RELIEF_RADIUS"))
            {
                terrainReliefRadius_ = std::max(0.0f, static_cast<float>(std::atof(radius)));
            }
            const char* overhang = std::getenv("GALAXY_TERRAIN_RELIEF_OVERHANG");
            terrainReliefOverhangs_ = overhang != nullptr && std::string(overhang) == "1";
        }
        return terrainReliefs_.Empty() ? nullptr : &terrainReliefs_;
    }

    void GalaxyEggbertGame::BuildUltraSky(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                          const std::uint8_t* rgba, int width, int height)
    {
        // The 3D effects' Ultra level lights the world with its background picture (UltraLogic.hpp): its
        // levels for the reflections, its irradiance for the light from around. Computed with every world
        // (small: 128 x 96 and its levels), so that the Setup can turn Ultra on at once; its texture made on
        // Ultra's first frame (UltraSceneAssetsForFrame), so that the other levels allocate nothing more --
        // a texture made here moved a High golden frame by 45 pixels.
        (void) device;
        ultraSkyData_ = GalaxyEggbert::Game::BuildSkyEnvironment(rgba, width, height, 4.0f, backgroundHaze_);
        ultraSky_.reset();
        ultraAssets_.environment = nullptr;
        ultraAssets_.irradiance = ultraSkyData_.irradiance;
    }

    const GalaxyEggbert::Game::UltraSceneAssets* GalaxyEggbertGame::UltraSceneAssetsForFrame(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        if (!sceneFrame_.effects.ultra || ultraSkyData_.levels.empty())
        {
            return nullptr;
        }
        if (!ultraSky_)
        {
            UploadUltraSky(device);
        }
        if (!ultraTerrainMapsTried_)
        {
            ultraTerrainMapsTried_ = true;
            BuildUltraTerrainMaps(device);
        }
        return &ultraAssets_;
    }

    void GalaxyEggbertGame::BuildUltraTerrainMaps(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        // Ultra's terrain atlas (TerrainMaps.hpp): finer (twice, as near as HiDef's 4096 allows), its normals
        // and its surface, from the
        // padded atlas's own texels, each with its levels. Once, on Ultra's first frame (a second or so).
        const auto started = std::chrono::steady_clock::now();
        const int width = terrainMipTexture_.getWidthProperty();
        const int height = terrainMipTexture_.getHeightProperty();
        if (width <= 0 || height <= 0)
        {
            return;
        }
        std::vector<Microsoft::Xna::Framework::Color> texels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
        terrainMipTexture_.GetData(0, nullptr, texels.data(), 0, static_cast<int>(texels.size()));
        std::vector<std::uint8_t> rgba(texels.size() * 4);
        for (std::size_t i = 0; i < texels.size(); ++i)
        {
            rgba[i * 4] = texels[i].getRProperty();
            rgba[i * 4 + 1] = texels[i].getGProperty();
            rgba[i * 4 + 2] = texels[i].getBProperty();
            rgba[i * 4 + 3] = texels[i].getAProperty();
        }
        const auto upload = [&](std::vector<std::uint8_t> level, int w, int h)
        {
            auto texture = std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(
                device, w, h, /*mipMap=*/true, Microsoft::Xna::Framework::Graphics::SurfaceFormat::Color);
            const int levels = texture->getLevelCountProperty();
            for (int mip = 0; mip < levels; ++mip)
            {
                std::vector<Microsoft::Xna::Framework::Color> colors(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
                for (std::size_t i = 0; i < colors.size(); ++i)
                {
                    colors[i] = Microsoft::Xna::Framework::Color(static_cast<int>(level[i * 4]), static_cast<int>(level[i * 4 + 1]),
                                                                 static_cast<int>(level[i * 4 + 2]), static_cast<int>(level[i * 4 + 3]));
                }
                texture->SetData(mip, nullptr, colors.data(), 0, static_cast<int>(colors.size()));
                if (mip + 1 < levels)
                {
                    level = GalaxyEggbert::Game::DownsampleHalf(level, w, h);
                    w = std::max(1, w / 2);
                    h = std::max(1, h / 2);
                }
            }
            return texture;
        };
        // A 2x atlas (TextureScale.hpp): its slopes per texel are half as steep, so the relief's strength
        // doubles to stay the same per tile; and it is already finer, so it is not upscaled again.
        const int atlasScale = terrainTileAtlas_.Scale();
        auto maps = GalaxyEggbert::Game::BuildSurfaceMaps(rgba.data(), width, height, terrainTileAtlas_.Padded().pitch,
                                                          3.0f * static_cast<float>(atlasScale));
        ultraTerrainNormals_ = upload(std::move(maps.normals), width, height);
        ultraTerrainSurface_ = upload(std::move(maps.surface), width, height);
        const auto fine = atlasScale > 1 ? std::array<int, 2>{width, height}
                                         : GalaxyEggbert::Game::UpscaledSize(width, height);
        if (atlasScale == 1)
        {
            ultraTerrainAlbedo_ =
                upload(GalaxyEggbert::Game::Upscale(rgba.data(), width, height, fine[0], fine[1]), fine[0], fine[1]);
        }
        ultraAssets_.terrainNormals = ultraTerrainNormals_.get();
        ultraAssets_.terrainSurface = ultraTerrainSurface_.get();
        ultraAssets_.terrainAlbedo = ultraTerrainAlbedo_.get(); // nullptr at 2x: the atlas as it is
        const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
        std::cout << "GalaxyEggbertCNA: Ultra's terrain maps made (" << fine[0] << "x" << fine[1] << " colours, "
                  << width << "x" << height << " normals and surface) in " << ms << " ms." << std::endl;
    }

    void GalaxyEggbertGame::UploadUltraSky(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        const auto& sky = ultraSkyData_;
        ultraSky_ = std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(
            device, sky.width, sky.height, /*mipMap=*/true, Microsoft::Xna::Framework::Graphics::SurfaceFormat::Color);
        const int levels = std::min(ultraSky_->getLevelCountProperty(), static_cast<int>(sky.levels.size()));
        for (int level = 0; level < levels; ++level)
        {
            const auto& data = sky.levels[static_cast<std::size_t>(level)];
            std::vector<Microsoft::Xna::Framework::Color> colors(data.size() / 4);
            for (std::size_t i = 0; i < colors.size(); ++i)
            {
                colors[i] = Microsoft::Xna::Framework::Color(static_cast<int>(data[i * 4]), static_cast<int>(data[i * 4 + 1]),
                                                             static_cast<int>(data[i * 4 + 2]), 255);
            }
            ultraSky_->SetData(level, nullptr, colors.data(), 0, static_cast<int>(colors.size()));
        }
        ultraAssets_.environment = ultraSky_.get();
        ultraAssets_.environmentLevels = levels;
    }

    bool GalaxyEggbertGame::Effects3DWanted() const
    {
        return EffectsLevel() > 0;
    }

    int GalaxyEggbertGame::EffectsLevel() const
    {
        // GALAXY_EFFECTS overrides the Setup's "3D effects" (tests, comparisons): 0-4 or off, low,
        // medium, high, ultra.
        if (const char* level = std::getenv("GALAXY_EFFECTS"); level != nullptr && *level != '\0')
        {
            const std::string text(level);
            if (text == "off") return 0;
            if (text == "low") return 1;
            if (text == "medium") return 2;
            if (text == "high") return 3;
            if (text == "ultra") return 4;
            return std::clamp(std::atoi(level), 0, GalaxyEggbert::Game::SaveData::kEffectsLevelUltra);
        }
        return saveData_.GetEffectsLevel();
    }


    const GalaxyEggbert::Game::TerrainGiSettings* GalaxyEggbertGame::TerrainGiWanted()
    {
        // Ultra's light from around, baked with the occlusion (TerrainGi.hpp): where its shaders draw it.
        if (EffectsLevel() < GalaxyEggbert::Game::SaveData::kEffectsLevelUltra || terrainGi_.albedo.empty() ||
            !TerrainOcclusionWanted())
        {
            return nullptr;
        }
        terrainGi_.sunDirection = GalaxyEggbert::Game::SceneShaders::SunDirection();
        return &terrainGi_;
    }

    bool GalaxyEggbertGame::TerrainOcclusionWanted()
    {
        // The 3D effects' ambient occlusion is baked into the terrain only where their terrain shader
        // draws it (the stock effect cannot).
        return EffectsLevel() >= 2 && worldRenderer_.TerrainShadersReady(getGraphicsDeviceProperty());
    }

    bool GalaxyEggbertGame::TerrainReliefWanted() const
    {
        // GALAXY_TERRAIN_RELIEF=0 or =1 overrides the Setup's "3D terrain" (tests, measurements).
        if (const char* on = std::getenv("GALAXY_TERRAIN_RELIEF"); on != nullptr && *on != '\0')
        {
            return std::string(on) != "0";
        }
        return saveData_.GetTerrainRelief();
    }

    void GalaxyEggbertGame::ApplyTerrainReliefSetting()
    {
        // The table loaded or dropped (its memory with it), and the terrain built again, when a
        // world is up. The old renderer goes first: its background relief builds read the table
        // (destroying it waits for them).
        const bool hadRenderer = terrainRenderer_ != nullptr;
        terrainRenderer_.reset();
        terrainReliefs_ = GalaxyEggbert::Game::TerrainReliefTable{};
        terrainReliefsLoaded_ = false;
        if (hadRenderer)
        {
            terrainRenderer_ = std::make_unique<TerrainRenderer>(getGraphicsDeviceProperty(), worldRuntime_.GetWorld(),
                                                                 terrainTileAtlas_, tileCoverage_, terrainDrawSecret_,
                                                                 StillModelIcons(), TerrainReliefs(),
                                                             TerrainOcclusionWanted(), TerrainGiWanted());
            terrainRenderer_->SetReliefOverhangs(terrainReliefOverhangs_);
            terrainReliefWarm_ = false;
            ApplyBlockModels();
            terrainRevision_ = worldRuntime_.GetWorld().blockRevision();
        }
    }

    std::vector<std::uint16_t> GalaxyEggbertGame::StillModelIcons() const
    {
        // The still terrain icons with a loaded model (BlockModels.hpp), drawn as it.
        std::vector<std::uint16_t> icons;
        std::vector<std::uint16_t> candidates(GalaxyEggbert::Game::kStillModelIcons.begin(),
                                              GalaxyEggbert::Game::kStillModelIcons.end());
        candidates.insert(candidates.end(), GalaxyEggbert::Game::kSignModelIcons.begin(),
                          GalaxyEggbert::Game::kSignModelIcons.end());
        for (const auto icon : candidates)
        {
            if (objectModelTable_.Find(ObjectTextureSource::ObjectM, icon) != nullptr)
            {
                icons.push_back(icon);
            }
        }
        return icons;
    }

    void GalaxyEggbertGame::ApplyBlockModels()
    {
        // The fan cells as the original's fan model (BlockModels.hpp, VIS-0127) when it is loaded;
        // without it (no avatars3d/objects) they stay textured cubes.
        if (!terrainRenderer_)
        {
            return;
        }
        std::vector<std::uint16_t> bases;
        if (objectModelTable_.Find(ObjectTextureSource::ObjectM, GalaxyEggbert::Game::kFanModelIcon) != nullptr)
        {
            bases.insert(bases.end(), GalaxyEggbert::Game::kFanModelBases.begin(),
                         GalaxyEggbert::Game::kFanModelBases.end());
        }
        // Animated tiles with their scene's model (BlockModels.hpp kAnimatedModelBases).
        for (const auto base : GalaxyEggbert::Game::kAnimatedModelBases)
        {
            if (objectModelTable_.Find(ObjectTextureSource::ObjectM, base) != nullptr)
            {
                bases.push_back(base);
            }
        }
        if (!bases.empty())
        {
            terrainRenderer_->SetModelDrawnBases(std::move(bases));
        }
        if (objectModelTable_.Find(ObjectTextureSource::ObjectM, GalaxyEggbert::BlockTypes::Saw) != nullptr)
        {
            terrainRenderer_->SetSawModelBases(std::vector<std::uint16_t>(
                GalaxyEggbert::Game::kSawModelBases.begin(), GalaxyEggbert::Game::kSawModelBases.end()));
        }
    }

    bool GalaxyEggbertGame::TerrainDrawSecret() const noexcept
    {
        return cheatShowSecret_ || phase_ == GalaxyEggbert::Def::GamePhase::Editor;
    }

    void GalaxyEggbertGame::BuildEnemyBlupiTextures(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        using Microsoft::Xna::Framework::Color;
        using Microsoft::Xna::Framework::Graphics::Texture2D;
        enemyBlupiTextures_.clear();
        for (auto& textures : editorStartTextures_) textures.clear();
        if (!blupiModelIsReal_ || !blupiModel_)
        {
            return;
        }
        const auto add = [&](const Texture2D* source)
        {
            if (source == nullptr || enemyBlupiTextures_.count(source) != 0)
            {
                return;
            }
            const int width = source->getWidthProperty();
            const int height = source->getHeightProperty();
            if (width <= 0 || height <= 0)
            {
                return;
            }
            std::vector<Color> texels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
            source->GetData(texels.data(), static_cast<int>(texels.size()));
            for (auto& texel : texels)
            {
                std::uint8_t r = texel.getRProperty();
                std::uint8_t g = texel.getGProperty();
                std::uint8_t b = texel.getBProperty();
                GalaxyEggbert::Game::RecolorEnemyBlupi(r, g, b);
                texel = Color(r, g, b, texel.getAProperty());
            }
            auto copy = std::make_unique<Texture2D>(device, width, height);
            copy->SetData(texels.data(), static_cast<int>(texels.size()));
            enemyBlupiTextures_.emplace(source, std::move(copy));
        };
        for (const auto& part : blupiModel_->Parts)
        {
            add(part.Texture);
        }
        for (const auto& face : blupiFaces_)
        {
            add(&face);
        }
        for (const auto& [name, vehicle] : blupiVehicles_)
        {
            if (name == "helico" || name == "tank")
            {
                for (const auto& part : vehicle->Parts)
                {
                    add(part.Texture);
                }
            }
        }
    }

}
