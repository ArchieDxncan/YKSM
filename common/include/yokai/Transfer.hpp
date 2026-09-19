/* GPL-3.0-or-later */
#ifndef YOKAI_TRANSFER_HPP
#define YOKAI_TRANSFER_HPP

#include "Bank.hpp"
#include <span>
#include <vector>

namespace yokai
{
    [[nodiscard]] bool compatible(Game target, const BankEntry& entry);
    [[nodiscard]] std::vector<std::uint8_t> convertRecord(
        const BankEntry& source, Game target, std::span<const std::uint8_t> destinationExample = {});
    void validateRecord(Game game, std::span<const std::uint8_t> record);
}

#endif
