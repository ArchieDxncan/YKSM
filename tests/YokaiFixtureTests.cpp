#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
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
    std::cout << "Validated " << tested << " encrypted save fixtures\n";
}
