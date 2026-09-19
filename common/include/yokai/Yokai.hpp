/*
 * Yo-kai Watch Bank for Nintendo 3DS
 * Copyright (C) 2026 Yo-kai Watch Bank contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef YOKAI_YOKAI_HPP
#define YOKAI_YOKAI_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace yokai
{
    enum class Game : std::uint8_t
    {
        YW1,
        YW2,
        YW3,
        Blasters,
        Busters2
    };

    struct Layout
    {
        std::size_t slots;
        std::size_t recordSize;
        std::size_t levelOffset;
        std::optional<std::size_t> xpOffset;
        std::size_t nicknameEnd;
    };

    struct Record
    {
        std::size_t slot = 0;
        std::uint32_t speciesId = 0;
        std::string species;
        std::string nickname;
        std::uint8_t level = 1;
        std::uint32_t xp = 0;
        std::vector<std::uint8_t> raw;

        [[nodiscard]] const std::string& displayName() const
        {
            return nickname.empty() ? species : nickname;
        }
    };

    [[nodiscard]] constexpr std::string_view gameName(Game game)
    {
        switch (game)
        {
            case Game::YW1: return "YW1";
            case Game::YW2: return "YW2";
            case Game::YW3: return "YW3";
            case Game::Blasters: return "BLASTERS";
            case Game::Busters2: return "BUSTERS2";
        }
        return "UNKNOWN";
    }

    [[nodiscard]] std::optional<Game> gameFromName(std::string_view name);
    [[nodiscard]] const Layout& layout(Game game);
    [[nodiscard]] std::string decodeNickname(std::span<const std::uint8_t> bytes);
}

#endif
