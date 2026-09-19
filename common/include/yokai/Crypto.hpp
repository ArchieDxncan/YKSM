/*
 * Level-5 Yo-kai cipher derived from togenyan/yw_save (MIT, 2016).
 * See THIRD_PARTY_LICENSES.md.
 */
#ifndef YOKAI_CRYPTO_HPP
#define YOKAI_CRYPTO_HPP

#include <cstdint>
#include <span>
#include <optional>
#include <array>
#include <vector>
#include "yokai/Yokai.hpp"

namespace yokai::crypto
{
    enum class SaveVariant : std::uint8_t
    {
        Yw1,
        Yw2,
        Yw2X,
        Yw3,
        Blasters1,
        Blasters2,
        Blasters3,
        Busters2Slot1,
        Busters2Slot2
    };

    struct DecryptedSave
    {
        std::vector<std::uint8_t> bytes;
        SaveVariant variant = SaveVariant::Yw1;
    };

    [[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> data);
    [[nodiscard]] bool hasValidYwChecksum(std::span<const std::uint8_t> data);
    [[nodiscard]] std::vector<std::uint8_t> decryptYw(std::span<const std::uint8_t> data);
    [[nodiscard]] std::vector<std::uint8_t> encryptYw(std::span<const std::uint8_t> data);
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> decryptCcmEnvelope(
        std::span<const std::uint8_t> data, std::span<const std::uint8_t, 16> key);
    [[nodiscard]] std::vector<std::uint8_t> encryptCcmEnvelope(
        std::span<const std::uint8_t> plain, std::span<const std::uint8_t, 16> key,
        std::span<const std::uint8_t, 12> nonce);
    [[nodiscard]] DecryptedSave decryptSave(Game game, std::span<const std::uint8_t> data,
        std::span<const std::uint8_t> head = {});
    [[nodiscard]] std::vector<std::uint8_t> encryptSave(Game game,
        std::span<const std::uint8_t> decrypted, SaveVariant variant,
        std::span<const std::uint8_t> head = {});
    [[nodiscard]] const char* variantName(SaveVariant variant);
}

#endif
