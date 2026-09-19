/* GPL-3.0-or-later */
#include "yokai/YokaiSaveContext.hpp"
#include <fstream>
#include <iterator>
#include <utility>

namespace
{
    constexpr const char* root = "/3ds/YKSM";

    std::vector<std::uint8_t> readFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) throw yokai::Error("Could not open " + path.string());
        return {(std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>()};
    }

    void recoverInterruptedExportCommit(const std::filesystem::path& bankPath)
    {
        const auto journal = std::filesystem::path(root) / "pending.commit";
        if (!std::filesystem::exists(journal)) return;
        const auto bytes = readFile(journal);
        const std::filesystem::path savePath(std::string(bytes.begin(), bytes.end()));
        const auto saveBackup = savePath.string() + ".bak";
        const auto bankBackup = bankPath.string() + ".bak";
        if (std::filesystem::exists(saveBackup))
            std::filesystem::copy_file(saveBackup, savePath,
                std::filesystem::copy_options::overwrite_existing);
        if (std::filesystem::exists(bankBackup))
            std::filesystem::copy_file(bankBackup, bankPath,
                std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(journal);
    }
}

namespace yokai
{
    SaveContext::SaveContext(SaveSource source, std::vector<std::uint8_t> originalRaw,
        std::vector<std::uint8_t> head, crypto::SaveVariant variant,
        Session session, std::string sourceLabel)
        : source(std::move(source)), originalRaw(std::move(originalRaw)), head(std::move(head)),
          variant(variant), session(std::move(session)), sourceLabel(std::move(sourceLabel))
    {
    }

    std::shared_ptr<SaveContext> SaveContext::load(SaveSource source)
    {
        const auto bankPath = std::filesystem::path(root) / "bank.ykb";
        recoverInterruptedExportCommit(bankPath);

        std::vector<std::uint8_t> raw;
        std::vector<std::uint8_t> head;
        std::string label;
        if (source.installed)
        {
            raw = title::read(*source.installed, source.installed->saveFile);
            if (source.game != Game::YW1)
                head = title::read(*source.installed, source.installed->headFile);
            label = source.installed->saveFile.substr(1);
        }
        else
        {
            raw = readFile(source.exported);
            if (source.game != Game::YW1)
            {
                const bool moonRabbit = source.exported.extension() == ".yw_g";
                auto headPath = source.exported.parent_path() /
                    (moonRabbit ? "head.yw_g" : "head.yw");
                if (!std::filesystem::exists(headPath))
                    headPath = source.exported.parent_path() /
                        (moonRabbit ? "head.yw" : "head.yw_g");
                head = readFile(headPath);
            }
            label = source.exported.filename().string();
        }

        auto decrypted = crypto::decryptSave(source.game, raw, head);
        Session session(SaveImage(source.game, std::move(decrypted.bytes)), Bank::load(bankPath));
        return std::shared_ptr<SaveContext>(new SaveContext(std::move(source), std::move(raw),
            std::move(head), decrypted.variant, std::move(session), std::move(label)));
    }
}
