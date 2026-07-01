#include "GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <cstdio>
#include <cstring>
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
        bool inDecor = false;
        int decorRow = 0;

        while (std::getline(file, line))
        {
            if (line.rfind("Decor:", 0) == 0)
            {
                inDecor = true;
                continue;
            }
            if (line.rfind("MoveObject:", 0) == 0)
            {
                // Object/decor parsing belongs to a later phase (plan.md Phase 7).
                continue;
            }
            if (!inDecor || decorRow >= kDecorGridSize)
            {
                continue;
            }

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
        }

        return true;
    }
}
