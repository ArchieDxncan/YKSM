/* GPL-3.0-or-later */
#include "yokai/YokaiTitleSource.hpp"
#include "Archive.hpp"
#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
#include <array>
#include <cstdio>
#include <optional>

namespace yokai::title
{
    namespace
    {
        std::vector<std::uint8_t> readFrom(Archive& archive, const std::string& path)
        {
            auto file = archive.file(path, FS_OPEN_READ);
            if (!file || R_FAILED(file->result()) || file->size() > 0x400000) return {};
            std::vector<std::uint8_t> bytes(file->size());
            if (!bytes.empty() && file->read(bytes.data(), bytes.size()) != bytes.size()) return {};
            return bytes;
        }

        std::optional<Game> gameForSize(std::size_t size)
        {
            if (size == 38624) return Game::YW1;
            if (size == 71016 || size == 71084) return Game::YW2;
            if (size == 113856) return Game::YW3;
            if (size == 62524 || size == 64288) return Game::Blasters;
            if (size == 109616) return Game::Busters2;
            return std::nullopt;
        }

        void inspectTitle(std::vector<Location>& output, FS_MediaType media, std::uint64_t titleId)
        {
            Archive archive = Archive::save(media, static_cast<u32>(titleId),
                static_cast<u32>(titleId >> 32), false);
            if (R_FAILED(archive.result())) return;
            for (const char* saveName : {"/game1.yw", "/game1.yw_g", "/game2.yw", "/game3.yw"})
            {
                const auto raw = readFrom(archive, saveName);
                const auto game = gameForSize(raw.size());
                if (!game) continue;
                std::string headName;
                std::vector<std::uint8_t> head;
                if (*game != Game::YW1)
                {
                    headName = std::string(saveName).ends_with(".yw_g") ? "/head.yw_g" : "/head.yw";
                    head = readFrom(archive, headName);
                    if (head.empty() && headName == "/head.yw_g")
                    {
                        headName = "/head.yw";
                        head = readFrom(archive, headName);
                    }
                    if (head.empty()) continue;
                }
                try
                {
                    const auto decrypted = crypto::decryptSave(*game, raw, head);
                    SaveImage validated(*game, decrypted.bytes);
                    (void)validated;
                    output.push_back({*game, media, titleId, saveName, headName});
                }
                catch (const std::exception&)
                {
                    // A matching fixed size is not sufficient; authentication is authoritative.
                }
            }
        }
    }

    std::vector<Location> discover()
    {
        std::vector<Location> output;
        for (const FS_MediaType media : {MEDIATYPE_GAME_CARD, MEDIATYPE_SD})
        {
            u32 count = 0;
            if (R_FAILED(AM_GetTitleCount(media, &count)) || !count) continue;
            std::vector<u64> ids(count);
            if (R_FAILED(AM_GetTitleList(nullptr, media, count, ids.data()))) continue;
            for (const u64 id : ids)
            {
                if (static_cast<u32>(id >> 32) != 0x00040000) continue;
                inspectTitle(output, media, id);
            }
        }
        return output;
    }

    std::vector<std::uint8_t> read(const Location& location, const std::string& file)
    {
        Archive archive = Archive::save(location.media, static_cast<u32>(location.titleId),
            static_cast<u32>(location.titleId >> 32), false);
        if (R_FAILED(archive.result())) throw Error("Could not open the installed game's save archive");
        auto bytes = readFrom(archive, file);
        if (bytes.empty()) throw Error("Could not read " + file + " from the installed game");
        return bytes;
    }

    void write(const Location& location, std::span<const std::uint8_t> data)
    {
        Archive archive = Archive::save(location.media, static_cast<u32>(location.titleId),
            static_cast<u32>(location.titleId >> 32), false);
        if (R_FAILED(archive.result())) throw Error("Could not open the installed game's save archive");
        auto file = archive.file(location.saveFile, FS_OPEN_READ | FS_OPEN_WRITE);
        if (!file || R_FAILED(file->result())) throw Error("Could not open the installed game's save file");
        if (R_FAILED(file->resize(data.size()))) throw Error("Could not resize the installed game's save file");
        file->seek(0, SEEK_SET);
        if (file->write(data.data(), data.size()) != data.size()) throw Error("Could not write the installed game's save file");
        if (R_FAILED(archive.commit())) throw Error("Could not commit the installed game's save archive");
        file->close();
        archive.close();

        if (location.media == MEDIATYPE_SD)
        {
            u8 out = 0;
            const u64 secureValue = (static_cast<u64>(SECUREVALUE_SLOT_SD) << 32) |
                (static_cast<u32>(location.titleId) & 0xFFFFFF00);
            FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, &secureValue, 8, &out, 1);
        }
    }
}
