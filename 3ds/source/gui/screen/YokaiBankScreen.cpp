/* GPL-3.0-or-later */
#include "YokaiBankScreen.hpp"
#include "gui.hpp"
#include "ScreenStack.hpp"
#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

namespace
{
    constexpr const char* root = "/3ds/YKSM";

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

YokaiBankScreen::YokaiBankScreen(
    yokai::Game initialGame, std::size_t initialSource, bool useInstalledSave)
    : Screen("D-Pad Choose\nSELECT Change side\nA Transfer\nX Mark\nY Mark all\nL/R Bank page\nSTART Save")
{
    installedSaves = yokai::title::discover();
    activeGame = initialGame;
    sourceIndex = initialSource;
    this->useInstalledSave = useInstalledSave;
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
    if (!candidates.empty()) return candidates[sourceIndex % candidates.size()];
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
    gameSelection = 0;
    bankSelection = 0;
    bankPage = 0;
    try
    {
        std::vector<yokai::title::Location> matches;
        std::copy_if(installedSaves.begin(), installedSaves.end(), std::back_inserter(matches),
            [game](const auto& location) { return location.game == game; });
        if (useInstalledSave && !matches.empty())
            activeInstalledSave = matches[sourceIndex % matches.size()];
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
    if (gameRows.empty()) gameSelection = 0;
    else gameSelection = std::min(gameSelection, gameRows.size() - 1);
    if (!session || session->bank().empty())
    {
        bankSelection = 0;
        bankPage = 0;
    }
    else
    {
        bankPage = std::min(bankPage, bankPageCount() - 1);
        const std::size_t first = bankPage * 9;
        const std::size_t last = first + bankPageSize() - 1;
        bankSelection = std::clamp(bankSelection, first, last);
    }
}

std::size_t YokaiBankScreen::visibleCount() const
{
    if (!session) return 0;
    return pane == Pane::Game ? gameRows.size() : session->bank().size();
}

std::size_t YokaiBankScreen::bankPageCount() const
{
    return !session || session->bank().empty() ? 1 : (session->bank().size() + 8) / 9;
}

std::size_t YokaiBankScreen::bankPageSize() const
{
    if (!session || session->bank().empty()) return 0;
    return std::min<std::size_t>(9, session->bank().size() - bankPage * 9);
}

std::size_t YokaiBankScreen::selectedIndex() const
{
    return pane == Pane::Game ? gameSelection : bankSelection;
}

void YokaiBankScreen::drawTop() const
{
    const PKSM_Color navy(15, 22, 89, 255);
    Gui::drawSolidRect(0, 0, 400, 240, COLOR_WHITE);
    Gui::drawSolidRect(0, 0, 400, 28, COLOR_HIGHBLUE);
    const std::string title = "LOCAL BANK  |  Page " + std::to_string(bankPage + 1) + "/" +
        std::to_string(bankPageCount());
    Gui::text(title, 8, 7, FONT_SIZE_14, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
    Gui::text("L", 365, 7, FONT_SIZE_12, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
    Gui::text("R", 385, 7, FONT_SIZE_12, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
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

    const std::size_t start = bankPage * 9;
    for (std::size_t row = 0; row < 9; row++)
    {
        const std::size_t index = start + row;
        if (index >= session->bank().size()) break;
        const int y = 49 + row * 20;
        if (pane == Pane::Bank && index == bankSelection)
            Gui::drawSolidRect(2, y - 2, 396, 19, COLOR_LIGHTBLUE);
        const auto& entry = session->bank().entries()[index];
        const std::string name = entry.nickname.empty() ? entry.species : entry.nickname;
        const bool marked = markedBankIds.contains(entry.id);
        Gui::text(marked ? "[x]" : "[ ]", 6, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(shortened(name, 27), 32, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(std::to_string(entry.level), 260, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(std::to_string(entry.xp), 302, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
    }
    if (session->bank().empty())
        Gui::text("The local bank is empty", 200, 105, FONT_SIZE_15, navy,
            TextPosX::CENTER, TextPosY::TOP);
}

void YokaiBankScreen::drawBottom() const
{
    const PKSM_Color navy(15, 22, 89, 255);
    Gui::drawSolidRect(0, 0, 320, 240, navy);
    Gui::drawSolidRect(0, 0, 320, 34, COLOR_HIGHBLUE);
    Gui::text("SAVE YO-KAI  |  " + std::string(yokai::gameName(activeGame)), 160, 9,
        FONT_SIZE_14, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    if (session)
    {
        const std::size_t start = gameSelection / 7 * 7;
        for (std::size_t row = 0; row < 7; row++)
        {
            const std::size_t index = start + row;
            if (index >= gameRows.size()) break;
            const int y = 42 + static_cast<int>(row) * 20;
            if (pane == Pane::Game && index == gameSelection)
                Gui::drawSolidRect(3, y - 2, 314, 19, COLOR_LIGHTBLUE);
            const auto& record = gameRows[index];
            Gui::text(markedGameSlots.contains(record.slot) ? "[x]" : "[ ]", 7, y,
                FONT_SIZE_9, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
            Gui::text(shortened(record.displayName(), 22), 33, y, FONT_SIZE_9, COLOR_WHITE,
                TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SLICE, 185);
            Gui::text("Lv " + std::to_string(record.level), 231, y, FONT_SIZE_9,
                COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
        }
        if (gameRows.empty())
            Gui::text("No Yo-kai in this save", 160, 102, FONT_SIZE_14, COLOR_WHITE,
                TextPosX::CENTER, TextPosY::TOP);
    }
    Gui::text(shortened(status, 48), 8, 185, FONT_SIZE_9, COLOR_WHITE,
        TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SQUISH, 304);
    Gui::text(pane == Pane::Game ? "Selected: SAVE" : "Selected: BANK", 8, 203,
        FONT_SIZE_9, COLOR_YELLOW, TextPosX::LEFT, TextPosY::TOP);
    Gui::text("SELECT Side  A Transfer  L/R Bank page", 160, 222,
        FONT_SIZE_9, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
}

void YokaiBankScreen::toggleSelected()
{
    if (!session || visibleCount() == 0) return;
    const std::size_t index = selectedIndex();
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
            if (markedGameSlots.empty() && !gameRows.empty())
                markedGameSlots.insert(gameRows[gameSelection].slot);
            const auto selected = markedGameSlots;
            for (std::size_t slot : selected) { session->deposit(slot); moved++; }
            markedGameSlots.clear();
        }
        else
        {
            if (markedBankIds.empty() && !session->bank().empty())
                markedBankIds.insert(session->bank().entries()[bankSelection].id);
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

void YokaiBankScreen::update(touchPosition* touch)
{
    const u32 down = hidKeysDown();
    if (down & KEY_SELECT) pane = pane == Pane::Game ? Pane::Bank : Pane::Game;
    if (down & KEY_L)
    {
        bankPage = bankPage == 0 ? bankPageCount() - 1 : bankPage - 1;
        bankSelection = bankPage * 9;
        pane = Pane::Bank;
    }
    if (down & KEY_R)
    {
        bankPage = (bankPage + 1) % bankPageCount();
        bankSelection = bankPage * 9;
        pane = Pane::Bank;
    }
    if (down & KEY_UP && visibleCount())
    {
        if (pane == Pane::Game)
            gameSelection = gameSelection == 0 ? gameRows.size() - 1 : gameSelection - 1;
        else
        {
            const std::size_t first = bankPage * 9;
            bankSelection = bankSelection == first ? first + bankPageSize() - 1 : bankSelection - 1;
        }
    }
    if (down & KEY_DOWN && visibleCount())
    {
        if (pane == Pane::Game) gameSelection = (gameSelection + 1) % gameRows.size();
        else
        {
            const std::size_t first = bankPage * 9;
            bankSelection = first + (bankSelection - first + 1) % bankPageSize();
        }
    }
    if (down & KEY_X) toggleSelected();
    if (down & KEY_Y) selectAll();
    if (down & KEY_A) transfer();
    if (down & KEY_START) commit();
    if (down & KEY_B)
    {
        if (session && session->dirty()) discard();
        else ScreenStack::requestPop();
    }
    if (touch && (down & KEY_TOUCH))
    {
        if (touch->py >= 38 && touch->py < 180)
        {
            const std::size_t row = (touch->py - 38) / 20;
            const std::size_t index = gameSelection / 7 * 7 + row;
            if (index < gameRows.size())
            {
                gameSelection = index;
                pane = Pane::Game;
            }
        }
    }
}
