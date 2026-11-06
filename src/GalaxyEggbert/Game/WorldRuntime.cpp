#include "WorldRuntime.hpp"
#include "PlateRotationMetadata.hpp"

#include "ObjectDefinitionRegistry.hpp"
#include "ObjectVerticalPlacement.hpp"
#include "BlupiController.hpp"

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
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

namespace GalaxyEggbert::Game
{
    namespace
    {
        // What the original's loader imposes whatever a world file says (Decor::Read,
        // Decor.cpp:11366-11370): a big creature (54) rests 152 ticks at each end, the length of
        // its rest animation (ObjectIcons.cpp), during which it is deadly.
        void ForceOriginalTiming(MobileObjSpec& spec)
        {
            if (spec.type == GalaxyEggbert::Def::ObjectType::ObjectType54)
            {
                spec.timeStopStartTicks = 152.0f;
                spec.timeStopEndTicks = 152.0f;
            }
        }
    }

    namespace
    {
        constexpr int kMobileTileSize = 64;
        constexpr int kDecorGridSize = 100;

        // Real mobile-eggbert .txt world files are read-only external data
        // (a hand-edited or malformed cell is a real, reachable input here,
        // even though this parser's only current callers pass known-good
        // reference files -- see this function's own comment on why it
        // stays a real, callable API rather than dead code) -- std::stoi()
        // throws on non-numeric or over-long text, uncaught, which would
        // otherwise terminate the process (found via a fresh code audit,
        // 2026-07-23). Returns 0 on any parse failure, which the existing
        // `if (tileId > 0)` gate at both call sites already treats as
        // "nothing here" -- no separate error path needed.
        int SafeStoi(const std::string& cell)
        {
            try
            {
                return std::stoi(cell);
            }
            catch (const std::exception&)
            {
                return 0;
            }
        }
    }

    WorldRuntime::WorldRuntime()
        : world_(std::make_unique<Worlds::World>())
    {
    }

    bool WorldRuntime::LoadFromMobileEggbertFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file)
        {
            return false;
        }

        world_ = std::make_unique<Worlds::World>();
        skyRegion_ = 0;
        // Real m_mission is derived from which level FILE is loaded (e.g.
        // world011.txt -> mission 11), not a header field within the file
        // itself (unlike region=) -- not modeled here, defaults to 0 (no
        // mission) same as skyRegion_ above. The .vwr loader below carries
        // a real, explicit missionNumber field instead.
        missionNumber_ = 0;

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
        spawnTileY_ = 1;
        spawnTileZ_ = blupiPixelY / kMobileTileSize;
        hasExplicitSpawnPoint_ = true;

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
        bigDecorCells_.clear();
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

                if (type < 0 || type > ObjectDefinitionRegistry::MaxObjectType)
                {
                    continue;
                }
                const auto objectType =
                    static_cast<GalaxyEggbert::Def::ObjectType>(type);
                const auto& objectDefinition = GetObjectDefinition(objectType);
                if (!objectDefinition.mobileWorldSupported)
                {
                    continue;
                }

                MobileObjSpec spec;
                spec.type = objectType;
                spec.posStartX = static_cast<float>(psx) / kMobileTileSize - kWorldCenterX;
                spec.posStartY = kGroundObjectCenterY;
                spec.posStartZ = static_cast<float>(psy) / kMobileTileSize - kWorldCenterZ;
                spec.posEndX = static_cast<float>(pex) / kMobileTileSize - kWorldCenterX;
                spec.posEndY = kGroundObjectCenterY;
                spec.posEndZ = static_cast<float>(pey) / kMobileTileSize - kWorldCenterZ;
                spec.speed = std::max(0.5f, static_cast<float>(stepAdv) / 3.0f);
                // Real patrol-timing fields (plan.md E3D-MIG-131), used by
                // every patrolling MoveObject type, lifts included (D43).
                // Real files always have all 4 present (01-world-file-format.md
                // §4's full field list) -- 1/1/0/0 (stepAdv/stepRec's real
                // minimum/timeStop's real default) if sscanf somehow didn't
                // match, not zero (a zero stepAdvanceTicks would divide by
                // zero in AdvancePatrolStep()).
                spec.stepAdvanceTicks = static_cast<float>(std::max(stepAdv, 1));
                spec.stepRecedeTicks = static_cast<float>(std::max(stepRec, 1));
                spec.timeStopStartTicks = static_cast<float>(std::max(stopStart, 0));
                spec.timeStopEndTicks = static_cast<float>(std::max(stopEnd, 0));
                ForceOriginalTiming(spec);

                if (objectDefinition.patrolMotion &&
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
                        const int tileId = SafeStoi(cell);
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
                        const int tileId = SafeStoi(cell);
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

        for (int row = 0; row < kDecorGridSize; ++row)
        {
            for (int col = 0; col < kDecorGridSize; ++col)
            {
                const std::uint16_t icon = bigDecor_[
                    static_cast<std::size_t>(row) * kDecorGridSize +
                    static_cast<std::size_t>(col)];
                if (icon != BlockTypes::Air)
                {
                    bigDecorCells_.push_back({
                        static_cast<float>(col - kWorldCenterX),
                        kGroundObjectCenterY,
                        static_cast<float>(row - kWorldCenterZ),
                        icon,
                    });
                }
            }
        }
        return true;
    }

    bool WorldRuntime::LoadFromVwrFile(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        return in && LoadFromVwrStream(in);
    }

    bool WorldRuntime::LoadFromVwrStream(std::istream& in)
    {
        try
        {
            world_ = std::make_unique<Worlds::World>(Worlds::World::loadFromStream(in));
        }
        catch (const std::exception&)
        {
            return false;
        }

        spawnTileX_ = 0;
        spawnTileY_ = 1;
        spawnTileZ_ = 0;
        hasExplicitSpawnPoint_ = false;
        bigDecor_.clear();
        bigDecorCells_.clear();

        // Found 2026-07-17 (hub/mission-progression system): a world switch
        // at runtime previously leaked the PREVIOUS world's animation phase
        // into the newly-loaded one (animPhase_/animTimer_ were never part
        // of "per-world state" before there was any such thing as switching
        // worlds at runtime) -- cosmetic only (animated tiles would start
        // mid-cycle instead of at phase 0), but still a real bug now that
        // LoadFromVwrFile() is called more than once per process lifetime.
        animPhase_ = 0;
        animTimer_ = 0.0f;

        ResyncFromWorld();
        return true;
    }

    void WorldRuntime::ResyncFromWorld()
    {
        // .vwr header v2 (2026-07-09) carries a real skyRegion field now --
        // unlike spawn point (still no .vwr equivalent), this one no longer
        // needs to reset to 0.
        skyRegion_ = static_cast<int>(world_->skyRegion());
        missionNumber_ = static_cast<int>(world_->missionNumber());
        if (world_->hasSpawnPoint())
        {
            spawnTileX_ = static_cast<int>(world_->spawnX());
            spawnTileY_ = static_cast<int>(world_->spawnY());
            spawnTileZ_ = static_cast<int>(world_->spawnZ());
            hasExplicitSpawnPoint_ = true;
        }
        else
        {
            spawnTileX_ = 0;
            spawnTileY_ = 1;
            spawnTileZ_ = 0;
            hasExplicitSpawnPoint_ = false;
        }

        hintZones_ = CollectHintZones(*world_);

        bigDecorCells_.clear();
        for (const auto& record : CollectBigDecor(*world_))
        {
            bigDecorCells_.push_back({
                static_cast<float>(record.x - kWorldCenterX),
                static_cast<float>(record.y),
                static_cast<float>(record.z - kWorldCenterZ),
                record.icon,
            });
        }
        if (bigDecorCells_.empty() &&
            bigDecor_.size() ==
                static_cast<std::size_t>(kDecorGridSize) * kDecorGridSize)
        {
            for (int row = 0; row < kDecorGridSize; ++row)
            {
                for (int col = 0; col < kDecorGridSize; ++col)
                {
                    const std::uint16_t icon = bigDecor_[
                        static_cast<std::size_t>(row) * kDecorGridSize +
                        static_cast<std::size_t>(col)];
                    if (icon != BlockTypes::Air)
                    {
                        bigDecorCells_.push_back({
                            static_cast<float>(col - kWorldCenterX),
                            kGroundObjectCenterY,
                            static_cast<float>(row - kWorldCenterZ),
                            icon,
                        });
                    }
                }
            }
        }

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
            if (!IsSupportedObjectType(record.type) ||
                GetObjectDefinition(record.type).placementKind == ObjectPlacementKind::Null)
            {
                continue;
            }
            MobileObjSpec spec;
            spec.type = record.type;
            spec.visualIcon = record.visualIcon;
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
            ForceOriginalTiming(spec);
            spec.currentX = spec.posStartX;
            spec.currentY = spec.posStartY;
            spec.currentZ = spec.posStartZ;
            mobileObjects_.push_back(spec);
        }
    }

    bool WorldRuntime::IsBlitzActiveAtPhase(int animPhase) noexcept
    {
        const int cycle = ((animPhase % 100) + 100) % 100; // defensive: handle a negative phase
        return cycle % 2 == 0 && cycle < 50;
    }

    bool WorldRuntime::IsCrusherActiveAtPhase(int animPhase) noexcept
    {
        const int cycle = (((animPhase / 3) % 10) + 10) % 10; // defensive: handle a negative phase
        return cycle <= 2;
    }

    bool WorldRuntime::IsTempPassableAtPhase(int animPhase) noexcept
    {
        const int cycle = (((animPhase / 4) % 20) + 20) % 20; // defensive: handle a negative phase
        return cycle >= 18;
    }

    int WorldRuntime::ComputeWorldSelectTarget(int currentMission, int selectIndex) noexcept
    {
        if (currentMission == 1)
        {
            return selectIndex == 13 ? 999 : selectIndex * 10;
        }
        if (selectIndex == 199)
        {
            return 1;
        }
        return (currentMission / 10) * 10 + selectIndex;
    }

    int WorldRuntime::ComputeMissionBack(int currentMission) noexcept
    {
        if (currentMission % 10 == 0)
        {
            return 1;
        }
        return (currentMission / 10) * 10;
    }

    int WorldRuntime::ComputeWinExitTarget(int currentMission) noexcept
    {
        if (currentMission == 1)
        {
            return 199;
        }
        if (currentMission == 199)
        {
            return 1;
        }
        return ComputeMissionBack(currentMission);
    }

    std::optional<bool> WorldRuntime::TryActivateSwitch(float blupiX, float blupiY, float blupiZ,
                                                            bool blupiOnGround)
    {
        if (!blupiOnGround)
        {
            return std::nullopt;
        }
        // The original's switch is a tile in Blupi's own cell, in front of which he stands
        // (Decor::IsSwitch: his middle within it, 4 px from its sides). In 3D it is a block: the one
        // he stands on (a switch in the floor), or one beside him at his own height that his body
        // touches (a switch machine standing on the floor or in a wall; owner 2026-11-04, OR-0067:
        // only the block under him counted, so walking up to the machine did nothing).
        const int blocksPerAxis = static_cast<int>(world_->blocksPerAxis());
        const float wx = blupiX + static_cast<float>(kWorldCenterX);
        const float wz = blupiZ + static_cast<float>(kWorldCenterZ);
        const int cx = static_cast<int>(std::lround(wx));
        const int cz = static_cast<int>(std::lround(wz));
        const int level = static_cast<int>(std::lround(blupiY));
        const auto isSwitchAt = [&](int x, int y, int z)
        {
            if (x < 0 || y < 0 || z < 0 || x >= blocksPerAxis || y >= blocksPerAxis || z >= blocksPerAxis)
            {
                return false;
            }
            const auto type = world_->getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                               static_cast<std::uint16_t>(z))
                                  .type();
            return type == GalaxyEggbert::BlockTypes::Switch || type == GalaxyEggbert::BlockTypes::SwitchOff;
        };
        constexpr float kReach = BlupiController::kBodyHalfWidth + 8.0f / 64.0f;
        int gx = cx;
        int gy = level - 1;
        int gz = cz;
        if (!isSwitchAt(gx, gy, gz))
        {
            float best = kReach;
            bool found = false;
            for (int dz = -1; dz <= 1; ++dz)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    if ((dx == 0 && dz == 0) || !isSwitchAt(cx + dx, level, cz + dz))
                    {
                        continue;
                    }
                    // His middle's distance from the block's footprint.
                    const float ox = std::max(0.0f, std::fabs(wx - static_cast<float>(cx + dx)) - 0.5f);
                    const float oz = std::max(0.0f, std::fabs(wz - static_cast<float>(cz + dz)) - 0.5f);
                    const float distance = std::sqrt(ox * ox + oz * oz);
                    if (distance <= best)
                    {
                        best = distance;
                        gx = cx + dx;
                        gy = level;
                        gz = cz + dz;
                        found = true;
                    }
                }
            }
            if (!found)
            {
                return std::nullopt;
            }
        }
        const auto switchType = world_->getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                   static_cast<std::uint16_t>(gz))
                                     .type();

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

    std::optional<std::uint16_t> WorldRuntime::TryConsumeFan(float blupiX, float blupiY, float blupiZ)
    {
        // Decor::IsVentillo(), in 3D: Blupi touches a fan at its mouth, the side it blows from --
        // left (126) toward -X, right (129) toward +X, up (132) toward +Y, down (135) toward -Y --
        // and the fan goes with the column of wind tiles in front of it (110, 114, 118, 122 and
        // their frames). The original's fan is open but for its housing (table_decor_quart: a
        // 16 px strip at its back) and goes off only with Blupi's middle inside its cell, within
        // 16 px of the mouth (Decor.cpp:7667-7735: on a floor fan, sunk into it; against a side
        // fan, into its mouth). Here a fan is a whole block (M3D-0004), so that is contact with
        // its mouth face: his middle within the fan's cross-section and his body against the face
        // -- standing on a floor fan, pressed into a side fan's mouth, his head up against a
        // ceiling fan. Beside a fan, a cell in front of it, he is safe (owner 2026-11-04, OR-0066:
        // the whole cell in front counted until then).
        struct FanMouth { std::uint16_t fan; int dx, dy; std::uint16_t wind; };
        static constexpr FanMouth kFans[] = {
            {GalaxyEggbert::BlockTypes::FanLeft, -1, 0, 110},
            {GalaxyEggbert::BlockTypes::FanRight, 1, 0, 114},
            {GalaxyEggbert::BlockTypes::FanUp, 0, 1, 118},
            {GalaxyEggbert::BlockTypes::FanDown, 0, -1, 122},
        };
        constexpr float kContact = BlupiController::kBodyHalfWidth + 2.0f / 64.0f; // against a side face
        constexpr float kFootContact = 4.0f / 64.0f;  // feet on a floor fan, head on a ceiling fan
        const int n = static_cast<int>(world_->blocksPerAxis());
        const float wx = blupiX + static_cast<float>(kWorldCenterX);
        const float wz = blupiZ + static_cast<float>(kWorldCenterZ);
        const int gx = static_cast<int>(std::lround(wx));
        const int gy = static_cast<int>(std::lround(blupiY));
        const int gz = static_cast<int>(std::lround(wz));
        const auto iconAt = [&](int x, int y, int z) -> std::uint16_t
        {
            if (x < 0 || y < 0 || z < 0 || x >= n || y >= n || z >= n)
            {
                return 0;
            }
            return world_->getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                    static_cast<std::uint16_t>(z))
                .type();
        };
        const auto turned = [&](int x, int y, int z)
        {
            return x >= 0 && y >= 0 && z >= 0 && x < n && y < n && z < n &&
                   IsPlateRotated(*world_, static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                  static_cast<std::uint16_t>(z));
        };
        const float feet = blupiY - 0.5f;
        const float head = feet + BlupiController::kHeadHeight;
        // The fan and the column of wind it blows go (ModifDecor(pos, -1) along the blast).
        const auto consume = [&](const FanMouth& mouth, int fx, int fy, int fz, bool alongZ)
        {
            const int stepX = alongZ ? 0 : mouth.dx;
            const int stepZ = alongZ ? mouth.dx : 0;
            world_->setBlock(static_cast<std::uint16_t>(fx), static_cast<std::uint16_t>(fy),
                             static_cast<std::uint16_t>(fz), Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));
            lastConsumedFan_ = {static_cast<float>(fx - kWorldCenterX), static_cast<float>(fy),
                                static_cast<float>(fz - kWorldCenterZ)};
            for (int x = fx + stepX, y = fy + mouth.dy, z = fz + stepZ;; x += stepX, y += mouth.dy, z += stepZ)
            {
                const auto icon = iconAt(x, y, z);
                if (icon < mouth.wind || icon > mouth.wind + 3)
                {
                    break;
                }
                world_->setBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                 static_cast<std::uint16_t>(z), Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));
            }
            return mouth.fan;
        };
        // Landed on a fan, whichever way it blows (owner 2026-11-04, OR-0068: on a side fan he stood
        // on its block's invisible top): his middle over it, his feet on its top. In the original
        // he would sink into its open cell; a whole block here, so its top is the contact.
        {
            const int fx = gx;
            const int fz = gz;
            const int fy = static_cast<int>(std::lround(feet - 0.5f));
            const auto icon = iconAt(fx, fy, fz);
            if (std::fabs(wx - static_cast<float>(fx)) < 0.5f && std::fabs(wz - static_cast<float>(fz)) < 0.5f &&
                std::fabs(feet - (static_cast<float>(fy) + 0.5f)) <= kFootContact)
            {
                for (const auto& mouth : kFans)
                {
                    if (icon == mouth.fan)
                    {
                        return consume(mouth, fx, fy, fz, mouth.dx != 0 && turned(fx, fy, fz));
                    }
                }
            }
        }
        for (const auto& mouth : kFans)
        {
            // The fan sits behind Blupi, against the way it blows; a turned side fan (M3D-0005)
            // blows along Z, -X/+X becoming -Z/+Z.
            for (const bool alongZ : {false, true})
            {
                if (alongZ && mouth.dx == 0)
                {
                    continue;
                }
                const int stepX = alongZ ? 0 : mouth.dx;
                const int stepZ = alongZ ? mouth.dx : 0;
                const int fx = gx - stepX;
                const int fy = gy - mouth.dy;
                const int fz = gz - stepZ;
                if (iconAt(fx, fy, fz) != mouth.fan || turned(fx, fy, fz) != alongZ)
                {
                    continue;
                }
                bool touching = false;
                if (mouth.dy == 0)
                {
                    // His middle across the fan's cell, his body against its mouth's face.
                    const float along = alongZ ? (wz - static_cast<float>(fz)) * static_cast<float>(mouth.dx)
                                               : (wx - static_cast<float>(fx)) * static_cast<float>(mouth.dx);
                    const float across = alongZ ? std::fabs(wx - static_cast<float>(fx))
                                                : std::fabs(wz - static_cast<float>(fz));
                    touching = across < 0.5f && along - 0.5f <= kContact;
                }
                else
                {
                    // Over (under) the fan, his feet on its top (his head at its bottom).
                    const bool over = std::fabs(wx - static_cast<float>(fx)) < 0.5f &&
                                      std::fabs(wz - static_cast<float>(fz)) < 0.5f;
                    touching = over && (mouth.dy > 0 ? feet - (static_cast<float>(fy) + 0.5f) <= kFootContact
                                                     : (static_cast<float>(fy) - 0.5f) - head <= kFootContact);
                }
                if (!touching)
                {
                    continue;
                }
                return consume(mouth, fx, fy, fz, alongZ);
            }
        }
        return std::nullopt;
    }

    bool WorldRuntime::FindTeleportDestination(std::uint16_t icon, float blupiX, float blupiY, float blupiZ,
                                                   float& destX, float& destY, float& destZ) const
    {
        // See this method's own header comment for why a distance-from-
        // Blupi radius replaces the real source's exact entry-tile
        // equality check.
        constexpr float kEntryExclusionRadius = 1.5f;
        const int blocksPerAxis = static_cast<int>(world_->blocksPerAxis());

        for (int gx = 0; gx < blocksPerAxis; ++gx)
        {
            for (int gy = 0; gy < blocksPerAxis; ++gy)
            {
                for (int gz = 0; gz < blocksPerAxis; ++gz)
                {
                    if (world_->getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                          static_cast<std::uint16_t>(gz))
                            .type() != icon)
                    {
                        continue;
                    }
                    const float candX = static_cast<float>(gx) - static_cast<float>(kWorldCenterX);
                    const float candZ = static_cast<float>(gz) - static_cast<float>(kWorldCenterZ);
                    const float dx = candX - blupiX;
                    const float dy = static_cast<float>(gy) - blupiY;
                    const float dz = candZ - blupiZ;
                    if (dx * dx + dy * dy + dz * dz < kEntryExclusionRadius * kEntryExclusionRadius)
                    {
                        continue;
                    }
                    // Directly beneath the partner, as the original lands him (`newpos` =
                    // the tile's column, one row down; BD-0017). He does not go straight
                    // back: the arrival starts the contact grace (m_blupiPosHelico,
                    // Decor.cpp:6360), and a teleporter takes him only once he has left
                    // that spot (5594).
                    destX = candX;
                    destY = static_cast<float>(gy) - 1.0f;
                    destZ = candZ;
                    return true;
                }
            }
        }
        return false;
    }

    void WorldRuntime::Update(float dt)
    {
        // Raw animation tick (2026-07-09, fixed from a flat 6fps clock
        // shared by every animated tile type -- reported live as "elements
        // animate too slowly"). Now advances at mobile-eggbert's real
        // reference tick rate, 20 ticks/sec (Config::ScaleTime(1)==1,
        // Decor.cpp/Config.hpp) -- the SAME base rate MobileObjSpec::phase
        // below already uses. TerrainRenderer::AnimIcon() divides this raw
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

    std::vector<SolidBox> WorldRuntime::SolidBigDecorBoxes() const
    {
        std::vector<SolidBox> boxes;
        if (world_ == nullptr)
        {
            return boxes;
        }
        for (const auto& decor : bigDecorCells_)
        {
            if (!IsSolidBigDecorIcon(decor.icon))
            {
                continue;
            }
            const float cx = decor.worldX + static_cast<float>(kWorldCenterX);
            const float cy = decor.worldY;
            const float cz = decor.worldZ + static_cast<float>(kWorldCenterZ);
            const int turns = ObjectQuarterTurns(*world_, static_cast<std::uint16_t>(std::lround(cx)),
                                                 static_cast<std::uint16_t>(std::lround(cy)),
                                                 static_cast<std::uint16_t>(std::lround(cz)));
            std::vector<SolidBox> shape;
            const auto found = bigDecorShapes_.find(decor.icon);
            if (found != bigDecorShapes_.end() && !found->second.empty())
            {
                shape = found->second;
            }
            else
            {
                shape.push_back({-0.5f, 0.5f, 0.0f, static_cast<float>(kSolidBigDecorHeight), -0.5f, 0.5f});
            }
            for (const auto& box : shape)
            {
                // A quarter turn about +Y as the model's (XNA CreateRotationY: x' = z, z' = -x).
                float minX = box.minX, maxX = box.maxX, minZ = box.minZ, maxZ = box.maxZ;
                for (int k = 0; k < turns; ++k)
                {
                    const float nMinX = minZ, nMaxX = maxZ, nMinZ = -maxX, nMaxZ = -minX;
                    minX = nMinX;
                    maxX = nMaxX;
                    minZ = nMinZ;
                    maxZ = nMaxZ;
                }
                boxes.push_back({cx + minX, cx + maxX, cy + box.minY, cy + box.maxY, cz + minZ, cz + maxZ});
            }
        }
        return boxes;
    }
}
