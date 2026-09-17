/* GPL-3.0-or-later */
#include "YokaiBankScreen.hpp"
#include "gui.hpp"
#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

namespace
{
    constexpr const char* root = "/3ds/YoKaiWatchBank";

    std::vector<std::uint8_t> readFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) throw yokai::Error("Could not open " + path.string());
        return {(std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>()};
    }

    void writeFile(const std::filesystem::path& path, std::span<const std::uint8_t> data)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream) throw yokai::Error("Could not create " + path.string());
        stream.write(reinterpret_cast<const char*>(data.data()), data.size());
        stream.flush();
        if (!stream) throw yokai::Error("Could not write " + path.string());
    }

    std::string shortened(std::string value, std::size_t limit)
    {
        if (value.size() > limit) value.resize(limit);
        return value;
    }
}

YokaiBankScreen::YokaiBankScreen()
    : Screen("A Transfer\nX Mark\nY Mark all\nL/R Game or Bank\nZL/ZR Save slot\nSELECT Change game\nSTART Save")
{
    installedSaves = yokai::title::discover();
    loadGame(activeGame);
}

std::filesystem::path YokaiBankScreen::savePath(yokai::Game game) const
{
    const auto directory = std::filesystem::path(root) / "saves" / std::string(yokai::gameName(game));
    std::vector<std::filesystem::path> candidates;
    for (const char* name : {"game1.yw", "game1.yw_g", "game2.yw", "game3.yw", "game.yw"})
    {
        const auto candidate = directory / name;
        if (std::filesystem::exists(candidate)) candidates.push_back(candidate);
    }
    if (!candidates.empty()) return candidates[sourceIndex[static_cast<std::size_t>(game)] % candidates.size()];
    return directory / "game1.yw";
}

void YokaiBankScreen::loadGame(yokai::Game game)
{
    activeGame = game;
    activeSavePath = savePath(game);
    activeInstalledSave.reset();
    activeOriginalRaw.clear();
    markedGameSlots.clear();
    markedBankIds.clear();
    activeHead.clear();
    hid.reset();
    try
    {
        std::vector<yokai::title::Location> matches;
        std::copy_if(installedSaves.begin(), installedSaves.end(), std::back_inserter(matches),
            [game](const auto& location) { return location.game == game; });
        if (!matches.empty())
            activeInstalledSave = matches[sourceIndex[static_cast<std::size_t>(game)] % matches.size()];
        const auto journal = std::filesystem::path(root) / "pending.commit";
        const auto bankPath = std::filesystem::path(root) / "bank.ykb";
        if (std::filesystem::exists(journal))
        {
            const auto journalBytes = readFile(journal);
            const std::filesystem::path pendingSave(std::string(journalBytes.begin(), journalBytes.end()));
            const auto saveBackup = pendingSave.string() + ".bak";
            const auto bankBackup = bankPath.string() + ".bak";
            if (std::filesystem::exists(saveBackup))
                std::filesystem::copy_file(saveBackup, pendingSave,
                    std::filesystem::copy_options::overwrite_existing);
            if (std::filesystem::exists(bankBackup))
                std::filesystem::copy_file(bankBackup, bankPath,
                    std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(journal);
        }
        const auto raw = activeInstalledSave
            ? yokai::title::read(*activeInstalledSave, activeInstalledSave->saveFile)
            : readFile(activeSavePath);
        activeOriginalRaw = raw;
        if (game != yokai::Game::YW1)
        {
            if (activeInstalledSave)
                activeHead = yokai::title::read(*activeInstalledSave, activeInstalledSave->headFile);
            else
            {
                const bool moonRabbit = activeSavePath.extension() == ".yw_g";
                auto headPath = activeSavePath.parent_path() / (moonRabbit ? "head.yw_g" : "head.yw");
                if (!std::filesystem::exists(headPath))
                    headPath = activeSavePath.parent_path() / (moonRabbit ? "head.yw" : "head.yw_g");
                activeHead = readFile(headPath);
            }
        }
        auto decrypted = yokai::crypto::decryptSave(game, raw, activeHead);
        activeVariant = decrypted.variant;
        yokai::Bank bank = yokai::Bank::load(bankPath);
        session = std::make_unique<yokai::Session>(
            yokai::SaveImage(game, std::move(decrypted.bytes)), std::move(bank));
        const std::string source = activeInstalledSave ? "installed title " + activeInstalledSave->saveFile
                                                       : activeSavePath.filename().string();
        status = "Loaded " + source + " (" +
            yokai::crypto::variantName(activeVariant) + ")";
        refresh();
    }
    catch (const std::exception& error)
    {
        session.reset();
        gameRows.clear();
        status = error.what();
    }
}

void YokaiBankScreen::refresh()
{
    gameRows = session ? session->save().records() : std::vector<yokai::Record>{};
    if (visibleCount() && hid.fullIndex() >= visibleCount()) hid.select(visibleCount() - 1, visibleCount());
}

std::size_t YokaiBankScreen::visibleCount() const
{
    if (!session) return 0;
    return pane == Pane::Game ? gameRows.size() : session->bank().size();
}

void YokaiBankScreen::drawTop() const
{
    Gui::drawSolidRect(0, 0, 400, 240, COLOR_WHITE);
    Gui::drawSolidRect(0, 0, 400, 28, COLOR_HIGHBLUE);
    const std::string title = std::string("Yo-kai Watch Bank  |  ") +
        (pane == Pane::Game ? "GAME: " : "BANK: ") + std::string(yokai::gameName(activeGame));
    Gui::text(title, 8, 7, FONT_SIZE_14, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
    Gui::text("Yo-kai", 32, 33, FONT_SIZE_11, COLOR_DARKGREY, TextPosX::LEFT, TextPosY::TOP);
    Gui::text("Lv", 258, 33, FONT_SIZE_11, COLOR_DARKGREY, TextPosX::LEFT, TextPosY::TOP);
    Gui::text("XP", 302, 33, FONT_SIZE_11, COLOR_DARKGREY, TextPosX::LEFT, TextPosY::TOP);

    if (!session)
    {
        Gui::text("No save loaded", 200, 90, FONT_SIZE_18, COLOR_UNSELECTRED, TextPosX::CENTER, TextPosY::TOP);
        Gui::text("Put a save at:", 200, 122, FONT_SIZE_12, COLOR_BLACK, TextPosX::CENTER, TextPosY::TOP);
        Gui::text(activeSavePath.string(), 200, 143, FONT_SIZE_9, COLOR_BLACK, TextPosX::CENTER, TextPosY::TOP);
        return;
    }

    const std::size_t start = hid.page() * hid.maxVisibleEntries();
    for (std::size_t row = 0; row < hid.maxVisibleEntries(); row++)
    {
        const std::size_t index = start + row;
        if (index >= visibleCount()) break;
        const int y = 49 + row * 20;
        if (row == hid.index()) Gui::drawSolidRect(2, y - 2, 396, 19, COLOR_LIGHTBLUE);
        std::string name;
        std::uint8_t level;
        std::uint32_t xp;
        bool marked;
        if (pane == Pane::Game)
        {
            const auto& record = gameRows[index];
            name = record.displayName(); level = record.level; xp = record.xp;
            marked = markedGameSlots.contains(record.slot);
        }
        else
        {
            const auto& entry = session->bank().entries()[index];
            name = entry.nickname.empty() ? entry.species : entry.nickname;
            level = entry.level; xp = entry.xp;
            marked = markedBankIds.contains(entry.id);
        }
        Gui::text(marked ? "[x]" : "[ ]", 6, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(shortened(name, 27), 32, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(std::to_string(level), 260, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(std::to_string(xp), 302, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
    }
}

void YokaiBankScreen::drawBottom() const
{
    Gui::drawSolidRect(0, 0, 320, 240, COLOR_MASKBLACK);
    Gui::drawSolidRect(0, 0, 320, 34, COLOR_HIGHBLUE);
    Gui::text(pane == Pane::Game ? "GAME SAVE LIST" : "LOCAL BANK LIST", 160, 9,
        FONT_SIZE_14, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    if (session)
    {
        Gui::text("Game: " + std::to_string(gameRows.size()) + "   Bank: " +
                std::to_string(session->bank().size()),
            12, 50, FONT_SIZE_12, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(session->dirty() ? "UNSAVED CHANGES" : "No pending changes", 12, 72,
            FONT_SIZE_12, session->dirty() ? COLOR_YELLOW : COLOR_LIGHTBLUE,
            TextPosX::LEFT, TextPosY::TOP);
    }
    Gui::text(shortened(status, 45), 12, 102, FONT_SIZE_9, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
    Gui::drawSolidRect(12, 150, 142, 34, pane == Pane::Game ? COLOR_HIGHBLUE : COLOR_DARKGREY);
    Gui::drawSolidRect(166, 150, 142, 34, pane == Pane::Bank ? COLOR_HIGHBLUE : COLOR_DARKGREY);
    Gui::text("Game", 83, 160, FONT_SIZE_12, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    Gui::text("Bank", 237, 160, FONT_SIZE_12, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    Gui::text("A Transfer   X Mark   Y All   START Save", 160, 211,
        FONT_SIZE_9, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
}

void YokaiBankScreen::toggleSelected()
{
    if (!session || visibleCount() == 0) return;
    const std::size_t index = hid.fullIndex();
    if (pane == Pane::Game)
    {
        const std::size_t slot = gameRows[index].slot;
        if (!markedGameSlots.erase(slot)) markedGameSlots.insert(slot);
    }
    else
    {
        const std::uint64_t id = session->bank().entries()[index].id;
        if (!markedBankIds.erase(id)) markedBankIds.insert(id);
    }
}

void YokaiBankScreen::selectAll()
{
    if (!session) return;
    if (pane == Pane::Game)
    {
        markedGameSlots.clear();
        for (const auto& record : gameRows) markedGameSlots.insert(record.slot);
    }
    else
    {
        markedBankIds.clear();
        for (const auto& entry : session->bank().entries()) markedBankIds.insert(entry.id);
    }
}

void YokaiBankScreen::transfer()
{
    if (!session) return;
    try
    {
        std::size_t moved = 0;
        if (pane == Pane::Game)
        {
            if (markedGameSlots.empty() && !gameRows.empty()) markedGameSlots.insert(gameRows[hid.fullIndex()].slot);
            const auto selected = markedGameSlots;
            for (std::size_t slot : selected) { session->deposit(slot); moved++; }
            markedGameSlots.clear();
        }
        else
        {
            if (markedBankIds.empty() && !session->bank().empty())
                markedBankIds.insert(session->bank().entries()[hid.fullIndex()].id);
            const auto selected = markedBankIds;
            for (std::uint64_t id : selected) { session->withdraw(id); moved++; }
            markedBankIds.clear();
        }
        status = "Staged " + std::to_string(moved) + " transfer(s)";
        refresh();
    }
    catch (const std::exception& error)
    {
        status = error.what();
        Gui::warn(status);
        refresh();
    }
}

void YokaiBankScreen::commit()
{
    if (!session || !session->dirty()) return;
    if (!Gui::showChoiceMessage("Write these changes to the game save and bank?")) return;
    const auto bankPath = std::filesystem::path(root) / "bank.ykb";
    const auto saveBackup = activeSavePath.string() + ".bak";
    const auto saveTemporary = activeSavePath.string() + ".tmp";
    const auto journal = std::filesystem::path(root) / "pending.commit";
    try
    {
        std::filesystem::create_directories(std::filesystem::path(root));
        if (activeInstalledSave)
        {
            const auto backupDirectory = std::filesystem::path(root) / "backups";
            std::filesystem::create_directories(backupDirectory);
            std::ostringstream name;
            name << std::hex << activeInstalledSave->titleId << '_' <<
                std::filesystem::path(activeInstalledSave->saveFile).filename().string() << ".bak";
            writeFile(backupDirectory / name.str(), activeOriginalRaw);
        }
        else
        {
            std::filesystem::copy_file(activeSavePath, saveBackup,
                std::filesystem::copy_options::overwrite_existing);
            const std::string journalText = activeSavePath.string();
            writeFile(journal, std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t*>(journalText.data()), journalText.size()));
        }
        const auto bytes = yokai::crypto::encryptSave(activeGame, session->save().bytes(),
            activeVariant, activeHead);
        if (activeInstalledSave)
            yokai::title::write(*activeInstalledSave, bytes);
        else
        {
            writeFile(saveTemporary, bytes);
            std::filesystem::remove(activeSavePath);
            std::filesystem::rename(saveTemporary, activeSavePath);
        }
        session->bank().saveAtomic(bankPath);
        if (!activeInstalledSave) std::filesystem::remove(journal);
        activeOriginalRaw = bytes;
        session->acceptCommitted();
        status = "Saved game and bank";
    }
    catch (const std::exception& error)
    {
        if (activeInstalledSave && !activeOriginalRaw.empty())
        {
            try { yokai::title::write(*activeInstalledSave, activeOriginalRaw); }
            catch (const std::exception&) {}
        }
        else if (std::filesystem::exists(saveBackup))
            std::filesystem::copy_file(saveBackup, activeSavePath,
                std::filesystem::copy_options::overwrite_existing);
        const auto bankBackup = bankPath.string() + ".bak";
        if (std::filesystem::exists(bankBackup))
            std::filesystem::copy_file(bankBackup, bankPath,
                std::filesystem::copy_options::overwrite_existing);
        status = error.what();
        Gui::warn("Save failed; game backup restored.\n" + status);
    }
}

void YokaiBankScreen::discard()
{
    if (!session || !session->dirty()) return;
    if (!Gui::showChoiceMessage("Discard all staged transfers?")) return;
    session->discard();
    markedGameSlots.clear(); markedBankIds.clear();
    status = "Pending transfers discarded";
    refresh();
}

void YokaiBankScreen::cycleGame()
{
    if (session && session->dirty())
    {
        status = "Save or discard changes before switching games";
        return;
    }
    const int next = (static_cast<int>(activeGame) + 1) % 5;
    loadGame(static_cast<yokai::Game>(next));
}

void YokaiBankScreen::cycleSave(int direction)
{
    if (session && session->dirty())
    {
        status = "Save or discard changes before switching save slots";
        return;
    }
    auto& index = sourceIndex[static_cast<std::size_t>(activeGame)];
    if (direction < 0 && index == 0) index = 4;
    else index = (index + direction + 5) % 5;
    loadGame(activeGame);
}

void YokaiBankScreen::update(touchPosition* touch)
{
    const u32 down = hidKeysDown();
    if (down & (KEY_L | KEY_R))
    {
        pane = pane == Pane::Game ? Pane::Bank : Pane::Game;
        hid.reset();
    }
    if (down & KEY_SELECT) cycleGame();
    if (down & KEY_ZL) cycleSave(-1);
    if (down & KEY_ZR) cycleSave(1);
    if (down & KEY_X) toggleSelected();
    if (down & KEY_Y) selectAll();
    if (down & KEY_A) transfer();
    if (down & KEY_START) commit();
    if (down & KEY_B)
    {
        if (session && session->dirty()) discard();
        else Gui::exitMainLoop();
    }
    if (touch && (down & KEY_TOUCH))
    {
        if (touch->py >= 150 && touch->py <= 184)
        {
            pane = touch->px < 160 ? Pane::Game : Pane::Bank;
            hid.reset();
        }
    }
    if (visibleCount()) hid.update(visibleCount());
}
