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
        const RecordArea area = recordArea();
        if (slot >= area.slots) return false;
        const Layout& info = layout(mGame);
        const std::size_t record = area.offset + slot * info.recordSize;
        if (read32(mBytes, record + 4) == 0) return false;
        const std::uint32_t number = read32(mBytes, record);

        // Section 0A (0x73DC in YW1) is an ordered identifier list, not a
        // slot-parallel table. Its leading entries are the active party/squad.
        // This matters for saves such as the YW1 fixture whose party Rubinyan
        // lives in record slot 116 but is the second identifier in this list.
        const std::size_t partySize =
            mGame == Game::Blasters || mGame == Game::Busters2 ? 4 : 6;
        const std::size_t indexes = indexOffset();
        for (std::size_t item = 0; item < partySize; item++)
            if (read32(mBytes, indexes + item * 4) == number) return true;
        return false;
    }

    std::vector<std::size_t> SaveImage::partySlots() const
    {
        const RecordArea area = recordArea();
        const Layout& info = layout(mGame);
        const std::size_t partySize =
            mGame == Game::Blasters || mGame == Game::Busters2 ? 4 : 6;
        const std::size_t indexes = indexOffset();
        std::vector<std::size_t> output;
        output.reserve(partySize);
        for (std::size_t member = 0; member < partySize; member++)
        {
            const std::uint32_t number = read32(mBytes, indexes + member * 4);
            for (std::size_t slot = 0; slot < area.slots; slot++)
            {
                const std::size_t record = area.offset + slot * info.recordSize;
                if (read32(mBytes, record + 4) != 0 && read32(mBytes, record) == number)
                {
                    output.push_back(slot);
                    break;
                }
            }
        }
        return output;
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
        const auto wanted = removedNumber.empty()
            ? std::span<const std::uint8_t>(mBytes).subspan(recordOffset, 4)
            : removedNumber;
        for (std::size_t item = 0; item < area.slots; item++)
        {
            const std::size_t index = indexes + item * 4;
            if (!removedNumber.empty() &&
                std::equal(wanted.begin(), wanted.end(), mBytes.begin() + index))
            {
                const std::size_t following = area.slots - item - 1;
                if (following)
                    std::memmove(mBytes.data() + index, mBytes.data() + index + 4,
                        following * 4);
                std::fill_n(mBytes.begin() + indexes + (area.slots - 1) * 4, 4, 0);
                return;
            }
            if (removedNumber.empty() && read32(mBytes, index) == 0)
            {
                std::copy_n(wanted.begin(), 4, mBytes.begin() + index);
                return;
            }
        }
        if (removedNumber.empty()) throw Error("Yo-kai index has no empty entries");
        throw Error("Yo-kai identifier was not found in the index");
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
        // Update the fallible identifier table first. Once that succeeds,
        // clearing a validated in-bounds record cannot fail.
        syncIndex(slot, number);
        std::fill_n(mBytes.begin() + offset, info.recordSize, 0);
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
                try
                {
                    syncIndex(slot);
                }
                catch (...)
                {
                    std::fill_n(mBytes.begin() + offset, info.recordSize, 0);
                    throw;
                }
                return slot;
            }
        }
        throw Error("Save has no empty Yo-kai slots");
    }
}
