/* GPL-3.0-or-later */
#ifndef YOKAI_TITLE_SOURCE_HPP
#define YOKAI_TITLE_SOURCE_HPP

#include "yokai/Yokai.hpp"
#include <3ds.h>
#include <cstdint>
#include <string>
#include <vector>

namespace yokai::title
{
    struct Location
    {
        Game game = Game::YW1;
        FS_MediaType media = MEDIATYPE_SD;
        std::uint64_t titleId = 0;
        std::string saveFile;
        std::string headFile;
    };

    [[nodiscard]] std::vector<Location> discover();
    [[nodiscard]] std::vector<std::uint8_t> read(const Location& location, const std::string& file);
    void write(const Location& location, std::span<const std::uint8_t> data);
}

#endif
