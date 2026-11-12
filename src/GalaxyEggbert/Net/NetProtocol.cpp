#include "NetProtocol.hpp"

#include <GalaxyEggbert/Game/TextMetrics.hpp>

#include <cmath>
#include <cstring>

namespace GalaxyEggbert::Net
{
    namespace
    {
        class Writer
        {
        public:
            void U8(std::uint8_t v) { bytes.push_back(v); }
            void U16(std::uint16_t v) { for (int i = 0; i < 2; ++i) bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i))); }
            void U32(std::uint32_t v) { for (int i = 0; i < 4; ++i) bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i))); }
            void U64(std::uint64_t v) { for (int i = 0; i < 8; ++i) bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i))); }
            void I32(std::int32_t v) { U32(static_cast<std::uint32_t>(v)); }
            void F32(float v)
            {
                std::uint32_t bits = 0;
                std::memcpy(&bits, &v, sizeof bits);
                U32(bits);
            }
            void Text(const std::string& text, std::size_t limit)
            {
                const std::string clean = CleanText(text, limit);
                U8(static_cast<std::uint8_t>(clean.size()));
                bytes.insert(bytes.end(), clean.begin(), clean.end());
            }
            std::vector<std::uint8_t> bytes;
        };

        class Reader
        {
        public:
            Reader(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}
            bool U8(std::uint8_t& v) { return Take(1) && (v = data_[pos_ - 1], true); }
            bool U16(std::uint16_t& v)
            {
                if (!Take(2)) return false;
                v = static_cast<std::uint16_t>(data_[pos_ - 2] | (data_[pos_ - 1] << 8));
                return true;
            }
            bool U32(std::uint32_t& v)
            {
                if (!Take(4)) return false;
                v = 0;
                for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(data_[pos_ - 4 + i]) << (8 * i);
                return true;
            }
            bool U64(std::uint64_t& v)
            {
                if (!Take(8)) return false;
                v = 0;
                for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(data_[pos_ - 8 + i]) << (8 * i);
                return true;
            }
            bool I32(std::int32_t& v)
            {
                std::uint32_t u = 0;
                if (!U32(u)) return false;
                v = static_cast<std::int32_t>(u);
                return true;
            }
            bool F32(float& v)
            {
                std::uint32_t bits = 0;
                if (!U32(bits)) return false;
                std::memcpy(&v, &bits, sizeof v);
                return std::isfinite(v) && std::fabs(v) < 1.0e6f; // no NaN, no runaway coordinate
            }
            bool Text(std::string& text, std::size_t limit)
            {
                std::uint8_t length = 0;
                if (!U8(length) || length > limit || !Take(length)) return false;
                text.assign(reinterpret_cast<const char*>(data_ + pos_ - length), length);
                return CleanText(text, limit) == text;
            }
            [[nodiscard]] bool Done() const noexcept { return pos_ == size_; }

        private:
            bool Take(std::size_t n)
            {
                if (size_ - pos_ < n) return false;
                pos_ += n;
                return true;
            }
            const std::uint8_t* data_;
            std::size_t size_;
            std::size_t pos_ = 0;
        };
    }

    std::string CleanText(const std::string& text, std::size_t limit)
    {
        std::string out;
        for (std::size_t at = 0; at < text.size();)
        {
            const std::size_t start = at;
            const char32_t c = Game::TextMetrics::NextCodePoint(text, at);
            const bool keep = (c >= 0x20 && c < 0x7f) ||
                              (c >= 0xA0 && c != U'\uFFFD' && c <= 0x10FFFF && !(c >= 0xFDD0 && c <= 0xFDEF) &&
                               (c & 0xFFFE) != 0xFFFE);
            if (!keep) continue;
            if (out.size() + (at - start) > limit) break;
            out.append(text, start, at - start);
        }
        return out;
    }

    std::string AsciiText(const std::string& text, std::size_t limit)
    {
        // The Latin letters of the game's languages (and their neighbours) without their marks.
        static constexpr struct { char32_t from; const char* to; } kPlain[] = {
            {U'à', "a"}, {U'á', "a"}, {U'â', "a"}, {U'ã', "a"}, {U'ä', "a"}, {U'å', "a"}, {U'ç', "c"}, {U'č', "c"},
            {U'ď', "d"}, {U'è', "e"}, {U'é', "e"}, {U'ê', "e"}, {U'ë', "e"}, {U'ě', "e"}, {U'ì', "i"}, {U'í', "i"},
            {U'î', "i"}, {U'ï', "i"}, {U'ľ', "l"}, {U'ĺ', "l"}, {U'ñ', "n"}, {U'ň', "n"}, {U'ò', "o"}, {U'ó', "o"},
            {U'ô', "o"}, {U'õ', "o"}, {U'ö', "o"}, {U'ŕ', "r"}, {U'ř', "r"}, {U'š', "s"}, {U'ť', "t"}, {U'ù', "u"},
            {U'ú', "u"}, {U'û', "u"}, {U'ü', "u"}, {U'ů', "u"}, {U'ý', "y"}, {U'ÿ', "y"}, {U'ž', "z"}, {U'ß', "ss"},
            {U'À', "A"}, {U'Á', "A"}, {U'Â', "A"}, {U'Ã', "A"}, {U'Ä', "A"}, {U'Å', "A"}, {U'Ç', "C"}, {U'Č', "C"},
            {U'Ď', "D"}, {U'È', "E"}, {U'É', "E"}, {U'Ê', "E"}, {U'Ë', "E"}, {U'Ě', "E"}, {U'Ì', "I"}, {U'Í', "I"},
            {U'Î', "I"}, {U'Ï', "I"}, {U'Ľ', "L"}, {U'Ĺ', "L"}, {U'Ñ', "N"}, {U'Ň', "N"}, {U'Ò', "O"}, {U'Ó', "O"},
            {U'Ô', "O"}, {U'Õ', "O"}, {U'Ö', "O"}, {U'Ŕ', "R"}, {U'Ř', "R"}, {U'Š', "S"}, {U'Ť', "T"}, {U'Ù', "U"},
            {U'Ú', "U"}, {U'Û', "U"}, {U'Ü', "U"}, {U'Ů', "U"}, {U'Ý', "Y"}, {U'Ÿ', "Y"}, {U'Ž', "Z"},
        };
        std::string out;
        for (std::size_t at = 0; at < text.size();)
        {
            const char32_t c = Game::TextMetrics::NextCodePoint(text, at);
            std::string piece;
            if (c >= 0x20 && c < 0x7f) piece = static_cast<char>(c);
            for (const auto& plain : kPlain)
                if (plain.from == c) piece = plain.to;
            if (out.size() + piece.size() > limit) break;
            out += piece;
        }
        return out;
    }

    bool IsReliable(MessageKind kind) noexcept { return kind != MessageKind::Pose; }

    std::vector<std::uint8_t> Encode(const Message& m)
    {
        Writer w;
        w.U16(kProtocolId);
        w.U8(kProtocolVersion);
        w.U8(static_cast<std::uint8_t>(m.kind));
        switch (m.kind)
        {
        case MessageKind::Hello:
            w.U32(m.simulationRevision);
            w.U64(m.contentHash);
            w.Text(m.name, kMaxName);
            break;
        case MessageKind::Lobby:
            w.U16(m.lobby.world);
            w.U8(m.lobby.lives);
            w.U8(m.lobby.minutes);
            w.U64(m.lobby.customHash);
            w.Text(m.lobby.customName, kMaxWorldName);
            for (const auto& slot : m.lobby.slots)
            {
                w.U8(static_cast<std::uint8_t>((slot.present ? 1 : 0) | (slot.ready ? 2 : 0) | (slot.playing ? 4 : 0)));
                w.U8(slot.team);
                w.U8(slot.gamerId);
                w.Text(slot.name, kMaxName);
            }
            break;
        case MessageKind::Team: w.U8(m.team); break;
        case MessageKind::Ready: w.U8(m.ready ? 1 : 0); break;
        case MessageKind::Chat: w.Text(m.text, kMaxChat); break;
        case MessageKind::Start:
            w.U16(m.world);
            w.U64(m.worldHash);
            w.U8(m.lives);
            w.U8(m.minutes);
            w.U64(m.customHash);
            break;
        case MessageKind::Pose:
            w.U32(m.pose.tick);
            w.F32(m.pose.x);
            w.F32(m.pose.y);
            w.F32(m.pose.z);
            for (const float v : m.pose.world) w.F32(v);
            w.U16(m.pose.clip);
            w.F32(m.pose.clipSeconds);
            w.U8(m.pose.alpha);
            w.U8(m.pose.flags);
            w.U8(m.pose.lives);
            w.U8(m.pose.aura);
            break;
        case MessageKind::Snapshot:
        case MessageKind::Events:
            if (m.kind == MessageKind::Snapshot) w.U8(m.target);
            w.U8(static_cast<std::uint8_t>(m.events.size() < kMaxEvents ? m.events.size() : kMaxEvents));
            for (std::size_t i = 0; i < m.events.size() && i < kMaxEvents; ++i)
            {
                const auto& e = m.events[i];
                w.U8(static_cast<std::uint8_t>(e.kind));
                w.I32(e.id);
                w.I32(e.a);
                w.I32(e.b);
                w.I32(e.c);
                w.I32(e.d);
                w.F32(e.x);
                w.F32(e.y);
                w.F32(e.z);
            }
            break;
        case MessageKind::Die:
            w.U8(m.lives);
            w.U8(m.killer);
            break;
        case MessageKind::Pause: w.U8(m.ready ? 1 : 0); break;
        case MessageKind::Lost: w.U8(m.killer); break;
        case MessageKind::Abandon: break;
        case MessageKind::Sounds:
            w.U8(static_cast<std::uint8_t>(m.sounds.size() < kMaxSounds ? m.sounds.size() : kMaxSounds));
            for (std::size_t i = 0; i < m.sounds.size() && i < kMaxSounds; ++i)
            {
                const auto& s = m.sounds[i];
                w.U8(s.channel);
                w.U8(s.stop ? 1 : 0);
                w.F32(s.x);
                w.F32(s.y);
                w.F32(s.z);
            }
            break;
        case MessageKind::Lifts:
            w.U8(static_cast<std::uint8_t>(m.lifts.size() < kMaxLifts ? m.lifts.size() : kMaxLifts));
            for (std::size_t i = 0; i < m.lifts.size() && i < kMaxLifts; ++i)
            {
                w.I32(m.lifts[i].index);
                w.U8(m.lifts[i].step);
            }
            break;
        case MessageKind::StopCloud: w.U8(m.target); break;
        case MessageKind::TimeUp:
            for (const auto place : m.places) w.U8(place);
            break;
        case MessageKind::Rejoin:
            w.U8(m.target);
            w.U8(m.rejoinSlot);
            w.U16(m.world);
            w.U64(m.customHash);
            w.U8(m.lives);
            w.U8(m.minutes);
            w.F32(m.seconds);
            w.U32(m.firstSpawn);
            for (const auto& s : m.scores)
            {
                w.U8(static_cast<std::uint8_t>((s.played ? 1 : 0) | (s.left ? 2 : 0)));
                w.U8(s.deaths);
                w.U8(s.kills);
                w.U8(s.out);
                w.U8(s.team);
                w.Text(s.name, kMaxName);
            }
            break;
        case MessageKind::WorldData:
        {
            const std::size_t n = m.chunk.size() < kWorldChunk ? m.chunk.size() : kWorldChunk;
            w.U64(m.customHash);
            w.U32(m.worldSize);
            w.U32(m.worldOffset);
            w.U16(static_cast<std::uint16_t>(n));
            w.bytes.insert(w.bytes.end(), m.chunk.begin(), m.chunk.begin() + static_cast<long>(n));
            break;
        }
        }
        return w.bytes;
    }

    std::optional<Message> Decode(const std::uint8_t* data, std::size_t size)
    {
        if (data == nullptr) return std::nullopt;
        Reader r(data, size);
        std::uint16_t id = 0;
        std::uint8_t version = 0, kind = 0;
        if (!r.U16(id) || id != kProtocolId || !r.U8(version) || version != kProtocolVersion || !r.U8(kind) ||
            kind < static_cast<std::uint8_t>(MessageKind::Hello) || kind > static_cast<std::uint8_t>(MessageKind::Snapshot))
            return std::nullopt;
        Message m;
        m.kind = static_cast<MessageKind>(kind);
        bool ok = true;
        switch (m.kind)
        {
        case MessageKind::Hello:
            ok = r.U32(m.simulationRevision) && r.U64(m.contentHash) && r.Text(m.name, kMaxName) && !m.name.empty();
            break;
        case MessageKind::Lobby:
            ok = r.U16(m.lobby.world) && m.lobby.world >= 1 && m.lobby.world <= 999 && r.U8(m.lobby.lives) &&
                 m.lobby.lives <= kMultiplayerLives && r.U8(m.lobby.minutes) && m.lobby.minutes <= kMaxMinutes &&
                 r.U64(m.lobby.customHash) && r.Text(m.lobby.customName, kMaxWorldName) &&
                 (m.lobby.customHash != 0 || m.lobby.customName.empty());
            for (auto& slot : m.lobby.slots)
            {
                std::uint8_t flags = 0;
                ok = ok && r.U8(flags) && flags <= 7 && r.U8(slot.team) && slot.team < kTeams && r.U8(slot.gamerId) &&
                     r.Text(slot.name, kMaxName);
                slot.present = (flags & 1) != 0;
                slot.ready = (flags & 2) != 0;
                slot.playing = (flags & 4) != 0;
                ok = ok && (slot.present || (!slot.ready && !slot.playing && slot.name.empty()));
            }
            break;
        case MessageKind::Team: ok = r.U8(m.team) && m.team < kTeams; break;
        case MessageKind::Ready:
        case MessageKind::Pause:
        {
            std::uint8_t v = 0;
            ok = r.U8(v) && v <= 1;
            m.ready = v == 1;
            break;
        }
        case MessageKind::Chat: ok = r.Text(m.text, kMaxChat) && !m.text.empty(); break;
        case MessageKind::Start:
            ok = r.U16(m.world) && m.world >= 1 && m.world <= 999 && r.U64(m.worldHash) && r.U8(m.lives) &&
                 m.lives <= kMultiplayerLives && r.U8(m.minutes) && m.minutes <= kMaxMinutes && r.U64(m.customHash);
            break;
        case MessageKind::Pose:
            ok = r.U32(m.pose.tick) && r.F32(m.pose.x) && r.F32(m.pose.y) && r.F32(m.pose.z);
            for (auto& v : m.pose.world) ok = ok && r.F32(v);
            ok = ok && r.U16(m.pose.clip) && r.F32(m.pose.clipSeconds) && r.U8(m.pose.alpha) && r.U8(m.pose.flags) &&
                 r.U8(m.pose.lives) && r.U8(m.pose.aura) && m.pose.flags <= kPoseFlags &&
                 m.pose.lives <= kMultiplayerLives && m.pose.aura <= kAuraLimit;
            break;
        case MessageKind::Snapshot:
        case MessageKind::Events:
        {
            std::uint8_t count = 0;
            if (m.kind == MessageKind::Snapshot) ok = r.U8(m.target);
            ok = ok && r.U8(count) && count >= 1 && count <= kMaxEvents;
            for (int i = 0; ok && i < count; ++i)
            {
                WorldEvent e;
                std::uint8_t k = 0;
                ok = r.U8(k) && k >= 1 && k <= 4 && r.I32(e.id) && r.I32(e.a) && r.I32(e.b) && r.I32(e.c) &&
                     r.I32(e.d) && r.F32(e.x) && r.F32(e.y) && r.F32(e.z);
                e.kind = static_cast<EventKind>(k);
                if (ok && e.kind == EventKind::BlockChanged)
                    ok = e.a >= 0 && e.a < 1024 && e.b >= 0 && e.b < 1024 && e.c >= 0 && e.c < 1024 && e.d >= 0 &&
                         e.d <= 0xFFFF;
                if (ok && e.kind == EventKind::ObjectRetired) ok = e.id >= 0 && e.id < 100000;
                if (ok && e.kind == EventKind::ObjectSpawned) ok = e.a >= 0 && e.a < 1024;
                m.events.push_back(e);
            }
            break;
        }
        case MessageKind::Die:
            ok = r.U8(m.lives) && m.lives <= kMultiplayerLives && r.U8(m.killer) &&
                 (m.killer < kMaxPlayers || m.killer == kNoKiller);
            break;
        case MessageKind::Lost: ok = r.U8(m.killer) && (m.killer < kMaxPlayers || m.killer == kNoKiller); break;
        case MessageKind::Abandon: break;
        case MessageKind::Sounds:
        {
            std::uint8_t count = 0;
            ok = r.U8(count) && count >= 1 && count <= kMaxSounds;
            for (int i = 0; ok && i < count; ++i)
            {
                SoundEvent s;
                std::uint8_t stop = 0;
                ok = r.U8(s.channel) && s.channel < kSoundChannels && r.U8(stop) && stop <= 1 && r.F32(s.x) &&
                     r.F32(s.y) && r.F32(s.z);
                s.stop = stop == 1;
                m.sounds.push_back(s);
            }
            break;
        }
        case MessageKind::Lifts:
        {
            std::uint8_t count = 0;
            ok = r.U8(count) && count >= 1 && count <= kMaxLifts;
            for (int i = 0; ok && i < count; ++i)
            {
                LiftSync l;
                ok = r.I32(l.index) && l.index >= 0 && l.index < 100000 && r.U8(l.step) && (l.step == 1 || l.step == 3);
                m.lifts.push_back(l);
            }
            break;
        }
        case MessageKind::StopCloud: ok = r.U8(m.target); break;
        case MessageKind::TimeUp:
            for (auto& place : m.places) ok = ok && r.U8(place) && place <= kMaxPlayers;
            break;
        case MessageKind::Rejoin:
            ok = r.U8(m.target) && r.U8(m.rejoinSlot) && m.rejoinSlot < kMaxPlayers && r.U16(m.world) && m.world >= 1 &&
                 m.world <= 999 && r.U64(m.customHash) && r.U8(m.lives) && m.lives <= kMultiplayerLives && r.U8(m.minutes) &&
                 m.minutes <= kMaxMinutes && r.F32(m.seconds) && m.seconds >= 0.0f && r.U32(m.firstSpawn) &&
                 m.firstSpawn >= 1 && m.firstSpawn <= 0xffffff;
            for (auto& s : m.scores)
            {
                std::uint8_t flags = 0;
                ok = ok && r.U8(flags) && flags <= 3 && r.U8(s.deaths) && r.U8(s.kills) && r.U8(s.out) && s.out <= kMaxPlayers &&
                     r.U8(s.team) && s.team < kTeams && r.Text(s.name, kMaxName);
                s.played = (flags & 1) != 0;
                s.left = (flags & 2) != 0;
            }
            break;
        case MessageKind::WorldData:
        {
            std::uint16_t n = 0;
            ok = r.U64(m.customHash) && m.customHash != 0 && r.U32(m.worldSize) && m.worldSize >= 1 &&
                 m.worldSize <= kMaxWorldBytes && r.U32(m.worldOffset) && r.U16(n) && n >= 1 && n <= kWorldChunk &&
                 m.worldOffset < m.worldSize && n <= m.worldSize - m.worldOffset;
            for (int i = 0; ok && i < n; ++i)
            {
                std::uint8_t b = 0;
                ok = r.U8(b);
                m.chunk.push_back(b);
            }
            break;
        }
        }
        if (!ok || !r.Done()) return std::nullopt;
        return m;
    }
}
