#include "GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

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
                // Object/decor parsing belongs to a later phase (plan.md Phase 7).
                section = Section::None;
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
        skyRegion_ = 0;
        bigDecor_.clear();
        return true;
    }

    void GEWorldRuntime::Update(float dt)
    {
        animTimer_ += dt;
        static constexpr float kAnimPeriod = 1.0f / 6.0f;
        while (animTimer_ >= kAnimPeriod)
        {
            animTimer_ -= kAnimPeriod;
            ++animPhase_;
        }
    }
}
