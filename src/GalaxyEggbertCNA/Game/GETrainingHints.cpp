#include "GETrainingHints.hpp"

#include <cstddef>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Real Decor::IsDisplayInfo gate codes (Decor.cpp:1313-1340).
        constexpr int kGateAlways = -1;
        constexpr int kGateNoVehicle = -2;
        constexpr int kGateAnyVehicle = -3;
        constexpr int kGateNoDynamite = -4;
        constexpr int kGateHasDynamite = -5;
        // Any gate value >= 0 means "treasuresCollected == gate" (real
        // `m_nbTresor == tableTresor`) -- checked directly, no named
        // constant needed.

        struct TrainingHintRecord
        {
            int colMin, colMax, rowMin, rowMax;
            int gate;
            const char* text;
        };

        // Mission 11 (Tables::table_training1, 22 real records). Real
        // source's own action-flag values: -2 (0 elsewhere in this table)
        // and 1 mean "treasuresCollected == 0" / "== 1" -- an exact-count
        // gate, not a threshold.
        constexpr TrainingHintRecord kTraining1[] = {
            {1, 3, 0, 50, kGateAlways, "Use the directional wheel [Move]."},
            {4, 4, 0, 50, 0, "Press Jump [Jump]."},
            {6, 6, 0, 50, 1, "Press Right [Move] and Jump [Jump]."},
            {9, 9, 0, 50, kGateAlways, "Press Right [Move] and Jump [Jump]."},
            {12, 14, 0, 50, 1, "Don't fall into the water [Move] [Jump]!"},
            {16, 16, 0, 50, kGateAlways, ""},
            {20, 21, 0, 50, kGateAlways, "Take the elevator quietly, without jumping [Move]."},
            {23, 24, 0, 50, kGateAlways, "Jump on the elevator."},
            {27, 27, 0, 50, kGateAlways, ""},
            {28, 28, 0, 50, kGateAlways, "Move forward without jumping nor stopping [Move]!"},
            {30, 31, 0, 50, kGateAlways, ""},
            {36, 36, 0, 50, kGateAlways, ""},
            {39, 39, 0, 50, kGateAlways, "Move forward on the platform [Move]."},
            {44, 44, 0, 50, kGateAlways, "Leave the platform [Move]."},
            {46, 46, 0, 50, kGateAlways, "Once again, but faster..."},
            {53, 53, 0, 50, kGateAlways, "Move forward on the platform [Move], then jump [Move] [Jump]."},
            {56, 56, 0, 50, kGateAlways, "Jump when you are on the platform [Move] [Jump]."},
            {62, 64, 0, 50, kGateAlways, "Choose the upper path [Move] [Jump]."},
            {65, 66, 0, 50, kGateAlways, "Eggs give you extra lives."},
            {69, 74, 0, 50, kGateAlways, "Once on the top, move forward on the other platform without delay..."},
            {80, 85, 0, 50, 1, "Catch the second and last treasure."},
            {87, 93, 0, 50, kGateAlways, "Join the red arrow."},
        };

        // Mission 12 (Tables::table_training2, 5 real records, all
        // unconditional).
        constexpr TrainingHintRecord kTraining2[] = {
            {9, 15, 0, 100, kGateAlways, "Push the box forward until the red dot with [Action]."},
            {16, 16, 0, 100, kGateAlways, "Practical, right?"},
            {19, 21, 0, 100, kGateAlways, "Pull the box backward until the red dot with [Move]."},
            {24, 31, 0, 100, kGateAlways, "Stack both boxes up on the red dot to move on."},
            {33, 40, 0, 100, kGateAlways, "Stack the three boxes up on the red dot to move on."},
        };

        // Mission 13 (Tables::table_training3, 11 real records). Real
        // source's -2/-3 gates only ever test Helicopter/Skateboard/Tank
        // vs. Jeep to resolve to the same "in ANY vehicle at all" result
        // either way (Decor.cpp:1319-1332) -- collapsed to a single
        // inAnyVehicle bool here, not a Jeep-specific distinction.
        constexpr TrainingHintRecord kTraining3[] = {
            {16, 24, 36, 40, kGateNoVehicle, "Take a helicopter with [Action]."},
            {16, 24, 36, 40, kGateAnyVehicle, "Use [Move] or [Jump] to take off. Direct with [Move] and [Move]."},
            {22, 25, 34, 34, kGateAnyVehicle, "Leave the helicopter with [Action], it dislikes water!"},
            {22, 31, 34, 37, kGateNoVehicle, "Plunge. Use [Move] [Move] [Move] [Move] to direct."},
            {32, 43, 34, 34, kGateNoVehicle, "Take a helicopter [Action] then take off with [Move] or [Jump]."},
            {32, 43, 26, 34, kGateAnyVehicle, "Grab the three treasures, then go up."},
            {44, 48, 20, 22, kGateNoVehicle, "Go and get a skate in the top left corner."},
            {29, 31, 12, 12, kGateAlways, "Take a skate with [Action]."},
            {44, 54, 20, 22, kGateAnyVehicle, "You can move on with your skate without fear [Move]."},
            {63, 65, 20, 22, kGateAlways, "Your skate dislikes water! Jump!"},
            {77, 81, 20, 22, kGateAnyVehicle, "Leave your skate with [Action]."},
        };

        // Mission 14 (Tables::table_training4, 5 real records).
        constexpr TrainingHintRecord kTraining4[] = {
            {7, 14, 0, 100, kGateNoDynamite, "Take the dynamite sticks with [Action]."},
            {7, 19, 0, 100, kGateHasDynamite, "Do not put down the dynamite here!"},
            {20, 22, 0, 100, kGateNoDynamite, "Go and get the dynamite sticks on the left."},
            {20, 22, 0, 100, kGateHasDynamite, "Put down the dynamite with [Action], then clear off!"},
            {27, 28, 42, 100, kGateAlways, "Put down another stick of dynamite here to move on."},
        };

        bool GateOpen(int gate, int treasuresCollected, bool inAnyVehicle, bool hasDynamite) noexcept
        {
            if (gate >= 0)
            {
                return treasuresCollected == gate;
            }
            switch (gate)
            {
                case kGateNoVehicle:
                    return !inAnyVehicle;
                case kGateAnyVehicle:
                    return inAnyVehicle;
                case kGateNoDynamite:
                    return !hasDynamite;
                case kGateHasDynamite:
                    return hasDynamite;
                default: // kGateAlways and anything unrecognized
                    return true;
            }
        }

        template <std::size_t N>
        const char* Scan(const TrainingHintRecord (&table)[N], int gridX, int gridZ,
                         int treasuresCollected, bool inAnyVehicle, bool hasDynamite) noexcept
        {
            for (const auto& record : table)
            {
                if (gridX < record.colMin || gridX > record.colMax ||
                    gridZ < record.rowMin || gridZ > record.rowMax)
                {
                    continue;
                }
                if (!GateOpen(record.gate, treasuresCollected, inAnyVehicle, hasDynamite))
                {
                    continue;
                }
                // First rect+gate match wins (real source's own `break`),
                // even if its text is empty -- do not fall through.
                return (record.text[0] != '\0') ? record.text : nullptr;
            }
            return nullptr;
        }
    }

    const char* FindTrainingHint(int mission, int gridX, int gridZ,
                                 int treasuresCollected,
                                 bool inAnyVehicle, bool hasDynamite) noexcept
    {
        switch (mission)
        {
            case 11:
                return Scan(kTraining1, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite);
            case 12:
                return Scan(kTraining2, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite);
            case 13:
                return Scan(kTraining3, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite);
            case 14:
                return Scan(kTraining4, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite);
            default:
                return nullptr;
        }
    }
}
