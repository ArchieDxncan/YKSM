/* GPL-3.0-or-later */
#include "YokaiBankScreen.hpp"
#include "gui.hpp"
#include "ScreenStack.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace
{
    constexpr const char* root = "/3ds/YKSM";

    void writeFile(const std::filesystem::path& path, std::span<const std::uint8_t> data)
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream) throw yokai::Error("Could not create " + path.string());
        stream.write(reinterpret_cast<const char*>(data.data()), data.size());
        stream.flush();
        if (!stream) throw yokai::Error("Could not write " + path.string());
    }

}

YokaiBankScreen::YokaiBankScreen(std::shared_ptr<yokai::SaveContext> context)
    : Screen("D-Pad Choose\nSELECT Change side\nA Transfer\nX Mark\nY Mark all\nL/R Bank page\nSTART Save"),
      context(std::move(context))
{
    session = this->context ? &this->context->session : nullptr;
    if (!this->context)
    {
        status = "No save loaded";
        return;
    }
    activeGame = this->context->source.game;
    activeSavePath = this->context->source.exported;
    status = "Loaded " + this->context->sourceLabel + " (" +
        yokai::crypto::variantName(this->context->variant) + ")";
    refresh();
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
    char title[48];
    std::snprintf(title, sizeof(title), "LOCAL BANK  |  Page %lu/%lu",
        static_cast<unsigned long>(bankPage + 1), static_cast<unsigned long>(bankPageCount()));
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
        const std::string& name = entry.nickname.empty() ? entry.species : entry.nickname;
        const bool marked = markedBankIds.contains(entry.id);
        char level[4];
        char xp[12];
        std::snprintf(level, sizeof(level), "%u", static_cast<unsigned>(entry.level));
        std::snprintf(xp, sizeof(xp), "%lu", static_cast<unsigned long>(entry.xp));
        Gui::text(marked ? "[x]" : "[ ]", 6, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(name, 32, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP,
            TextWidthAction::SLICE, 215);
        Gui::text(level, 260, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(xp, 302, y, FONT_SIZE_11, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
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
    char saveTitle[40];
    std::snprintf(saveTitle, sizeof(saveTitle), "SAVE YO-KAI  |  %s",
        yokai::gameName(activeGame).data());
    Gui::text(saveTitle, 160, 9,
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
            char level[8];
            std::snprintf(level, sizeof(level), "Lv %u", static_cast<unsigned>(record.level));
            Gui::text(markedGameSlots.contains(record.slot) ? "[x]" : "[ ]", 7, y,
                FONT_SIZE_9, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
            Gui::text(record.displayName(), 33, y, FONT_SIZE_9, COLOR_WHITE,
                TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SLICE, 185);
            Gui::text(level, 231, y, FONT_SIZE_9,
                COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
        }
        if (gameRows.empty())
            Gui::text("No Yo-kai in this save", 160, 102, FONT_SIZE_14, COLOR_WHITE,
                TextPosX::CENTER, TextPosY::TOP);
    }
    Gui::text(status, 8, 185, FONT_SIZE_9, COLOR_WHITE,
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
            std::vector<yokai::Record> selected;
            selected.reserve(markedGameSlots.size());
            for (const auto& record : gameRows)
                if (markedGameSlots.contains(record.slot)) selected.push_back(record);
            for (const auto& record : selected)
            {
                Logging::info("YKSM deposit: game {}, slot {}", yokai::gameName(activeGame), record.slot);
                session->deposit(record);
                moved++;
            }
            markedGameSlots.clear();
        }
        else
        {
            if (markedBankIds.empty() && !session->bank().empty())
                markedBankIds.insert(session->bank().entries()[bankSelection].id);
            const auto selected = markedBankIds;
            const std::vector<std::uint8_t> example =
                gameRows.empty() ? std::vector<std::uint8_t>{} : gameRows.front().raw;
            for (std::uint64_t id : selected)
            {
                session->withdraw(id, example);
                moved++;
            }
            markedBankIds.clear();
        }
        status = "Staged " + std::to_string(moved) + " transfer(s)";
        refresh();
    }
    catch (const std::exception& error)
    {
        status = error.what();
        Logging::error("YKSM transfer failed: {}", status);
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
        Logging::info("YKSM commit: begin for {}", yokai::gameName(activeGame));
        std::filesystem::create_directories(std::filesystem::path(root));
        if (context->source.installed)
        {
            const auto backupDirectory = std::filesystem::path(root) / "backups";
            std::filesystem::create_directories(backupDirectory);
            std::ostringstream name;
            name << std::hex << context->source.installed->titleId << '_' <<
                std::filesystem::path(context->source.installed->saveFile).filename().string() << ".bak";
            writeFile(backupDirectory / name.str(), context->originalRaw);
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
            context->variant, context->head);
        Logging::info("YKSM commit: encrypted {} bytes", bytes.size());
        if (context->source.installed)
        {
            yokai::title::write(*context->source.installed, bytes);
            Logging::info("YKSM commit: installed save written");
        }
        else
        {
            writeFile(saveTemporary, bytes);
            std::filesystem::remove(activeSavePath);
            std::filesystem::rename(saveTemporary, activeSavePath);
        }
        session->bank().saveAtomic(bankPath);
        Logging::info("YKSM commit: bank written with {} entries", session->bank().size());
        if (!context->source.installed) std::filesystem::remove(journal);
        context->originalRaw = bytes;
        session->acceptCommitted();
        status = "Saved game and bank";
    }
    catch (const std::exception& error)
    {
        if (context->source.installed && !context->originalRaw.empty())
        {
            try { yokai::title::write(*context->source.installed, context->originalRaw); }
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
        Logging::error("YKSM commit failed after backup restore: {}", status);
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
