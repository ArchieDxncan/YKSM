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
        const Record selected = *found;
        // Mutate the save first. If its stale-record guard rejects the change,
        // no bank entry has been added.
        mSave.remove(slot, selected.raw);
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

    std::size_t Session::withdraw(std::uint64_t bankId)
    {
        const BankEntry* selected = mBank.find(bankId);
        if (!selected) throw Error("Selected bank entry was not found");
        const BankEntry entry = *selected;
        if (!compatible(mSave.game(), entry))
            throw Error(entry.species + " is not compatible with " + std::string(gameName(mSave.game())));
        std::span<const std::uint8_t> example;
        const auto existing = mSave.records();
        if (!existing.empty()) example = existing.front().raw;
        const auto record = convertRecord(entry, mSave.game(), example);
        const std::size_t slot = mSave.insert(record, entry.sourceGame != mSave.game());
        try
        {
            mBank.erase(bankId);
        }
        catch (...)
        {
            mSave.remove(slot, record);
            throw;
        }
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
