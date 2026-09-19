/* GPL-3.0-or-later */
#include "yokai/Bank.hpp"
#include "yokai/SaveImage.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>

namespace yokai
{
    namespace
    {
        constexpr std::array<std::uint8_t, 4> magic = {'Y', 'K', 'B', '1'};
        constexpr std::uint16_t version = 1;

        std::uint32_t crc32(std::span<const std::uint8_t> bytes)
        {
            std::uint32_t crc = 0xFFFFFFFF;
            for (std::uint8_t byte : bytes)
            {
                crc ^= byte;
                for (int bit = 0; bit < 8; bit++)
                {
                    crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
                }
            }
            return ~crc;
        }

        template <typename T> void appendInteger(std::vector<std::uint8_t>& out, T value)
        {
            for (std::size_t index = 0; index < sizeof(T); index++)
            {
                out.push_back(static_cast<std::uint8_t>(value >> (index * 8)));
            }
        }

        template <typename T> T readInteger(std::span<const std::uint8_t> data, std::size_t& offset)
        {
            if (offset + sizeof(T) > data.size()) throw Error("Bank file is truncated");
            T value = 0;
            for (std::size_t index = 0; index < sizeof(T); index++)
            {
                value |= static_cast<T>(data[offset++]) << (index * 8);
            }
            return value;
        }

        void appendBytes(std::vector<std::uint8_t>& out, std::span<const std::uint8_t> bytes)
        {
            if (bytes.size() > std::numeric_limits<std::uint16_t>::max())
                throw Error("Bank field is too large");
            appendInteger<std::uint16_t>(out, static_cast<std::uint16_t>(bytes.size()));
            out.insert(out.end(), bytes.begin(), bytes.end());
        }

        void appendString(std::vector<std::uint8_t>& out, std::string_view value)
        {
            appendBytes(out, {reinterpret_cast<const std::uint8_t*>(value.data()), value.size()});
        }

        std::vector<std::uint8_t> readBytes(std::span<const std::uint8_t> data, std::size_t& offset)
        {
            const std::size_t length = readInteger<std::uint16_t>(data, offset);
            if (offset + length > data.size()) throw Error("Bank field is truncated");
            std::vector<std::uint8_t> result(data.begin() + offset, data.begin() + offset + length);
            offset += length;
            return result;
        }

        std::string readString(std::span<const std::uint8_t> data, std::size_t& offset)
        {
            auto bytes = readBytes(data, offset);
            return {bytes.begin(), bytes.end()};
        }
    }

    Bank Bank::load(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path)) return {};
        std::ifstream stream(path, std::ios::binary);
        if (!stream) throw Error("Could not open bank file");
        std::vector<std::uint8_t> data(
            (std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        return decode(data);
    }

    Bank Bank::decode(std::span<const std::uint8_t> data)
    {
        if (data.size() < 16 || !std::equal(magic.begin(), magic.end(), data.begin()))
            throw Error("Not a Yo-kai Watch Bank file");
        std::size_t offset = 4;
        if (readInteger<std::uint16_t>(data, offset) != version)
            throw Error("Unsupported bank version");
        (void)readInteger<std::uint16_t>(data, offset);
        const std::uint32_t count = readInteger<std::uint32_t>(data, offset);
        const std::uint32_t expectedCrc = readInteger<std::uint32_t>(data, offset);
        if (crc32(data.subspan(offset)) != expectedCrc) throw Error("Bank checksum does not match");

        Bank bank;
        bank.mEntries.clear();
        for (std::uint32_t index = 0; index < count; index++)
        {
            BankEntry entry;
            entry.id = readInteger<std::uint64_t>(data, offset);
            entry.arrival = readInteger<std::uint64_t>(data, offset);
            const auto game = readInteger<std::uint8_t>(data, offset);
            if (game > static_cast<std::uint8_t>(Game::Busters2)) throw Error("Invalid source game");
            entry.sourceGame = static_cast<Game>(game);
            entry.level = readInteger<std::uint8_t>(data, offset);
            (void)readInteger<std::uint16_t>(data, offset);
            entry.speciesId = readInteger<std::uint32_t>(data, offset);
            entry.xp = readInteger<std::uint32_t>(data, offset);
            entry.species = readString(data, offset);
            entry.nickname = readString(data, offset);
            entry.raw = readBytes(data, offset);
            if (entry.level < 1 || entry.level > 99 || entry.raw.size() != layout(entry.sourceGame).recordSize)
                throw Error("Invalid bank entry");
            bank.mNextId = std::max(bank.mNextId, entry.id + 1);
            bank.mNextArrival = std::max(bank.mNextArrival, entry.arrival + 1);
            bank.mEntries.emplace_back(std::move(entry));
        }
        if (offset != data.size()) throw Error("Bank contains trailing data");
        std::stable_sort(bank.mEntries.begin(), bank.mEntries.end(),
            [](const BankEntry& left, const BankEntry& right) { return left.arrival < right.arrival; });
        return bank;
    }

    std::vector<std::uint8_t> Bank::encode() const
    {
        std::vector<std::uint8_t> payload;
        for (const BankEntry& entry : mEntries)
        {
            appendInteger<std::uint64_t>(payload, entry.id);
            appendInteger<std::uint64_t>(payload, entry.arrival);
            appendInteger<std::uint8_t>(payload, static_cast<std::uint8_t>(entry.sourceGame));
            appendInteger<std::uint8_t>(payload, entry.level);
            appendInteger<std::uint16_t>(payload, 0);
            appendInteger<std::uint32_t>(payload, entry.speciesId);
            appendInteger<std::uint32_t>(payload, entry.xp);
            appendString(payload, entry.species);
            appendString(payload, entry.nickname);
            appendBytes(payload, entry.raw);
        }
        std::vector<std::uint8_t> output(magic.begin(), magic.end());
        appendInteger<std::uint16_t>(output, version);
        appendInteger<std::uint16_t>(output, 0);
        appendInteger<std::uint32_t>(output, static_cast<std::uint32_t>(mEntries.size()));
        appendInteger<std::uint32_t>(output, crc32(payload));
        output.insert(output.end(), payload.begin(), payload.end());
        return output;
    }

    void Bank::saveAtomic(const std::filesystem::path& path) const
    {
        std::filesystem::create_directories(path.parent_path());
        const auto temporary = path.string() + ".tmp";
        const auto backup = path.string() + ".bak";
        const auto bytes = encode();
        {
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            if (!stream) throw Error("Could not create temporary bank");
            stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            stream.flush();
            if (!stream) throw Error("Could not write temporary bank");
        }
        if (std::filesystem::exists(path))
        {
            std::filesystem::remove(backup);
            std::filesystem::rename(path, backup);
        }
        try
        {
            std::filesystem::rename(temporary, path);
        }
        catch (...)
        {
            if (std::filesystem::exists(backup)) std::filesystem::rename(backup, path);
            throw;
        }
    }

    BankEntry& Bank::append(Game source, const Record& record)
    {
        BankEntry entry;
        entry.id = mNextId++;
        entry.arrival = mNextArrival++;
        entry.sourceGame = source;
        entry.speciesId = record.speciesId;
        entry.species = record.species;
        entry.nickname = record.nickname;
        entry.level = record.level;
        entry.xp = record.xp;
        entry.raw = record.raw;
        mEntries.emplace_back(std::move(entry));
        return mEntries.back();
    }

    void Bank::erase(std::uint64_t id)
    {
        const auto found = std::find_if(mEntries.begin(), mEntries.end(),
            [id](const BankEntry& entry) { return entry.id == id; });
        if (found == mEntries.end()) throw Error("Bank entry was not found");
        mEntries.erase(found);
    }

    const BankEntry* Bank::find(std::uint64_t id) const
    {
        const auto found = std::find_if(mEntries.begin(), mEntries.end(),
            [id](const BankEntry& entry) { return entry.id == id; });
        return found == mEntries.end() ? nullptr : &*found;
    }
}
