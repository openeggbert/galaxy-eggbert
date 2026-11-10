#pragma once

#include "NetProtocol.hpp"

#include <algorithm>

namespace GalaxyEggbert::Net
{
    // The host's lobby (the original's CEvent multiplayer screens, blupi_recon's assessment §3):
    // four places, each a player's name, colour and ready flag. Colours need not differ, as in the
    // original; the host may start when at least two are present and every present one is ready.
    class Lobby
    {
    public:
        // The place of a player who arrived (the first free one, his colour that place's), -1 full.
        int Add(std::uint8_t gamerId, const std::string& name)
        {
            if (const int existing = SlotOf(gamerId); existing >= 0) return existing;
            for (int i = 0; i < kMaxPlayers; ++i)
            {
                auto& slot = state.slots[static_cast<std::size_t>(i)];
                if (slot.present) continue;
                slot = LobbySlot{};
                slot.present = true;
                slot.gamerId = gamerId;
                slot.team = static_cast<std::uint8_t>(i);
                slot.name = CleanText(name, kMaxName);
                if (slot.name.empty()) slot.name = "Player";
                return i;
            }
            return -1;
        }
        void Remove(std::uint8_t gamerId)
        {
            if (const int slot = SlotOf(gamerId); slot >= 0) state.slots[static_cast<std::size_t>(slot)] = LobbySlot{};
        }
        void SetTeam(std::uint8_t gamerId, std::uint8_t team)
        {
            if (const int slot = SlotOf(gamerId); slot >= 0 && team < kTeams)
                state.slots[static_cast<std::size_t>(slot)].team = team;
        }
        void SetReady(std::uint8_t gamerId, bool ready)
        {
            if (const int slot = SlotOf(gamerId); slot >= 0) state.slots[static_cast<std::size_t>(slot)].ready = ready;
        }
        // After a match: nobody is ready any more.
        void ClearReady()
        {
            for (auto& slot : state.slots) slot.ready = false;
        }
        [[nodiscard]] int SlotOf(std::uint8_t gamerId) const noexcept
        {
            for (int i = 0; i < kMaxPlayers; ++i)
            {
                const auto& slot = state.slots[static_cast<std::size_t>(i)];
                if (slot.present && slot.gamerId == gamerId) return i;
            }
            return -1;
        }
        [[nodiscard]] int Present() const noexcept
        {
            return static_cast<int>(std::count_if(state.slots.begin(), state.slots.end(), [](const auto& s) { return s.present; }));
        }
        [[nodiscard]] bool CanStart() const noexcept
        {
            return Present() >= 2 &&
                   std::all_of(state.slots.begin(), state.slots.end(), [](const auto& s) { return !s.present || s.ready; });
        }

        LobbyState state;
    };
}
