#include "GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr int kMobileTileSize = 64;
        constexpr int kDecorGridSize = 100;

        // Same ObjectType allowlist as GESimple3D::GEWorldRuntime's
        // already-approved MoveObject parser — the 29 real, in-use types
        // confirmed against all 78 mobile-eggbert level files (see
        // mobile-eggbert-reference/01-world-file-format.md).
        bool IsSupportedMoveObjectType(int type)
        {
            return type == 1  || type == 2  || type == 3  || type == 4  || type == 5  ||
                   type == 6  || type == 7  || type == 12 || type == 13 || type == 16 ||
                   type == 17 || type == 19 || type == 20 || type == 21 || type == 24 ||
                   type == 25 || type == 26 || type == 30 || type == 32 || type == 33 ||
                   type == 40 || type == 44 || type == 46 || type == 47 || type == 49 ||
                   type == 50 || type == 51 || type == 54 || type == 55 || type == 96;
        }

        // 32 (blupih), 44 (wasp), 54 (large creature) patrol posStart<->posEnd
        // the same way as the other patrol enemies (Decor.cpp
        // MoveObjectStepIcon keys their turn/walk icon off posStart vs
        // posEnd, i.e. they are patrol-line objects too) — mirrors
        // GESimple3D::GEWorldRuntime's isPatrol logic.
        bool IsPatrolMoveObjectType(int type)
        {
            return type == 2 || type == 3 || type == 4 || type == 20 || type == 32 ||
                   type == 33 || type == 44 || type == 54;
        }
    }

    GEWorldRuntime::GEWorldRuntime()
        : world_(std::make_unique<Worlds::World>())
    {
    }

    bool GEWorldRuntime::LoadFromMobileEggbertFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file)
        {
            return false;
        }

        world_ = std::make_unique<Worlds::World>();
        skyRegion_ = 0;

        std::string header;
        if (!std::getline(file, header))
        {
            return false;
        }

        int blupiPixelX = 0;
        int blupiPixelY = 0;
        if (const char* bp = std::strstr(header.c_str(), "blupiPos="))
        {
            std::sscanf(bp, "blupiPos=%d;%d", &blupiPixelX, &blupiPixelY);
        }
        if (const char* rg = std::strstr(header.c_str(), "region="))
        {
            std::sscanf(rg, "region=%d", &skyRegion_);
        }

        spawnTileX_ = blupiPixelX / kMobileTileSize;
        spawnTileZ_ = blupiPixelY / kMobileTileSize;

        std::string line;
        // BigDecor: is a distinct section from Decor: (see
        // mobile-eggbert-2d-reference.md §2.3) — must be checked as its own
        // prefix, not folded into the Decor: row counter, or its 100 rows
        // are silently skipped once decorRow already reached 100 from the
        // main grid.
        enum class Section { None, Decor, BigDecor };
        Section section = Section::None;
        int decorRow = 0;
        int bigDecorRow = 0;
        bigDecor_.assign(static_cast<std::size_t>(kDecorGridSize) * kDecorGridSize, BlockTypes::Air);
        mobileObjects_.clear();

        while (std::getline(file, line))
        {
            if (line.rfind("BigDecor:", 0) == 0)
            {
                section = Section::BigDecor;
                continue;
            }
            if (line.rfind("Decor:", 0) == 0)
            {
                section = Section::Decor;
                continue;
            }
            if (line.rfind("MoveObject:", 0) == 0)
            {
                section = Section::None;

                int type = 0, psx = 0, psy = 0, pex = 0, pey = 0, stepAdv = 1;
                int stepRec = 1, stopStart = 0, stopEnd = 0;
                std::sscanf(line.c_str(),
                    "MoveObject: type=%d stepAdvance=%d stepRecede=%d timeStopStart=%d timeStopEnd=%d "
                    "posStart=%d;%d posEnd=%d;%d",
                    &type, &stepAdv, &stepRec, &stopStart, &stopEnd, &psx, &psy, &pex, &pey);

                if (!IsSupportedMoveObjectType(type))
                {
                    continue;
                }

                MobileObjSpec spec;
                spec.type = static_cast<ObjectType>(type);
                spec.posStartX = static_cast<float>(psx) / kMobileTileSize - kWorldCenterX;
                spec.posStartY = 0.0f;
                spec.posStartZ = static_cast<float>(psy) / kMobileTileSize - kWorldCenterZ;
                spec.posEndX = static_cast<float>(pex) / kMobileTileSize - kWorldCenterX;
                spec.posEndY = 0.0f;
                spec.posEndZ = static_cast<float>(pey) / kMobileTileSize - kWorldCenterZ;
                spec.speed = std::max(0.5f, static_cast<float>(stepAdv) / 3.0f);
                // Real patrol-timing fields (plan.md E3D-MIG-131), used by
                // every MoveObject type except platform lifts/crates (which
                // keep the speed-based ping-pong `spec.speed` above feeds).
                // Real files always have all 4 present (01-world-file-format.md
                // §4's full field list) -- 1/1/0/0 (stepAdv/stepRec's real
                // minimum/timeStop's real default) if sscanf somehow didn't
                // match, not zero (a zero stepAdvanceTicks would divide by
                // zero in AdvancePatrolStep()).
                spec.stepAdvanceTicks = static_cast<float>(std::max(stepAdv, 1));
                spec.stepRecedeTicks = static_cast<float>(std::max(stepRec, 1));
                spec.timeStopStartTicks = static_cast<float>(std::max(stopStart, 0));
                spec.timeStopEndTicks = static_cast<float>(std::max(stopEnd, 0));

                if (IsPatrolMoveObjectType(type) &&
                    spec.posStartX == spec.posEndX && spec.posStartZ == spec.posEndZ)
                {
                    spec.posStartX -= 2.0f;
                    spec.posEndX += 2.0f;
                }

                spec.currentX = spec.posStartX;
                spec.currentY = spec.posStartY;
                spec.currentZ = spec.posStartZ;

                mobileObjects_.push_back(spec);
                continue;
            }

            if (section == Section::Decor && decorRow < kDecorGridSize)
            {
                std::stringstream row(line);
                std::string cell;
                int col = 0;
                while (col < kDecorGridSize && std::getline(row, cell, ','))
                {
                    if (!cell.empty())
                    {
                        const int tileId = std::stoi(cell);
                        if (tileId > 0)
                        {
                            const std::uint16_t blockType = BlockTypes::fromMobileIconId(tileId);
                            world_->setBlock(
                                static_cast<std::uint16_t>(col), 0,
                                static_cast<std::uint16_t>(decorRow),
                                Worlds::Block::make(blockType));
                        }
                    }
                    ++col;
                }
                ++decorRow;
                continue;
            }

            if (section == Section::BigDecor && bigDecorRow < kDecorGridSize)
            {
                std::stringstream row(line);
                std::string cell;
                int col = 0;
                while (col < kDecorGridSize && std::getline(row, cell, ','))
                {
                    if (!cell.empty())
                    {
                        const int tileId = std::stoi(cell);
                        if (tileId > 0)
                        {
                            bigDecor_[static_cast<std::size_t>(bigDecorRow) * kDecorGridSize +
                                      static_cast<std::size_t>(col)] = BlockTypes::fromMobileIconId(tileId);
                        }
                    }
                    ++col;
                }
                ++bigDecorRow;
                continue;
            }
        }

        return true;
    }

    bool GEWorldRuntime::LoadFromVwrFile(const std::string& path)
    {
        try
        {
            world_ = std::make_unique<Worlds::World>(Worlds::World::loadFromFile(path));
        }
        catch (const std::exception&)
        {
            return false;
        }

        spawnTileX_ = 0;
        spawnTileZ_ = 0;
        // .vwr header v2 (2026-07-09) carries a real skyRegion field now --
        // unlike spawn point (still no .vwr equivalent), this one no longer
        // needs to reset to 0.
        skyRegion_ = static_cast<int>(world_->skyRegion());
        bigDecor_.clear();

        // Unlike BigDecor: (a mobile-eggbert .txt-only concept), MoveObjects
        // CAN be embedded directly in the 3D .vwr format itself, via
        // Worlds::World's block-extra-metadata mechanism (see
        // GalaxyEggbert::MoveObjectRecord, NEXT.md §8 -- "3D world format
        // move-object storage"). Convert each engine-agnostic
        // MoveObjectRecord (raw grid-space floats, see its own doc comment)
        // into this class's own MobileObjSpec shape, applying the same
        // kWorldCenterX/Z shift the .txt loader and GetBigDecor() callers
        // already apply -- MobileObjSpec's positions are render/camera
        // space, not raw grid space. Y is never shifted (matches BigDecor).
        mobileObjects_.clear();
        for (const auto& record : CollectMoveObjects(*world_))
        {
            MobileObjSpec spec;
            spec.type = record.type;
            spec.posStartX = record.posStartX - static_cast<float>(kWorldCenterX);
            spec.posStartY = record.posStartY;
            spec.posStartZ = record.posStartZ - static_cast<float>(kWorldCenterZ);
            spec.posEndX = record.posEndX - static_cast<float>(kWorldCenterX);
            spec.posEndY = record.posEndY;
            spec.posEndZ = record.posEndZ - static_cast<float>(kWorldCenterZ);
            spec.speed = record.speed;
            spec.stepAdvanceTicks = record.stepAdvanceTicks;
            spec.stepRecedeTicks = record.stepRecedeTicks;
            spec.timeStopStartTicks = record.timeStopStartTicks;
            spec.timeStopEndTicks = record.timeStopEndTicks;
            spec.currentX = spec.posStartX;
            spec.currentY = spec.posStartY;
            spec.currentZ = spec.posStartZ;
            mobileObjects_.push_back(spec);
        }
        return true;
    }

    bool GEWorldRuntime::IsBlitzActiveAtPhase(int animPhase) noexcept
    {
        const int cycle = ((animPhase % 100) + 100) % 100; // defensive: handle a negative phase
        return cycle % 2 == 0 && cycle < 50;
    }

    bool GEWorldRuntime::IsCrusherActiveAtPhase(int animPhase) noexcept
    {
        const int cycle = (((animPhase / 3) % 10) + 10) % 10; // defensive: handle a negative phase
        return cycle <= 2;
    }

    bool GEWorldRuntime::IsTempPassableAtPhase(int animPhase) noexcept
    {
        const int cycle = (((animPhase / 4) % 20) + 20) % 20; // defensive: handle a negative phase
        return cycle >= 18;
    }

    std::optional<bool> GEWorldRuntime::TryActivateSwitch(float blupiX, float blupiY, float blupiZ,
                                                            bool blupiOnGround)
    {
        if (!blupiOnGround)
        {
            return std::nullopt;
        }
        const int blocksPerAxis = static_cast<int>(world_->blocksPerAxis());
        const int gx = std::clamp(static_cast<int>(std::lround(blupiX)) + kWorldCenterX, 0, blocksPerAxis - 1);
        const int gz = std::clamp(static_cast<int>(std::lround(blupiZ)) + kWorldCenterZ, 0, blocksPerAxis - 1);
        const int gy = static_cast<int>(std::lround(blupiY)) - 1;
        if (gy < 0 || gy >= blocksPerAxis)
        {
            return std::nullopt;
        }

        const auto switchType = world_->getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                   static_cast<std::uint16_t>(gz))
                                     .type();
        if (switchType != GalaxyEggbert::BlockTypes::Switch && switchType != GalaxyEggbert::BlockTypes::SwitchOff)
        {
            return std::nullopt;
        }

        // Real behavior: always toggles to the opposite of its current
        // state (Decor.cpp's own call site passes `currentIcon == 385`).
        const bool turningOn = (switchType == GalaxyEggbert::BlockTypes::SwitchOff);
        world_->setBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                          static_cast<std::uint16_t>(gz),
                          Worlds::Block::make(turningOn ? GalaxyEggbert::BlockTypes::Switch
                                                         : GalaxyEggbert::BlockTypes::SwitchOff));

        // Real 41-cell window (Decor.cpp:7138-7147): this switch's X ±20,
        // same Y and Z -- BlockTypes.hpp's own isSwitch() comment already
        // documents this exact mapping.
        const std::uint16_t sawFrom = turningOn ? GalaxyEggbert::BlockTypes::SawStopped
                                                 : GalaxyEggbert::BlockTypes::Saw;
        const std::uint16_t sawTo = turningOn ? GalaxyEggbert::BlockTypes::Saw
                                               : GalaxyEggbert::BlockTypes::SawStopped;
        for (int sx = gx - 20; sx <= gx + 20; ++sx)
        {
            if (sx < 0 || sx >= blocksPerAxis)
            {
                continue;
            }
            const auto sawBlock = world_->getBlock(static_cast<std::uint16_t>(sx),
                                                     static_cast<std::uint16_t>(gy),
                                                     static_cast<std::uint16_t>(gz));
            if (sawBlock.type() == sawFrom)
            {
                world_->setBlock(static_cast<std::uint16_t>(sx), static_cast<std::uint16_t>(gy),
                                  static_cast<std::uint16_t>(gz), Worlds::Block::make(sawTo));
            }
        }

        return turningOn;
    }

    void GEWorldRuntime::Update(float dt)
    {
        // Raw animation tick (2026-07-09, fixed from a flat 6fps clock
        // shared by every animated tile type -- reported live as "elements
        // animate too slowly"). Now advances at mobile-eggbert's real
        // reference tick rate, 20 ticks/sec (Config::ScaleTime(1)==1,
        // Decor.cpp/Config.hpp) -- the SAME base rate MobileObjSpec::phase
        // below already uses. GETerrainRenderer::AnimIcon() divides this raw
        // tick by each tile type's own real divisor (Decor.cpp's per-type
        // ScaleDiv(N), e.g. Saw/Fan tick every raw tick == 50ms, Lava every
        // 2 == 100ms, Water1/Crusher every 3 == 150ms, Spike/Temp every 4 ==
        // 200ms) -- a flat 166ms/frame for all of them was simply wrong, not
        // just a stylistic simplification.
        animTimer_ += dt;
        static constexpr float kAnimTickPeriod = 1.0f / 20.0f;
        while (animTimer_ >= kAnimTickPeriod)
        {
            animTimer_ -= kAnimTickPeriod;
            ++animPhase_;
        }

        // MoveObject per-instance animation phase (2026-07-09) -- 20fps
        // reference tick rate, matching mobile-eggbert's Config::ScaleTime(1)
        // base (Decor.cpp's MoveObject phase fields advance by 1 per tick at
        // that reference rate; ScaleTime()/ScaleDiv() only rescale it for
        // other target FPS values, see Config.hpp).
        constexpr float kMoveObjectPhaseTicksPerSecond = 20.0f;
        const float phaseDelta = dt * kMoveObjectPhaseTicksPerSecond;
        for (auto& obj : mobileObjects_)
        {
            obj.phase += phaseDelta;
        }
    }
}
