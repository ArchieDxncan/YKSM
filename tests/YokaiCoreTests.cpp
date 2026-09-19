#include "yokai/Bank.hpp"
#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
#include "yokai/Session.hpp"
#include "yokai/Species.hpp"
#include "yokai/Transfer.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    void write32(std::vector<std::uint8_t>& data, std::size_t offset, std::uint32_t value)
    {
        for (int index = 0; index < 4; index++)
            data[offset + index] = static_cast<std::uint8_t>(value >> (index * 8));
    }

    std::vector<std::uint8_t> yw1Record(std::uint32_t id)
    {
        std::vector<std::uint8_t> record(0x5C);
        write32(record, 4, id);
        std::copy_n("Buddy", 5, record.begin() + 8);
        write32(record, 0x38, 1234);
        const std::uint8_t ivs[] = {1, 2, 3, 1, 3};
        std::copy(std::begin(ivs), std::end(ivs), record.begin() + 0x45);
        record[0x54] = 27;
        record[0x55] = 4;
        record[0x57] = 1;
        record[0x58] = 42;
        record[0x5A] = 42;
        return record;
    }

    std::vector<std::uint8_t> yw1Save(const std::vector<std::uint8_t>& record)
    {
        std::vector<std::uint8_t> save(0x1D08 + 240 * 0x5C);
        std::copy_n("Nathan", 6, save.begin() + 0x28);
        write32(save, 0x60, 60 * 60 * 60);
        std::copy(record.begin(), record.end(), save.begin() + 0x1D08);
        std::copy_n(record.begin(), 4, save.begin() + 0x73DC);
        return save;
    }

    void append32(std::vector<std::uint8_t>& data, std::uint32_t value)
    {
        const std::size_t offset = data.size();
        data.resize(offset + 4);
        write32(data, offset, value);
    }

    void appendSection(std::vector<std::uint8_t>& body, std::uint8_t id,
        const std::vector<std::uint8_t>& payload)
    {
        append32(body, 0x0000FFFE);
        append32(body, static_cast<std::uint32_t>(payload.size() << 8) | id);
        body.insert(body.end(), payload.begin(), payload.end());
        append32(body, 0x0000FEFF);
    }

    std::vector<std::uint8_t> nativeYw3Save(std::uint32_t id)
    {
        std::vector<std::uint8_t> records(656 * 0x54);
        write32(records, 4, id);
        std::copy_n("Native", 6, records.begin() + 8);
        write32(records, 0x28, 5678);
        const std::uint8_t ivs[] = {16, 8, 8, 8, 8};
        std::copy(std::begin(ivs), std::end(ivs), records.begin() + 0x34);
        records[0x49] = 44;
        std::vector<std::uint8_t> indexes(656 * 4);
        std::copy_n(records.begin(), 4, indexes.begin());
        std::vector<std::uint8_t> body;
        appendSection(body, 0x07, records);
        appendSection(body, 0x0A, indexes);
        std::vector<std::uint8_t> save(0x20);
        save.insert(save.end(), body.begin(), body.end());
        save.resize(save.size() + 8);
        return save;
    }

    std::vector<std::uint8_t> fromHex(std::string_view text)
    {
        const auto digit = [](char value)
        {
            return value <= '9' ? value - '0' : value - 'a' + 10;
        };
        std::vector<std::uint8_t> output;
        for (std::size_t index = 0; index < text.size(); index += 2)
            output.push_back(static_cast<std::uint8_t>((digit(text[index]) << 4) | digit(text[index + 1])));
        return output;
    }
}

int main()
{
    using namespace yokai;
    const auto jibanyan = speciesId(Game::YW1, "Jibanyan");
    assert(jibanyan);
    assert(speciesName(Game::YW1, *jibanyan) == "Jibanyan");
    assert(transferSpeciesId(Game::YW1, Game::YW3, "Jibanyan"));

    const auto originalRecord = yw1Record(*jibanyan);
    SaveImage save(Game::YW1, yw1Save(originalRecord));
    assert(save.playerName() == "Nathan");
    assert(save.playTimeSeconds() == 3600);
    auto records = save.records();
    assert(records.size() == 1);
    assert(records[0].slot == 0);
    assert(records[0].nickname == "Buddy");
    assert(records[0].level == 27);
    assert(records[0].xp == 1234);

    Bank bank;
    const std::uint64_t bankId = bank.append(Game::YW1, records[0]).id;
    auto encoded = bank.encode();
    Bank decoded = Bank::decode(encoded);
    assert(decoded.size() == 1);
    assert(decoded.find(bankId));
    assert(decoded.entries()[0].nickname == "Buddy");
    auto corrupted = encoded;
    corrupted.back() ^= 0xFF;
    try
    {
        (void)Bank::decode(corrupted);
        assert(false && "corrupt bank accepted");
    }
    catch (const Error&)
    {
    }

    save.remove(0, originalRecord);
    assert(save.records().empty());
    const std::size_t restoredSlot = save.insert(originalRecord);
    assert(restoredSlot == 0);
    assert(save.records()[0].raw == originalRecord);

    const BankEntry& entry = decoded.entries()[0];
    auto converted = convertRecord(entry, Game::YW3);
    assert(converted.size() == 0x54);
    assert(converted[0x49] == 27);
    assert(converted[0x34] == 8);
    assert(converted[0x35] == 8);
    assert(converted[0x36] == 12);
    assert(converted[0x37] == 4);
    assert(converted[0x38] == 12);
    validateRecord(Game::YW3, converted);

    const auto yw3Jibanyan = speciesId(Game::YW3, "Jibanyan");
    assert(yw3Jibanyan);
    SaveImage native(Game::YW3, nativeYw3Save(*yw3Jibanyan));
    const auto nativeRows = native.records();
    assert(nativeRows.size() == 1);
    assert(nativeRows[0].nickname == "Native");
    assert(nativeRows[0].level == 44);
    native.remove(0, nativeRows[0].raw);
    assert(native.records().empty());
    assert(native.insert(nativeRows[0].raw) == 0);

    Bank arrival;
    Record second = records[0];
    second.nickname = "Second";
    arrival.append(Game::YW1, records[0]);
    arrival.append(Game::YW1, second);
    auto roundTrip = Bank::decode(arrival.encode());
    assert(roundTrip.entries()[0].nickname == "Buddy");
    assert(roundTrip.entries()[1].nickname == "Second");

    Session session(SaveImage(Game::YW1, yw1Save(originalRecord)), Bank{});
    const std::uint64_t staged = session.deposit(0);
    assert(session.dirty());
    assert(session.save().records().empty());
    assert(session.bank().size() == 1);
    assert(session.withdraw(staged) == 0);
    assert(session.save().records().size() == 1);

    auto plain = yw1Save(originalRecord);
    write32(plain, plain.size() - 4, 0x12345678);
    const auto encrypted = crypto::encryptYw(plain);
    assert(crypto::hasValidYwChecksum(encrypted));
    const auto decrypted = crypto::decryptYw(encrypted);
    assert(std::equal(decrypted.begin(), decrypted.end() - 8, plain.begin()));
    assert(std::equal(decrypted.end() - 4, decrypted.end(), plain.end() - 4));
    assert(crypto::encryptYw(decrypted) == encrypted);

    const auto ccmVector = fromHex(
        "000102030405060708090a0b00000000212ea1e87bc4e16585d410cee5763dd6"
        "838b79e1326b7961cb06e4093aa9ad20af45be41a8f126871003c0e81969d2e5"
        "1865866ca6d59c084006d7a92b9822d9cf");
    const std::array<std::uint8_t, 16> ccmKey = {
        '5','+','N','I','8','W','V','q','0','9','V','7','L','I','5','w'};
    const auto ccmPlain = crypto::decryptCcmEnvelope(ccmVector, ccmKey);
    assert(ccmPlain && ccmPlain->size() == 49);
    for (std::size_t index = 0; index < ccmPlain->size(); index++)
        assert((*ccmPlain)[index] == index + 1);
    std::array<std::uint8_t, 12> ccmNonce{};
    for (std::size_t index = 0; index < ccmNonce.size(); index++) ccmNonce[index] = index;
    assert(crypto::encryptCcmEnvelope(*ccmPlain, ccmKey, ccmNonce) == ccmVector);
    assert(session.bank().empty());
    session.discard();
    assert(!session.dirty());
    assert(session.save().records().size() == 1);

    std::cout << "Yo-kai core tests passed\n";
}
