/* GPL-3.0-or-later */
#ifndef YOKAI_SPECIES_HPP
#define YOKAI_SPECIES_HPP

#include "Yokai.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace yokai
{
    [[nodiscard]] std::string_view speciesName(Game game, std::uint32_t id);
    [[nodiscard]] std::optional<std::uint32_t> speciesId(Game game, std::string_view name);
    [[nodiscard]] std::optional<std::uint32_t> transferSpeciesId(
        Game source, Game target, std::string_view name);
    [[nodiscard]] std::span<const std::uint32_t> defaultMoves(Game game, std::uint32_t id);
}

#endif
