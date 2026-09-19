/* GPL-3.0-or-later */
#ifndef YOKAI_BANK_HPP
#define YOKAI_BANK_HPP

#include "Yokai.hpp"
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace yokai
{
    struct BankEntry
    {
        std::uint64_t id = 0;
        std::uint64_t arrival = 0;
        Game sourceGame = Game::YW1;
        std::uint32_t speciesId = 0;
        std::string species;
        std::string nickname;
        std::uint8_t level = 1;
        std::uint32_t xp = 0;
        std::vector<std::uint8_t> raw;
    };

    class Bank
    {
    public:
        static Bank load(const std::filesystem::path& path);
        static Bank decode(std::span<const std::uint8_t> data);

        [[nodiscard]] std::vector<std::uint8_t> encode() const;
        void saveAtomic(const std::filesystem::path& path) const;

        [[nodiscard]] const std::vector<BankEntry>& entries() const { return mEntries; }
        [[nodiscard]] std::vector<BankEntry>& entries() { return mEntries; }
        [[nodiscard]] bool empty() const { return mEntries.empty(); }
        [[nodiscard]] std::size_t size() const { return mEntries.size(); }

        BankEntry& append(Game source, const Record& record);
        void erase(std::uint64_t id);
        [[nodiscard]] const BankEntry* find(std::uint64_t id) const;

    private:
        std::vector<BankEntry> mEntries;
        std::uint64_t mNextId = 1;
        std::uint64_t mNextArrival = 1;
    };
}

#endif
