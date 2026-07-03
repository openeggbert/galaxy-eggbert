#include "GEWorldRuntime.hpp"
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace GalaxyEggbert;
using namespace GalaxyEggbert::Worlds;
using namespace Simple3D;

namespace GESimple3D {

static constexpr float kBlupiHalfH = 23.0f / 64.0f; // same as old Blupi::kHalfH

const char* GEWorldRuntime::WorldName(int world) {
    static const char* kNames[] = {
        "Grassland", "Forest", "Ice Caves", "Lava Fields", "Space Station"
    };
    int idx = world - 1;
    return (idx >= 0 && idx < 5) ? kNames[idx] : "Unknown";
}

GEWorldRuntime::GEWorldRuntime()
    : world_(std::make_unique<World>()) {}

bool GEWorldRuntime::LoadFromMobileEggbertFile(const std::string& path) {
    static constexpr int kMobTile = 64;

    std::ifstream f(path);
    if (!f) return false;

    world_ = std::make_unique<World>();
    mobileObjects_.clear();
    bigDecor_.assign(100 * 100, BlockTypes::Air);
    skyRegion_ = 0;
    totalTreasures_ = 0;

    int blupiPosX = 0, blupiPosY = 0;
    {
        std::string header;
        if (!std::getline(f, header)) return false;
        const char* bp = std::strstr(header.c_str(), "blupiPos=");
        if (bp) std::sscanf(bp, "blupiPos=%d;%d", &blupiPosX, &blupiPosY);
        const char* rg = std::strstr(header.c_str(), "region=");
        if (rg) std::sscanf(rg, "region=%d", &skyRegion_);
    }

    const int blupiTileCol = blupiPosX / kMobTile;
    const int blupiTileRow = blupiPosY / kMobTile;
    blupiSpawn_ = Vector3(
        static_cast<float>(blupiTileCol - kWCX),
        kBlupiHalfH + 0.5f,
        static_cast<float>(blupiTileRow - kWCZ));

    std::string line;
    int decorRow = 0;
    int bigDecorRow = 0;
    // BigDecor: is a distinct section from Decor: (see mobile-eggbert-2d-reference.md
    // §2.3) — must be checked as its own prefix, not folded into the Decor:
    // row counter, or its 100 rows are silently skipped once decorRow
    // already reached 100 from the main grid.
    enum class Section { None, Decor, BigDecor };
    Section section = Section::None;

    while (std::getline(f, line)) {
        if (line.rfind("BigDecor:", 0) == 0) { section = Section::BigDecor; continue; }
        if (line.rfind("Decor:", 0) == 0)    { section = Section::Decor;    continue; }

        if (line.rfind("MoveObject:", 0) == 0) {
            section = Section::None;
            int type = 0, psx = 0, psy = 0, pex = 0, pey = 0, stepAdv = 1;
            std::sscanf(line.c_str(),
                "MoveObject: type=%d stepAdvance=%d %*s %*s %*s posStart=%d;%d posEnd=%d;%d",
                &type, &stepAdv, &psx, &psy, &pex, &pey);

            // Types 19,21,24,26,32,40,44,46,47,54,55,96 added — real ObjectType
            // values found in use across all 78 mobile-eggbert level files that
            // this loader previously silently dropped (mobile-eggbert-2d-reference.md §2.4).
            bool supported = (type == 1  || type == 2  || type == 3  || type == 4  || type == 5  ||
                              type == 6  || type == 7  || type == 12 || type == 13 || type == 16 ||
                              type == 17 || type == 19 || type == 20 || type == 21 || type == 24 ||
                              type == 25 || type == 26 || type == 30 || type == 32 || type == 33 ||
                              type == 40 || type == 44 || type == 46 || type == 47 || type == 49 ||
                              type == 50 || type == 51 || type == 54 || type == 55 || type == 96);
            if (!supported) continue;

            auto pixToV3 = [&](int px, int py) -> Vector3 {
                return Vector3(
                    static_cast<float>(px / kMobTile - kWCX),
                    1.05f,  // 0.05 above tile top (y=0.5) to avoid sprite depth-fight with tile face
                    static_cast<float>(py / kMobTile - kWCZ));
            };

            MobileObjSpec spec;
            spec.type     = static_cast<ObjectType>(type);
            spec.posStart = pixToV3(psx, psy);
            spec.posEnd   = pixToV3(pex, pey);
            spec.speed    = std::max(0.5f, static_cast<float>(stepAdv) / 3.0f);

            // 32 (blupih), 44 (wasp), 54 (large creature) patrol posStart<->posEnd
            // the same way as the existing patrol enemies (Decor.cpp
            // MoveObjectStepIcon keys their turn/walk icon off posStart vs
            // posEnd, i.e. they are patrol-line objects too).
            bool isPatrol = (type == 2 || type == 3 || type == 4 || type == 20 || type == 32 ||
                             type == 33 || type == 44 || type == 54);
            if (isPatrol && spec.posStart.x_ == spec.posEnd.x_ &&
                            spec.posStart.z_ == spec.posEnd.z_) {
                spec.posStart.x_ -= 2.0f;
                spec.posEnd.x_   += 2.0f;
            }
            if (type == 20) { spec.posStart.y_ = 3.0f; spec.posEnd.y_ = 3.0f; }
            if (type == 16) { spec.posStart.y_ = 4.0f; spec.posEnd.y_ = 1.0f; }
            // Wasp/bee (44) flies at head height, same convention as the bird (20).
            if (type == 44) { spec.posStart.y_ = 3.0f; spec.posEnd.y_ = 3.0f; }

            if (type == 5) ++totalTreasures_;

            mobileObjects_.push_back(spec);
            continue;
        }

        if (section == Section::Decor && decorRow < 100) {
            std::stringstream ss(line);
            std::string token;
            int col = 0;
            while (col < 100 && std::getline(ss, token, ',')) {
                if (!token.empty()) {
                    int tileId = std::stoi(token);
                    if (tileId > 0) {
                        uint16_t bt = BlockTypes::fromMobileIconId(tileId);
                        world_->setBlock(
                            static_cast<uint16_t>(col), 0,
                            static_cast<uint16_t>(decorRow),
                            Block::make(bt));
                    }
                }
                ++col;
            }
            ++decorRow;
            continue;
        }

        if (section == Section::BigDecor && bigDecorRow < 100) {
            std::stringstream ss(line);
            std::string token;
            int col = 0;
            while (col < 100 && std::getline(ss, token, ',')) {
                if (!token.empty()) {
                    int tileId = std::stoi(token);
                    if (tileId > 0) {
                        bigDecor_[static_cast<std::size_t>(bigDecorRow) * 100 + static_cast<std::size_t>(col)] =
                            BlockTypes::fromMobileIconId(tileId);
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

void GEWorldRuntime::BuildDemoWorld() {
    world_ = std::make_unique<World>();
    mobileObjects_.clear();
    bigDecor_.assign(100 * 100, BlockTypes::Air);
    skyRegion_ = 0;
    blupiSpawn_ = Vector3(0.0f, kBlupiHalfH + 0.5f, 0.0f);

    auto setBlock = [&](int dx, int h, int dz, uint16_t type) {
        int wx = kWCX + dx, wz = kWCZ + dz;
        if (wx >= 0 && wz >= 0 && wx < 100 && wz < 100)
            world_->setBlock(
                static_cast<uint16_t>(wx),
                static_cast<uint16_t>(h),
                static_cast<uint16_t>(wz),
                Block::make(type));
    };

    const int R = 12;
    for (int dz = -R; dz <= R; ++dz)
        for (int dx = -R; dx <= R; ++dx)
            setBlock(dx, 0, dz, BlockTypes::Ground);

    for (int dz = -3; dz <= 3; ++dz)
        for (int dx = 4; dx <= 8; ++dx)
            setBlock(dx, 1, dz, BlockTypes::StoneA);

    for (int dx = -4; dx <= 4; ++dx)
        setBlock(dx, 0, -6, BlockTypes::Lava);
    setBlock(2, 0, -6, BlockTypes::StoneB);
    setBlock(-2, 0, -6, BlockTypes::StoneB);
}

void GEWorldRuntime::ResetLevel() {
    levelTime_ = 0.0f;
    animTimer_ = 0.0f;
    animPhase_ = 0;
}

void GEWorldRuntime::Update(float dt) {
    levelTime_ += dt;
    animTimer_ += dt;
    // 6 fps animation tick — matches mobile-eggbert's animated tile rate
    static constexpr float kAnimPeriod = 1.0f / 6.0f;
    while (animTimer_ >= kAnimPeriod) {
        animTimer_ -= kAnimPeriod;
        animPhase_++;
    }
}

} // namespace GESimple3D
