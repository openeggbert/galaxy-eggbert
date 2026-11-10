#include "NetProtocol.hpp"

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
        for (const char c : text)
        {
            if (c >= 0x20 && c < 0x7f && out.size() < limit) out += c;
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
            for (const auto& slot : m.lobby.slots)
            {
                w.U8(static_cast<std::uint8_t>((slot.present ? 1 : 0) | (slot.ready ? 2 : 0)));
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
            break;
        case MessageKind::Pose:
            w.U32(m.pose.tick);
            w.F32(m.pose.x);
            w.F32(m.pose.y);
            w.F32(m.pose.z);
            w.F32(m.pose.yaw);
            w.U16(m.pose.clip);
            w.F32(m.pose.clipSeconds);
            w.U8(m.pose.alpha);
            w.U8(m.pose.flags);
            w.U8(m.pose.lives);
            break;
        case MessageKind::Events:
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
        case MessageKind::Die: w.U8(m.lives); break;
        case MessageKind::Lost:
        case MessageKind::Abandon:
        case MessageKind::End: break;
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
            kind < static_cast<std::uint8_t>(MessageKind::Hello) || kind > static_cast<std::uint8_t>(MessageKind::End))
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
            ok = r.U16(m.lobby.world) && m.lobby.world >= 1 && m.lobby.world <= 99;
            for (auto& slot : m.lobby.slots)
            {
                std::uint8_t flags = 0;
                ok = ok && r.U8(flags) && flags <= 3 && r.U8(slot.team) && slot.team < kTeams && r.U8(slot.gamerId) &&
                     r.Text(slot.name, kMaxName);
                slot.present = (flags & 1) != 0;
                slot.ready = (flags & 2) != 0;
                ok = ok && (slot.present || (!slot.ready && slot.name.empty()));
            }
            break;
        case MessageKind::Team: ok = r.U8(m.team) && m.team < kTeams; break;
        case MessageKind::Ready:
        {
            std::uint8_t v = 0;
            ok = r.U8(v) && v <= 1;
            m.ready = v == 1;
            break;
        }
        case MessageKind::Chat: ok = r.Text(m.text, kMaxChat) && !m.text.empty(); break;
        case MessageKind::Start: ok = r.U16(m.world) && m.world >= 1 && m.world <= 99 && r.U64(m.worldHash); break;
        case MessageKind::Pose:
            ok = r.U32(m.pose.tick) && r.F32(m.pose.x) && r.F32(m.pose.y) && r.F32(m.pose.z) && r.F32(m.pose.yaw) &&
                 r.U16(m.pose.clip) && r.F32(m.pose.clipSeconds) && r.U8(m.pose.alpha) && r.U8(m.pose.flags) &&
                 r.U8(m.pose.lives) && m.pose.flags < 4 && m.pose.lives <= kMultiplayerLives;
            break;
        case MessageKind::Events:
        {
            std::uint8_t count = 0;
            ok = r.U8(count) && count >= 1 && count <= kMaxEvents;
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
        case MessageKind::Die: ok = r.U8(m.lives) && m.lives <= kMultiplayerLives; break;
        case MessageKind::Lost:
        case MessageKind::Abandon:
        case MessageKind::End: break;
        }
        if (!ok || !r.Done()) return std::nullopt;
        return m;
    }
}
