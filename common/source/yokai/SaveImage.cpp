/* GPL-3.0-or-later */
#include "yokai/SaveImage.hpp"
#include "yokai/Species.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace yokai
{
    namespace
    {
        std::uint16_t read16(std::span<const std::uint8_t> data, std::size_t offset)
        {
            if (offset + 2 > data.size()) throw Error("Unexpected end of save");
            return static_cast<std::uint16_t>(data[offset]) |
                   static_cast<std::uint16_t>(data[offset + 1] << 8);
        }

        std::uint32_t read32(std::span<const std::uint8_t> data, std::size_t offset)
        {
            if (offset + 4 > data.size()) throw Error("Unexpected end of save");
            return static_cast<std::uint32_t>(data[offset]) |
                   (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
                   (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
                   (static_cast<std::uint32_t>(data[offset + 3]) << 24);
        }

        void write16(std::span<std::uint8_t> data, std::size_t offset, std::uint16_t value)
        {
            if (offset + 2 > data.size()) throw Error("Unexpected end of save");
            data[offset] = static_cast<std::uint8_t>(value);
            data[offset + 1] = static_cast<std::uint8_t>(value >> 8);
        }

        struct Section
        {
            std::size_t payload;
            std::size_t size;
        };

        Section findSection(std::span<const std::uint8_t> body, std::uint8_t id,
            std::size_t minimumSize = 0)
        {
            for (std::size_t offset = 0; offset + 12 <= body.size(); offset++)
            {
                const std::uint32_t h1 = read32(body, offset);
                const std::uint32_t h2 = read32(body, offset + 4);
                const std::size_t size = h2 >> 8;
                if ((h1 & 0xFFFF) != 0xFFFE || (h2 & 0xFF) != id || size < minimumSize)
                {
                    continue;
                }
                const std::size_t end = offset + 8 + size;
                if (end + 4 <= body.size() && (read32(body, end) & 0xFFFF) == 0xFEFF)
                {
                    return {offset + 8, size};
                }
            }
            throw Error("Required save section was not found");
        }
    }

    SaveImage::SaveImage(Game game, std::vector<std::uint8_t> bytes)
        : mGame(game), mBytes(std::move(bytes))
    {
        (void)recordArea();
    }

    SaveImage::RecordArea SaveImage::recordArea() const
    {
        const Layout& info = layout(mGame);
        if (mGame == Game::YW1)
        {
            constexpr std::size_t offset = 0x1D08;
            if (mBytes.size() < offset + info.slots * info.recordSize)
            {
                throw Error("Save is too small for Yo-kai Watch 1");
            }
            return {offset, info.slots};
        }
        if (mBytes.size() < 0x28) throw Error("Native save is too small");
        const auto body = std::span<const std::uint8_t>(mBytes).subspan(0x20, mBytes.size() - 0x28);
        Section section = findSection(body, 0x07);
        if (section.size % info.recordSize != 0)
        {
            throw Error("Yo-kai record section has an invalid size");
        }
        const std::size_t slots = mGame == Game::Blasters ? section.size / info.recordSize : info.slots;
        if (section.size < slots * info.recordSize)
        {
            throw Error("Yo-kai record section is truncated");
        }
        return {0x20 + section.payload, slots};
    }

    std::size_t SaveImage::indexOffset() const
    {
        if (mGame == Game::YW1) return 0x73DC;
        const auto body = std::span<const std::uint8_t>(mBytes).subspan(0x20, mBytes.size() - 0x28);
        return 0x20 + findSection(body, 0x0A, recordArea().slots * 4).payload;
    }

    std::vector<Record> SaveImage::records() const
    {
        const Layout& info = layout(mGame);
        const RecordArea area = recordArea();
        std::vector<Record> output;
        output.reserve(area.slots);
        for (std::size_t slot = 0; slot < area.slots; slot++)
        {
            const std::size_t offset = area.offset + slot * info.recordSize;
            const auto raw = std::span<const std::uint8_t>(mBytes).subspan(offset, info.recordSize);
            const std::uint32_t id = read32(raw, 4);
            if (id == 0) continue;
            const std::uint8_t level = raw[info.levelOffset];
            if (level < 1 || level > 99) throw Error("Invalid Yo-kai level");
            const std::size_t nicknameEnd = std::min(info.nicknameEnd, raw.size());
            const std::string nickname = decodeNickname(raw.subspan(8, nicknameEnd - 8));
            const std::string_view knownName = speciesName(mGame, id);
            Record record;
            record.slot = slot;
            record.speciesId = id;
            record.species = knownName.empty() ? "Unknown #" + std::to_string(id) : std::string(knownName);
            record.nickname = nickname;
            record.level = level;
            if (info.xpOffset)
            {
                const std::uint32_t value = read32(raw, *info.xpOffset);
                record.xp = mGame == Game::Blasters && (value & 0x80000000) ? 0 : value;
            }
            record.raw.assign(raw.begin(), raw.end());
            output.emplace_back(std::move(record));
        }
        return output;
    }

    bool SaveImage::hasFreeSlot() const
    {
        const Layout& info = layout(mGame);
        const RecordArea area = recordArea();
        for (std::size_t slot = 0; slot < area.slots; slot++)
        {
            if (read32(mBytes, area.offset + slot * info.recordSize + 4) == 0) return true;
        }
        return false;
    }

    bool SaveImage::isPartySlot(std::size_t slot) const
    {
        // The main-series games keep the six active members at the start of
        // the Yo-kai array. YW1 fixtures containing only a party consistently
        // occupy slots 0-5; erasing one creates a hole that the game's Change
        // Members screen renders as an apparent duplicate. The action games
        // use four-member squads in the same leading positions.
        const std::size_t partySize =
            mGame == Game::Blasters || mGame == Game::Busters2 ? 4 : 6;
        return slot < partySize;
    }

    std::string SaveImage::playerName() const
    {
        // The published YW1 save dumper identifies the player-name field at
        // 0x28. Later games keep profile data inside versioned/obfuscated
        // sections, so do not guess at a field and risk displaying junk.
        if (mGame != Game::YW1 || mBytes.size() < 0x38) return {};
        return decodeNickname(std::span<const std::uint8_t>(mBytes).subspan(0x28, 0x10));
    }

    std::optional<std::uint64_t> SaveImage::playTimeSeconds() const
    {
        // No play-time field is currently verified across the supported save
        // formats. In particular, YW1 offset 0x60 is unrelated profile/world
        // state, not a 60 Hz play-time counter. Never present it as time.
        return std::nullopt;
    }

    void SaveImage::syncIndex(std::size_t slot, std::span<const std::uint8_t> removedNumber)
    {
        const Layout& info = layout(mGame);
        const RecordArea area = recordArea();
        const std::size_t recordOffset = area.offset + slot * info.recordSize;
        const std::size_t indexes = indexOffset();
        if (mGame == Game::Blasters || mGame == Game::Busters2)
        {
            const auto wanted = removedNumber.empty()
                ? std::span<const std::uint8_t>(mBytes).subspan(recordOffset, 4)
                : removedNumber;
            for (std::size_t item = 0; item < area.slots; item++)
            {
                const std::size_t index = indexes + item * 4;
                if (!removedNumber.empty() && std::equal(wanted.begin(), wanted.end(), mBytes.begin() + index))
                {
                    std::fill_n(mBytes.begin() + index, 4, 0);
                    return;
                }
                if (removedNumber.empty() && read32(mBytes, index) == 0)
                {
                    std::copy_n(wanted.begin(), 4, mBytes.begin() + index);
                    return;
                }
            }
            return;
        }
        const std::size_t index = indexes + slot * 4;
        if (read32(mBytes, recordOffset + 4) == 0)
        {
            std::fill_n(mBytes.begin() + index, 4, 0);
        }
        else
        {
            std::copy_n(mBytes.begin() + recordOffset, 4, mBytes.begin() + index);
        }
    }

    void SaveImage::assignNumbers(std::size_t absoluteRecordOffset)
    {
        const Layout& info = layout(mGame);
        const RecordArea area = recordArea();
        std::uint16_t highest = 0;
        bool found = false;
        for (std::size_t slot = 0; slot < area.slots; slot++)
        {
            const std::size_t offset = area.offset + slot * info.recordSize;
            if (offset != absoluteRecordOffset && read32(mBytes, offset + 4) != 0)
            {
                highest = std::max(highest, read16(mBytes, offset));
                found = true;
            }
        }
        const std::uint16_t number = found ? static_cast<std::uint16_t>(highest + 1) : 0;
        write16(mBytes, absoluteRecordOffset, number);
        write16(mBytes, absoluteRecordOffset + 2, static_cast<std::uint16_t>(number + 1));
    }

    void SaveImage::remove(std::size_t slot, std::span<const std::uint8_t> expected)
    {
        const Layout& info = layout(mGame);
        const RecordArea area = recordArea();
        if (slot >= area.slots || expected.size() != info.recordSize) throw Error("Invalid save slot");
        const std::size_t offset = area.offset + slot * info.recordSize;
        if (!std::equal(expected.begin(), expected.end(), mBytes.begin() + offset))
        {
            throw Error("Save changed since it was loaded");
        }
        const std::array<std::uint8_t, 4> number = {
            mBytes[offset], mBytes[offset + 1], mBytes[offset + 2], mBytes[offset + 3]};
        std::fill_n(mBytes.begin() + offset, info.recordSize, 0);
        syncIndex(slot, number);
    }

    std::size_t SaveImage::insert(std::span<const std::uint8_t> record, bool renumber)
    {
        const Layout& info = layout(mGame);
        const RecordArea area = recordArea();
        if (record.size() != info.recordSize) throw Error("Record has the wrong size");
        for (std::size_t slot = 0; slot < area.slots; slot++)
        {
            const std::size_t offset = area.offset + slot * info.recordSize;
            if (read32(mBytes, offset + 4) == 0)
            {
                std::copy(record.begin(), record.end(), mBytes.begin() + offset);
                if (renumber) assignNumbers(offset);
                syncIndex(slot);
                return slot;
            }
        }
        throw Error("Save has no empty Yo-kai slots");
    }
}
