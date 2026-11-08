#include "Clipboard.hpp"

#include "AutoTile.hpp"

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>
#include <GalaxyEggbert/Game/LegacySawFloors.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <optional>
#include <set>

namespace GalaxyEggbert::Editor
{
    namespace
    {
        using Key = std::array<int, 3>;

        int TurnByte(const Worlds::World& world, int x, int y, int z, std::uint16_t type)
        {
            const auto* payload = world.findBlockExtraMetadata(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                               static_cast<std::uint16_t>(z), type);
            return payload != nullptr && !payload->empty() ? ((*payload)[0] & 3) : -1;
        }

        // A side fan (126-128 blowing -X, 129-131 +X, BlockTypes.hpp) or the wind it blows (110-113,
        // 114-117), whose way along its axis a turn or a mirror can reverse.
        bool IsSideFanOrWind(std::uint16_t icon) noexcept
        {
            return (icon >= 110 && icon <= 117) || (icon >= 126 && icon <= 131);
        }

        std::uint16_t OtherWay(std::uint16_t icon) noexcept
        {
            if (icon >= 110 && icon <= 113) return static_cast<std::uint16_t>(icon + 4);
            if (icon >= 114 && icon <= 117) return static_cast<std::uint16_t>(icon - 4);
            if (icon >= 126 && icon <= 128) return static_cast<std::uint16_t>(icon + 3);
            if (icon >= 129 && icon <= 131) return static_cast<std::uint16_t>(icon - 3);
            return icon;
        }

        bool IsFrontModel(std::uint16_t icon) noexcept
        {
            return std::find(Game::kFrontModelIcons.begin(), Game::kFrontModelIcons.end(), icon) !=
                       Game::kFrontModelIcons.end() ||
                   Game::IsSignModelIcon(icon);
        }

        // The tiles whose drawing or blowing reads the plate turns (a plate's axis, a fan's or wind
        // tile's way, a saw's): every tile that is not a plain cube.
        bool ReadsPlateTurns(const Worlds::Block& block) noexcept
        {
            return !block.isAir() && (IsSideFanOrWind(block.type()) ||
                                      GetBlockDefinition(block.type()).renderMode != BlockRenderMode::UniformCube);
        }

        // A side fan or wind tile along X (its plate turns even) blows the other way once turned
        // a quarter (+X goes to -Z, which is the other icon turned) or mirrored.
        Worlds::Block WithOtherWayAlongX(const Clipboard::Cell& cell)
        {
            const std::uint16_t icon = cell.block.type();
            if (cell.block.isAir() || !IsSideFanOrWind(icon) || (std::max(cell.plateTurns, 0) & 1) != 0)
            {
                return cell.block;
            }
            return Worlds::Block::make(OtherWay(icon), cell.block.metadata());
        }

        // The cells an object or a BigDecor stands in (its turns are its cell's, type 5).
        std::set<std::size_t> HeldCells(const Clipboard& clipboard)
        {
            std::set<std::size_t> held;
            const auto add = [&](long x, long y, long z) {
                if (x >= 0 && y >= 0 && z >= 0 && x < clipboard.sizeX && y < clipboard.sizeY && z < clipboard.sizeZ)
                {
                    held.insert(clipboard.Index(static_cast<int>(x), static_cast<int>(y), static_cast<int>(z)));
                }
            };
            for (const auto& object : clipboard.objects)
            {
                add(std::lround(object.posStartX), std::lround(object.posStartY), std::lround(object.posStartZ));
            }
            for (const auto& decor : clipboard.bigDecor)
            {
                add(decor.x, decor.y, decor.z);
            }
            return held;
        }

        Key AnchorOf(const MoveObjectRecord& record)
        {
            return {static_cast<int>(std::floor(record.posStartX)), static_cast<int>(std::floor(record.posStartY)),
                    static_cast<int>(std::floor(record.posStartZ))};
        }
    }

    Clipboard CopyBox(const Worlds::World& world, const BoxRegion& box)
    {
        Clipboard clipboard;
        clipboard.sizeX = box.maxX - box.minX + 1;
        clipboard.sizeY = box.maxY - box.minY + 1;
        clipboard.sizeZ = box.maxZ - box.minZ + 1;
        clipboard.cells.resize(static_cast<std::size_t>(clipboard.sizeX) * static_cast<std::size_t>(clipboard.sizeY) *
                               static_cast<std::size_t>(clipboard.sizeZ));
        for (int x = 0; x < clipboard.sizeX; ++x)
        {
            for (int y = 0; y < clipboard.sizeY; ++y)
            {
                for (int z = 0; z < clipboard.sizeZ; ++z)
                {
                    const int wx = box.minX + x;
                    const int wy = box.minY + y;
                    const int wz = box.minZ + z;
                    Clipboard::Cell& cell = clipboard.cells[clipboard.Index(x, y, z)];
                    cell.block = world.getBlock(static_cast<std::uint16_t>(wx), static_cast<std::uint16_t>(wy),
                                                static_cast<std::uint16_t>(wz));
                    cell.plateTurns = TurnByte(world, wx, wy, wz, Game::kPlateRotationMetadataType);
                    cell.objectTurns = TurnByte(world, wx, wy, wz, Game::kObjectTurnMetadataType);
                    cell.legacySwitchRow = TurnByte(world,wx,wy,wz,Game::kLegacySwitchSawRowMetadataType);
                }
            }
        }
        const auto inBox = [&](int x, int y, int z) {
            return x >= box.minX && x <= box.maxX && y >= box.minY && y <= box.maxY && z >= box.minZ && z <= box.maxZ;
        };
        for (MoveObjectRecord record : CollectMoveObjects(world))
        {
            const Key anchor = AnchorOf(record);
            if (!inBox(anchor[0], anchor[1], anchor[2]))
            {
                continue;
            }
            record.posStartX -= box.minX;
            record.posStartY -= box.minY;
            record.posStartZ -= box.minZ;
            record.posEndX -= box.minX;
            record.posEndY -= box.minY;
            record.posEndZ -= box.minZ;
            clipboard.objects.push_back(record);
        }
        for (const auto& decor : CollectBigDecor(world))
        {
            if (inBox(decor.x, decor.y, decor.z))
            {
                clipboard.bigDecor.push_back(
                    {decor.icon, decor.x - box.minX, decor.y - box.minY, decor.z - box.minZ});
            }
        }
        return clipboard;
    }

    Clipboard TurnedQuarter(const Clipboard& clipboard)
    {
        Clipboard turned = clipboard;
        turned.sizeX = clipboard.sizeZ;
        turned.sizeZ = clipboard.sizeX;
        const std::set<std::size_t> held = HeldCells(clipboard);
        const float lastX = static_cast<float>(clipboard.sizeX - 1);
        for (int x = 0; x < clipboard.sizeX; ++x)
        {
            for (int y = 0; y < clipboard.sizeY; ++y)
            {
                for (int z = 0; z < clipboard.sizeZ; ++z)
                {
                    const std::size_t from = clipboard.Index(x, y, z);
                    Clipboard::Cell cell = clipboard.cells[from];
                    cell.block = WithOtherWayAlongX(cell);
                    if (cell.plateTurns >= 0 || ReadsPlateTurns(cell.block))
                    {
                        cell.plateTurns = (std::max(cell.plateTurns, 0) + 1) & 3;
                    }
                    if (!IsFrontModel(cell.block.type()) && (cell.objectTurns >= 0 || held.count(from) != 0))
                    {
                        cell.objectTurns = (std::max(cell.objectTurns, 0) + 1) & 3;
                    }
                    turned.cells[turned.Index(z, y, clipboard.sizeX - 1 - x)] = cell;
                }
            }
        }
        for (auto& object : turned.objects)
        {
            const float startX = object.posStartX;
            const float endX = object.posEndX;
            object.posStartX = object.posStartZ;
            object.posStartZ = lastX - startX;
            object.posEndX = object.posEndZ;
            object.posEndZ = lastX - endX;
        }
        for (auto& decor : turned.bigDecor)
        {
            const int x = decor.x;
            decor.x = decor.z;
            decor.z = clipboard.sizeX - 1 - x;
        }
        turned.quarterTurns = (clipboard.quarterTurns + 1) % 4;
        return turned;
    }

    Clipboard MirroredX(const Clipboard& clipboard)
    {
        Clipboard mirrored = clipboard;
        const float lastX = static_cast<float>(clipboard.sizeX - 1);
        for (int x = 0; x < clipboard.sizeX; ++x)
        {
            for (int y = 0; y < clipboard.sizeY; ++y)
            {
                for (int z = 0; z < clipboard.sizeZ; ++z)
                {
                    Clipboard::Cell cell = clipboard.cells[clipboard.Index(x, y, z)];
                    cell.block = WithOtherWayAlongX(cell);
                    if (cell.plateTurns >= 0)
                    {
                        cell.plateTurns = (4 - cell.plateTurns) & 3;
                    }
                    if (cell.objectTurns >= 0)
                    {
                        cell.objectTurns = (4 - cell.objectTurns) & 3;
                    }
                    mirrored.cells[mirrored.Index(clipboard.sizeX - 1 - x, y, z)] = cell;
                }
            }
        }
        for (auto& object : mirrored.objects)
        {
            object.posStartX = lastX - object.posStartX;
            object.posEndX = lastX - object.posEndX;
        }
        for (auto& decor : mirrored.bigDecor)
        {
            decor.x = clipboard.sizeX - 1 - decor.x;
        }
        mirrored.mirrored = !clipboard.mirrored;
        return mirrored;
    }

    EditCommand PasteBox(Worlds::World& world, const Clipboard& clipboard, int ox, int oy, int oz, bool keepUnderAir,
                         const std::function<int()>& next)
    {
        EditCommand group;
        group.kind = EditCommand::Kind::Group;
        const int n = static_cast<int>(world.blocksPerAxis());
        const auto inWorld = [n](int x, int y, int z) {
            return x >= 0 && y >= 0 && z >= 0 && x < n && y < n && z < n;
        };
        const auto inStamp = [&](int x, int y, int z) {
            return x >= ox && y >= oy && z >= oz && x < ox + clipboard.sizeX && y < oy + clipboard.sizeY &&
                   z < oz + clipboard.sizeZ;
        };

        std::map<Key, MoveObjectRecord> objectsBefore;
        for (const auto& record : CollectMoveObjects(world))
        {
            objectsBefore.emplace(AnchorOf(record), record);
        }
        std::map<Key, BigDecorRecord> decorBefore;
        for (const auto& decor : CollectBigDecor(world))
        {
            decorBefore.emplace(Key{decor.x, decor.y, decor.z}, decor);
        }

        // The blocks and their turns.
        EditCommand blocks;
        blocks.kind = EditCommand::Kind::BlockEdit;
        std::vector<EditCommand> turns;
        const auto setTurns = [&](int x, int y, int z, bool object, int wanted) {
            const auto ux = static_cast<std::uint16_t>(x);
            const auto uy = static_cast<std::uint16_t>(y);
            const auto uz = static_cast<std::uint16_t>(z);
            const int had = std::max(
                TurnByte(world, x, y, z, object ? Game::kObjectTurnMetadataType : Game::kPlateRotationMetadataType), 0);
            const int want = std::max(wanted, 0);
            if (had == want)
            {
                return;
            }
            (object ? Game::SetObjectQuarterTurns : Game::SetQuarterTurns)(world, ux, uy, uz, want);
            EditCommand turn;
            turn.kind = EditCommand::Kind::TurnEdit;
            turn.turnX = ux;
            turn.turnY = uy;
            turn.turnZ = uz;
            turn.turnBefore = had;
            turn.turnAfter = want;
            turn.turnObject = object;
            turns.push_back(turn);
        };
        for (int x = 0; x < clipboard.sizeX; ++x)
        {
            for (int y = 0; y < clipboard.sizeY; ++y)
            {
                for (int z = 0; z < clipboard.sizeZ; ++z)
                {
                    const int wx = ox + x;
                    const int wy = oy + y;
                    const int wz = oz + z;
                    if (!inWorld(wx, wy, wz))
                    {
                        continue;
                    }
                    const Clipboard::Cell& cell = clipboard.cells[clipboard.Index(x, y, z)];
                    const bool keep = keepUnderAir && cell.block.isAir();
                    if (!keep)
                    {
                        const auto ux = static_cast<std::uint16_t>(wx);
                        const auto uy = static_cast<std::uint16_t>(wy);
                        const auto uz = static_cast<std::uint16_t>(wz);
                        const Worlds::Block before = world.getBlock(ux, uy, uz);
                        if (!(before == cell.block))
                        {
                            world.setBlock(ux, uy, uz, cell.block);
                            blocks.blockChanges.push_back({ux, uy, uz, before, cell.block});
                        }
                    }
                    // Under a kept empty cell, its own turns go only where the clipboard has some.
                    if (!keep || cell.plateTurns >= 0)
                    {
                        setTurns(wx, wy, wz, false, cell.plateTurns);
                    }
                    if (!keep || cell.objectTurns >= 0)
                    {
                        setTurns(wx, wy, wz, true, cell.objectTurns);
                    }
                    if (!keep || cell.legacySwitchRow>=0)
                    {
                        EditCommand meta;
                        meta.kind=EditCommand::Kind::MetadataEdit;
                        meta.metadataX=wx; meta.metadataY=wy; meta.metadataZ=wz;
                        meta.metadataType=Game::kLegacySwitchSawRowMetadataType;
                        if (const auto* old=world.findBlockExtraMetadata(wx,wy,wz,meta.metadataType)) meta.metadataBefore=*old;
                        if (cell.legacySwitchRow==1) meta.metadataAfter=std::vector<std::uint8_t>{1};
                        if (meta.metadataBefore!=meta.metadataAfter)
                        {
                            if (meta.metadataAfter) world.setBlockExtraMetadata(wx,wy,wz,meta.metadataType,*meta.metadataAfter);
                            else world.removeBlockExtraMetadata(wx,wy,wz,meta.metadataType);
                            turns.push_back(std::move(meta));
                        }
                    }
                }
            }
        }

        // The objects and BigDecor: those of the box go (unless kept), the clipboard's come.
        std::set<Key> objectKeys;
        std::set<Key> decorKeys;
        if (!keepUnderAir)
        {
            for (const auto& [key, record] : objectsBefore)
            {
                if (inStamp(key[0], key[1], key[2]))
                {
                    RemoveMoveObject(world, static_cast<std::uint16_t>(key[0]), static_cast<std::uint16_t>(key[1]),
                                     static_cast<std::uint16_t>(key[2]));
                    objectKeys.insert(key);
                }
            }
            for (const auto& [key, decor] : decorBefore)
            {
                if (inStamp(key[0], key[1], key[2]))
                {
                    RemoveBigDecor(world, decor.x, decor.y, decor.z);
                    decorKeys.insert(key);
                }
            }
        }
        for (MoveObjectRecord record : clipboard.objects)
        {
            record.posStartX += static_cast<float>(ox);
            record.posStartY += static_cast<float>(oy);
            record.posStartZ += static_cast<float>(oz);
            record.posEndX += static_cast<float>(ox);
            record.posEndY += static_cast<float>(oy);
            record.posEndZ += static_cast<float>(oz);
            const Key anchor = AnchorOf(record);
            if (!inWorld(anchor[0], anchor[1], anchor[2]))
            {
                continue;
            }
            PlaceMoveObject(world, record);
            objectKeys.insert(anchor);
        }
        for (const auto& decor : clipboard.bigDecor)
        {
            const Key anchor{ox + decor.x, oy + decor.y, oz + decor.z};
            if (!inWorld(anchor[0], anchor[1], anchor[2]))
            {
                continue;
            }
            PlaceBigDecor(world, {decor.icon, static_cast<std::uint16_t>(anchor[0]), static_cast<std::uint16_t>(anchor[1]),
                                  static_cast<std::uint16_t>(anchor[2])});
            decorKeys.insert(anchor);
        }

        // The edge pieces: round the stamp (its outer cells and the world's cells beside them), or
        // every cell placed when it was turned or mirrored (ED-0007, as a box fill).
        const std::vector<BlockChange> placed = blocks.blockChanges;
        const auto adaptMid = [&](int x, int y, int z) { AutoTile::AdaptMid(world, x, y, z, next, blocks.blockChanges); };
        for (const auto& change : placed)
        {
            const int x = change.x;
            const int y = change.y;
            const int z = change.z;
            if (clipboard.Transformed())
            {
                AutoTile::AdaptBorder(world, x, y, z, next, blocks.blockChanges);
                continue;
            }
            const bool onShell = x == ox || y == oy || z == oz || x == ox + clipboard.sizeX - 1 ||
                                 y == oy + clipboard.sizeY - 1 || z == oz + clipboard.sizeZ - 1;
            if (!onShell)
            {
                continue;
            }
            adaptMid(x, y, z);
            static constexpr std::array<Key, 6> kSides{
                {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
            for (const auto& side : kSides)
            {
                if (!inStamp(x + side[0], y + side[1], z + side[2]))
                {
                    adaptMid(x + side[0], y + side[1], z + side[2]);
                }
            }
        }

        if (!blocks.blockChanges.empty())
        {
            group.children.push_back(std::move(blocks));
        }
        for (auto& turn : turns)
        {
            group.children.push_back(std::move(turn));
        }
        std::map<Key, MoveObjectRecord> objectsAfter;
        for (const auto& record : CollectMoveObjects(world))
        {
            objectsAfter.emplace(AnchorOf(record), record);
        }
        for (const auto& key : objectKeys)
        {
            EditCommand command;
            command.kind = EditCommand::Kind::MoveObjectEdit;
            command.objectAnchorX = static_cast<std::uint16_t>(key[0]);
            command.objectAnchorY = static_cast<std::uint16_t>(key[1]);
            command.objectAnchorZ = static_cast<std::uint16_t>(key[2]);
            if (const auto found = objectsBefore.find(key); found != objectsBefore.end())
            {
                command.objectBefore = found->second;
            }
            if (const auto found = objectsAfter.find(key); found != objectsAfter.end())
            {
                command.objectAfter = found->second;
            }
            if (command.objectBefore || command.objectAfter)
            {
                group.children.push_back(std::move(command));
            }
        }
        std::map<Key, BigDecorRecord> decorAfter;
        for (const auto& decor : CollectBigDecor(world))
        {
            decorAfter.emplace(Key{decor.x, decor.y, decor.z}, decor);
        }
        for (const auto& key : decorKeys)
        {
            EditCommand command;
            command.kind = EditCommand::Kind::BigDecorEdit;
            command.bigDecorAnchorX = static_cast<std::uint16_t>(key[0]);
            command.bigDecorAnchorY = static_cast<std::uint16_t>(key[1]);
            command.bigDecorAnchorZ = static_cast<std::uint16_t>(key[2]);
            if (const auto found = decorBefore.find(key); found != decorBefore.end())
            {
                command.bigDecorBefore = found->second;
            }
            if (const auto found = decorAfter.find(key); found != decorAfter.end())
            {
                command.bigDecorAfter = found->second;
            }
            if (command.bigDecorBefore || command.bigDecorAfter)
            {
                group.children.push_back(std::move(command));
            }
        }
        return group;
    }
}
