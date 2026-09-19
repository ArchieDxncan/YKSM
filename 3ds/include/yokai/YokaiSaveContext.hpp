/* GPL-3.0-or-later */
#ifndef YOKAI_SAVE_CONTEXT_HPP
#define YOKAI_SAVE_CONTEXT_HPP

#include "yokai/Crypto.hpp"
#include "yokai/Session.hpp"
#include "yokai/YokaiTitleSource.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace yokai
{
    struct SaveSource
    {
        Game game = Game::YW1;
        std::optional<title::Location> installed;
        std::filesystem::path exported;

        [[nodiscard]] bool isInstalled() const { return installed.has_value(); }
    };

    // Owns one decrypted save and its bank for the entire selector -> overview
    // -> transfer flow. On 3DS this avoids rediscovering titles, reopening the
    // archive, decrypting the save, and parsing every Yo-kai on each screen.
    class SaveContext
    {
    public:
        static std::shared_ptr<SaveContext> load(SaveSource source);

        SaveSource source;
        std::vector<std::uint8_t> originalRaw;
        std::vector<std::uint8_t> head;
        crypto::SaveVariant variant = crypto::SaveVariant::Yw1;
        Session session;
        std::string sourceLabel;

    private:
        SaveContext(SaveSource source, std::vector<std::uint8_t> originalRaw,
            std::vector<std::uint8_t> head, crypto::SaveVariant variant,
            Session session, std::string sourceLabel);
    };
}

#endif
