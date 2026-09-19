/* GPL-3.0-or-later */
#ifndef YOKAI_SAVEIMAGE_HPP
#define YOKAI_SAVEIMAGE_HPP

#include "Yokai.hpp"
#include <span>
#include <stdexcept>
#include <vector>

namespace yokai
{
    class Error : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    // Mutable decrypted save image. Encryption and archive I/O are deliberately
    // outside this type so the parser can be host-tested byte-for-byte.
    class SaveImage
    {
    public:
        SaveImage(Game game, std::vector<std::uint8_t> bytes);

        [[nodiscard]] Game game() const { return mGame; }
        [[nodiscard]] const std::vector<std::uint8_t>& bytes() const { return mBytes; }
        [[nodiscard]] std::vector<Record> records() const;
        [[nodiscard]] bool hasFreeSlot() const;

        void remove(std::size_t slot, std::span<const std::uint8_t> expected);
        std::size_t insert(std::span<const std::uint8_t> record, bool assignNumbers = false);

    private:
        struct RecordArea
        {
            std::size_t offset;
            std::size_t slots;
        };

        [[nodiscard]] RecordArea recordArea() const;
        [[nodiscard]] std::size_t indexOffset() const;
        void syncIndex(std::size_t slot, std::span<const std::uint8_t> removedNumber = {});
        void assignNumbers(std::size_t absoluteRecordOffset);

        Game mGame;
        std::vector<std::uint8_t> mBytes;
    };
}

#endif
