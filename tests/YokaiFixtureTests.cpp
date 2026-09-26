#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
#include "yokai/Session.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>

namespace fs = std::filesystem;

static std::vector<std::uint8_t> readFile(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot open " + path.string());
    return {(std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>()};
}

static std::optional<yokai::Game> gameFor(const fs::path& relative)
{
    const std::string folder = relative.begin()->string();
    if (folder.find("B2") != std::string::npos) return yokai::Game::Busters2;
    if (folder.find("BLASTERS") != std::string::npos) return yokai::Game::Blasters;
    if (folder.find("WATCH 2") != std::string::npos) return yokai::Game::YW2;
    if (folder.find("Watch 3") != std::string::npos) return yokai::Game::YW3;
    if (folder == "YO-KAI WATCH") return yokai::Game::YW1;
    return std::nullopt;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: YokaiFixtureTests <extracted-save-root>\n";
        return 2;
    }
    const fs::path root = argv[1];
    std::size_t tested = 0;
    std::size_t reserveTransfers = 0;
    std::size_t partyFixtures = 0;
    std::size_t rubinyanPartyProtected = 0;
    try
    {
        for (const auto& entry : fs::recursive_directory_iterator(root))
        {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            if (!name.starts_with("game") || name.ends_with("_2.yw")) continue;
            if (!(name.ends_with(".yw") || name.ends_with(".yw_g"))) continue;
            const auto game = gameFor(fs::relative(entry.path(), root));
            if (!game) continue;

            const auto raw = readFile(entry.path());
            std::vector<std::uint8_t> head;
            if (*game != yokai::Game::YW1)
            {
                fs::path headPath = entry.path().parent_path() /
                    (name.ends_with(".yw_g") ? "head.yw_g" : "head.yw");
                if (!fs::exists(headPath)) headPath = entry.path().parent_path() / "head.yw";
                head = readFile(headPath);
            }
            const auto decrypted = yokai::crypto::decryptSave(*game, raw, head);
            const yokai::SaveImage image(*game, decrypted.bytes);
            const auto encrypted = yokai::crypto::encryptSave(*game, decrypted.bytes,
                decrypted.variant, head);
            if (encrypted != raw)
                throw std::runtime_error("Byte-exact round trip failed for " + entry.path().string());
            const auto second = yokai::crypto::decryptSave(*game, encrypted, head);
            if (second.bytes != decrypted.bytes)
                throw std::runtime_error("Decrypt-after-encrypt failed for " + entry.path().string());
            const auto originalRecords = image.records();
            const std::size_t expectedParty = std::min<std::size_t>(
                *game == yokai::Game::Blasters || *game == yokai::Game::Busters2 ? 4 : 6,
                originalRecords.size());
            const auto detectedParty = image.partySlots();
            if (detectedParty.size() != expectedParty)
                throw std::runtime_error("Party detection failed for " + entry.path().string());
            if (expectedParty)
            {
                const auto member = std::find_if(originalRecords.begin(), originalRecords.end(),
                    [&detectedParty](const auto& record) {
                        return std::find(detectedParty.begin(), detectedParty.end(), record.slot) !=
                            detectedParty.end();
                    });
                yokai::Session partyProbe(
                    yokai::SaveImage(*game, decrypted.bytes), yokai::Bank{});
                try
                {
                    (void)partyProbe.deposit(member->slot);
                    throw std::runtime_error("Party deposit was accepted for " +
                                             entry.path().string());
                }
                catch (const yokai::Error&)
                {
                    partyFixtures++;
                }
            }
            const auto reserve = std::find_if(originalRecords.begin(), originalRecords.end(),
                [&detectedParty](const auto& record) {
                    return std::find(detectedParty.begin(), detectedParty.end(), record.slot) ==
                        detectedParty.end();
                });
            if (reserve != originalRecords.end())
            {
                yokai::Session reserveProbe(
                    yokai::SaveImage(*game, decrypted.bytes), yokai::Bank{});
                const auto id = reserveProbe.deposit(reserve->slot);
                if (!reserveProbe.bank().find(id) ||
                    reserveProbe.save().records().size() + 1 != originalRecords.size() ||
                    reserveProbe.save().partySlots().size() != expectedParty)
                    throw std::runtime_error("Reserve transfer failed for " +
                                             entry.path().string());
                reserveTransfers++;
            }
            for (const auto& record : originalRecords)
            {
                if (*game != yokai::Game::YW1 || record.species != "Rubinyan") continue;
                yokai::Session transferProbe(
                    yokai::SaveImage(*game, decrypted.bytes), yokai::Bank{});
                if (image.isPartySlot(record.slot))
                {
                    try
                    {
                        (void)transferProbe.deposit(record.slot);
                        throw std::runtime_error("Party Rubinyan deposit was accepted for " +
                                                 entry.path().string());
                    }
                    catch (const yokai::Error&)
                    {
                        rubinyanPartyProtected++;
                    }
                    continue;
                }
                const auto id = transferProbe.deposit(record.slot);
                const auto bank = yokai::Bank::decode(transferProbe.bank().encode());
                if (!bank.find(id) || bank.find(id)->species != "Rubinyan")
                    throw std::runtime_error("Rubinyan bank round trip failed for " +
                                             entry.path().string());
                reserveTransfers++;
            }
            if (!originalRecords.empty())
            {
                yokai::SaveImage edited(*game, decrypted.bytes);
                edited.remove(originalRecords.front().slot, originalRecords.front().raw);
                const auto editedRaw = yokai::crypto::encryptSave(*game, edited.bytes(),
                    decrypted.variant, head);
                const auto editedPlain = yokai::crypto::decryptSave(*game, editedRaw, head);
                const yokai::SaveImage verified(*game, editedPlain.bytes);
                if (verified.records().size() + 1 != originalRecords.size())
                    throw std::runtime_error("Edited round trip failed for " + entry.path().string());
            }
            std::cout << yokai::gameName(*game) << ' ' << yokai::crypto::variantName(decrypted.variant)
                      << ' ' << image.records().size() << " records: " << name << '\n';
            tested++;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    if (!tested)
    {
        std::cerr << "No fixtures were found\n";
        return 1;
    }
    std::cout << "Validated " << tested << " encrypted save fixtures, " << partyFixtures
              << " party guards, " << reserveTransfers << " reserve transfers, and "
              << rubinyanPartyProtected << " protected party Rubinyan entries\n";
}
