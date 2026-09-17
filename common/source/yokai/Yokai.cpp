/* GPL-3.0-or-later */
#include "yokai/Yokai.hpp"
#include <algorithm>
#include <array>
#include <cctype>

namespace yokai
{
    namespace
    {
        constexpr Layout layouts[] = {
            {240, 0x5C, 0x54, 0x38, 0x2C},
            {406, 0x5C, 0x4F, 0x34, 0x20},
            {656, 0x54, 0x49, 0x28, 0x20},
            {418, 0x4C, 0x49, 0x38, 0x24},
            {766, 0x4C, 0x48, std::nullopt, 0x20},
        };
    }

    std::optional<Game> gameFromName(std::string_view name)
    {
        std::string upper(name);
        std::transform(upper.begin(), upper.end(), upper.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (upper == "YW1") return Game::YW1;
        if (upper == "YW2") return Game::YW2;
        if (upper == "YW3") return Game::YW3;
        if (upper == "BLASTERS" || upper == "BUSTERS") return Game::Blasters;
        if (upper == "BUSTERS2" || upper == "BUSTERS 2") return Game::Busters2;
        return std::nullopt;
    }

    const Layout& layout(Game game)
    {
        return layouts[static_cast<std::size_t>(game)];
    }

    std::string decodeNickname(std::span<const std::uint8_t> bytes)
    {
        const auto zero = std::find(bytes.begin(), bytes.end(), 0);
        std::string result(bytes.begin(), zero);
        // Save text can be UTF-8 or Shift-JIS. Preserve non-ASCII bytes for the
        // renderer's replacement path; printable ASCII remains immediately useful.
        for (char& value : result)
        {
            if (static_cast<unsigned char>(value) < 0x20)
            {
                value = '?';
            }
        }
        return result;
    }
}
