/* GPL-3.0-or-later */
#ifndef YOKAI_SESSION_HPP
#define YOKAI_SESSION_HPP

#include "Bank.hpp"
#include "SaveImage.hpp"
#include <cstdint>
#include <span>

namespace yokai
{
    // A transfer session owns working copies of both sides. The originals are
    // untouched until the platform commit layer has safely written both.
    class Session
    {
    public:
        Session(SaveImage save, Bank bank);

        [[nodiscard]] const SaveImage& save() const { return mSave; }
        [[nodiscard]] SaveImage& save() { return mSave; }
        [[nodiscard]] const Bank& bank() const { return mBank; }
        [[nodiscard]] Bank& bank() { return mBank; }
        [[nodiscard]] bool dirty() const { return mDirty; }

        std::uint64_t deposit(std::size_t slot);
        std::uint64_t deposit(const Record& record);
        std::uint64_t copyToBank(std::size_t slot);
        std::uint64_t copyToBank(const Record& record);
        std::size_t withdraw(std::uint64_t bankId);
        std::size_t withdraw(
            std::uint64_t bankId, std::span<const std::uint8_t> destinationExample);
        std::size_t copyToSave(std::uint64_t bankId);
        std::size_t copyToSave(
            std::uint64_t bankId, std::span<const std::uint8_t> destinationExample);
        void discard();
        void acceptCommitted();

    private:
        SaveImage mOriginalSave;
        Bank mOriginalBank;
        SaveImage mSave;
        Bank mBank;
        bool mDirty = false;
    };
}

#endif
