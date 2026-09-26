/* GPL-3.0-or-later */
#include "yokai/Session.hpp"
#include "yokai/Transfer.hpp"
#include <algorithm>

namespace yokai
{
    Session::Session(SaveImage save, Bank bank)
        : mOriginalSave(save), mOriginalBank(bank), mSave(std::move(save)), mBank(std::move(bank))
    {
    }

    std::uint64_t Session::deposit(std::size_t slot)
    {
        const auto records = mSave.records();
        const auto found = std::find_if(records.begin(), records.end(),
            [slot](const Record& record) { return record.slot == slot; });
        if (found == records.end()) throw Error("Selected save entry was not found");
        // Own the selected record before mutating the save. This mirrors
        // ArchieDxncan/ykw-bank's staged-transfer model and prevents any
        // view/iterator into a parsed record list from surviving mutation.
        return deposit(*found);
    }

    std::uint64_t Session::deposit(const Record& record)
    {
        const Record selected = record;
        if (mSave.isPartySlot(selected.slot))
            throw Error("Move this Yo-kai out of your active party first");
        // Mutate the save first. If its stale-record guard rejects the change,
        // no bank entry has been added.
        mSave.remove(selected.slot, selected.raw);
        try
        {
            const std::uint64_t id = mBank.append(mSave.game(), selected).id;
            mDirty = true;
            return id;
        }
        catch (...)
        {
            mSave.insert(selected.raw);
            throw;
        }
    }

    std::uint64_t Session::copyToBank(std::size_t slot)
    {
        const auto records = mSave.records();
        const auto found = std::find_if(records.begin(), records.end(),
            [slot](const Record& record) { return record.slot == slot; });
        if (found == records.end()) throw Error("Selected save entry was not found");
        return copyToBank(*found);
    }

    std::uint64_t Session::copyToBank(const Record& record)
    {
        const std::uint64_t id = mBank.append(mSave.game(), record).id;
        mDirty = true;
        return id;
    }

    std::size_t Session::withdraw(std::uint64_t bankId)
    {
        std::vector<std::uint8_t> example;
        const auto existing = mSave.records();
        if (!existing.empty()) example = existing.front().raw;
        return withdraw(bankId, example);
    }

    std::size_t Session::withdraw(
        std::uint64_t bankId, std::span<const std::uint8_t> destinationExample)
    {
        const BankEntry* selected = mBank.find(bankId);
        if (!selected) throw Error("Selected bank entry was not found");
        const BankEntry entry = *selected;
        if (!compatible(mSave.game(), entry))
            throw Error(entry.species + " is not compatible with " + std::string(gameName(mSave.game())));
        const auto record = convertRecord(entry, mSave.game(), destinationExample);
        const bool renumber = entry.sourceGame != mSave.game() ||
            mSave.containsIdentifier(std::span<const std::uint8_t>(record).first<4>());
        const std::size_t slot = mSave.insert(record, renumber);
        try
        {
            mBank.erase(bankId);
        }
        catch (...)
        {
            const auto inserted = mSave.records();
            const auto found = std::find_if(inserted.begin(), inserted.end(),
                [slot](const Record& item) { return item.slot == slot; });
            if (found != inserted.end()) mSave.remove(slot, found->raw);
            throw;
        }
        mDirty = true;
        return slot;
    }

    std::size_t Session::copyToSave(std::uint64_t bankId)
    {
        std::vector<std::uint8_t> example;
        const auto existing = mSave.records();
        if (!existing.empty()) example = existing.front().raw;
        return copyToSave(bankId, example);
    }

    std::size_t Session::copyToSave(
        std::uint64_t bankId, std::span<const std::uint8_t> destinationExample)
    {
        const BankEntry* selected = mBank.find(bankId);
        if (!selected) throw Error("Selected bank entry was not found");
        const BankEntry entry = *selected;
        if (!compatible(mSave.game(), entry))
            throw Error(entry.species + " is not compatible with " + std::string(gameName(mSave.game())));
        const auto record = convertRecord(entry, mSave.game(), destinationExample);
        // Copy always creates a new identity. This is essential when copying a
        // party member back into the save that already owns the source record.
        const std::size_t slot = mSave.insert(record, true);
        mDirty = true;
        return slot;
    }

    void Session::discard()
    {
        mSave = mOriginalSave;
        mBank = mOriginalBank;
        mDirty = false;
    }

    void Session::acceptCommitted()
    {
        mOriginalSave = mSave;
        mOriginalBank = mBank;
        mDirty = false;
    }
}
