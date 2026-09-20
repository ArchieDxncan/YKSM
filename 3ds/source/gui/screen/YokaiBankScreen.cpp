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

    const PKSM_Color orange(225, 151, 38, 255);
    const PKSM_Color headerBlue(36, 75, 126, 255);
    const PKSM_Color rowBlue(183, 225, 225, 255);
    const PKSM_Color rowPink(245, 191, 194, 255);
    const PKSM_Color ink(46, 55, 62, 255);

    void pill(float x, float y, float width, float height, PKSM_Color color)
    {
        const float radius = height / 2;
        Gui::drawSolidRect(x + radius, y, width - height, height, color);
        Gui::drawSolidCircle(x + radius, y + radius, radius, color);
        Gui::drawSolidCircle(x + width - radius, y + radius, radius, color);
    }

    void mark(float x, float y, bool selected)
    {
        const PKSM_Color border(91, 73, 45, 255);
        Gui::drawSolidCircle(x + 6, y + 6, 7, border);
        Gui::drawSolidCircle(x + 6, y + 6, 5, selected ? PKSM_Color(244, 176, 42, 255) : COLOR_WHITE);
        if (selected)
        {
            Gui::drawLine(x + 3, y + 6, x + 5, y + 9, 2, COLOR_WHITE);
            Gui::drawLine(x + 5, y + 9, x + 10, y + 3, 2, COLOR_WHITE);
        }
    }

    void bottomButton(float x, PKSM_Color color, const char* key, const char* label)
    {
        Gui::drawSolidRect(x, 208, 79, 32, color);
        Gui::text(key, x + 8, 211, FONT_SIZE_9, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(label, x + 42, 219, FONT_SIZE_12, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    }

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
    : Screen("D-Pad Choose\nSELECT Change side\nA Menu\nY Sort\nX Save\nL/R Bank page"),
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
    bankRows.clear();
    if (session)
        for (std::size_t index = 0; index < session->bank().size(); index++) bankRows.push_back(index);
    applySort();
    if (gameRows.empty()) gameSelection = 0;
    else gameSelection = std::min(gameSelection, gameRows.size() - 1);
    if (!session || bankRows.empty())
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

void YokaiBankScreen::applySort()
{
    if (gameSort == SortMode::Original)
        std::sort(gameRows.begin(), gameRows.end(), [](const auto& a, const auto& b) { return a.slot < b.slot; });
    else if (gameSort == SortMode::Name)
        std::stable_sort(gameRows.begin(), gameRows.end(), [](const auto& a, const auto& b) {
            return a.displayName() < b.displayName();
        });
    else
        std::stable_sort(gameRows.begin(), gameRows.end(), [](const auto& a, const auto& b) {
            return a.level > b.level;
        });

    const SortMode mode = bankSort;
    std::stable_sort(bankRows.begin(), bankRows.end(), [this, mode](auto left, auto right) {
        const auto* a = &session->bank().entries()[left];
        const auto* b = &session->bank().entries()[right];
        if (mode == SortMode::Original) return a->arrival < b->arrival;
        if (mode == SortMode::Level) return a->level > b->level;
        const std::string& an = a->nickname.empty() ? a->species : a->nickname;
        const std::string& bn = b->nickname.empty() ? b->species : b->nickname;
        return an < bn;
    });
}

void YokaiBankScreen::sortActive()
{
    SortMode& mode = pane == Pane::Game ? gameSort : bankSort;
    const auto next = [](SortMode current) {
        return current == SortMode::Original ? SortMode::Name :
            current == SortMode::Name ? SortMode::Level : SortMode::Original;
    };
    const std::size_t gameSlot = gameRows.empty() ? 0 : gameRows[gameSelection].slot;
    const auto* selectedBankEntry = bankEntry(bankSelection);
    const std::uint64_t bankId = selectedBankEntry ? selectedBankEntry->id : 0;
    mode = next(mode);
    applySort();
    if (pane == Pane::Game)
    {
        const auto found = std::find_if(gameRows.begin(), gameRows.end(),
            [gameSlot](const auto& row) { return row.slot == gameSlot; });
        if (found != gameRows.end()) gameSelection = found - gameRows.begin();
    }
    else
    {
        const auto found = std::find_if(bankRows.begin(), bankRows.end(), [this, bankId](auto index) {
            return session->bank().entries()[index].id == bankId;
        });
        if (found != bankRows.end()) bankSelection = found - bankRows.begin();
        bankPage = bankSelection / 9;
    }
    const char* label = mode == SortMode::Original ? "original order" :
        mode == SortMode::Name ? "name" : "level";
    status = std::string("Sorted by ") + label;
}

std::size_t YokaiBankScreen::visibleCount() const
{
    if (!session) return 0;
    return pane == Pane::Game ? gameRows.size() : bankRows.size();
}

std::size_t YokaiBankScreen::bankPageCount() const
{
    return !session || bankRows.empty() ? 1 : (bankRows.size() + 8) / 9;
}

std::size_t YokaiBankScreen::bankPageSize() const
{
    if (!session || bankRows.empty()) return 0;
    return std::min<std::size_t>(9, bankRows.size() - bankPage * 9);
}

const yokai::BankEntry* YokaiBankScreen::bankEntry(std::size_t viewIndex) const
{
    return !session || viewIndex >= bankRows.size() ? nullptr :
        &session->bank().entries()[bankRows[viewIndex]];
}

std::size_t YokaiBankScreen::selectedIndex() const
{
    return pane == Pane::Game ? gameSelection : bankSelection;
}

void YokaiBankScreen::drawTop() const
{
    Gui::drawSolidRect(0, 0, 400, 240, orange);
    for (int y = 0; y < 240; y += 12)
        Gui::drawSolidRect(0, y, 400, 1, PKSM_Color(236, 173, 59, 255));
    Gui::drawSolidRect(0, 0, 400, 31, headerBlue);
    Gui::drawSolidRect(0, 27, 400, 4, PKSM_Color(63, 183, 208, 255));
    char title[48];
    std::snprintf(title, sizeof(title), "Local Bank  %lu/%lu",
        static_cast<unsigned long>(bankPage + 1), static_cast<unsigned long>(bankPageCount()));
    Gui::text(title, 10, 7, FONT_SIZE_14, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
    pill(330, 4, 61, 22, PKSM_Color(50, 57, 74, 255));
    Gui::text("L / R", 360, 9, FONT_SIZE_9, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);

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
        const auto* entry = bankEntry(index);
        if (!entry) break;
        const int y = 37 + row * 21;
        const bool selected = pane == Pane::Bank && index == bankSelection;
        pill(8, y, 384, 18, selected ? rowPink : rowBlue);
        const std::string& name = entry->nickname.empty() ? entry->species : entry->nickname;
        const bool marked = markedBankIds.contains(entry->id);
        char level[4];
        std::snprintf(level, sizeof(level), "%u", static_cast<unsigned>(entry->level));
        mark(14, y + 3, marked);
        Gui::text(name, 36, y + 3, FONT_SIZE_11, ink, TextPosX::LEFT, TextPosY::TOP,
            TextWidthAction::SLICE, 280);
        Gui::text("Lv", 330, y + 3, FONT_SIZE_9, headerBlue, TextPosX::LEFT, TextPosY::TOP);
        Gui::text(level, 363, y + 2, FONT_SIZE_11, ink, TextPosX::CENTER, TextPosY::TOP);
    }
    if (bankRows.empty())
        Gui::text("The local bank is empty", 200, 105, FONT_SIZE_15, ink,
            TextPosX::CENTER, TextPosY::TOP);
}

void YokaiBankScreen::drawBottom() const
{
    Gui::drawSolidRect(0, 0, 320, 240, orange);
    for (int y = 0; y < 208; y += 12)
        Gui::drawSolidRect(0, y, 320, 1, PKSM_Color(236, 173, 59, 255));
    Gui::drawSolidRect(0, 0, 320, 34, headerBlue);
    Gui::drawSolidRect(0, 29, 320, 5, PKSM_Color(63, 183, 208, 255));
    Gui::text("Change Members", 8, 7, FONT_SIZE_14, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
    pill(220, 5, 94, 24, PKSM_Color(50, 57, 74, 255));
    Gui::text(pane == Pane::Game ? "Save" : "Bank", 267, 10, FONT_SIZE_11,
        COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    if (session)
    {
        const std::size_t start = gameSelection / 6 * 6;
        for (std::size_t row = 0; row < 6; row++)
        {
            const std::size_t index = start + row;
            if (index >= gameRows.size()) break;
            const int y = 40 + static_cast<int>(row) * 23;
            const bool selected = pane == Pane::Game && index == gameSelection;
            pill(8, y, 304, 20, selected ? rowPink : rowBlue);
            const auto& record = gameRows[index];
            char level[8];
            std::snprintf(level, sizeof(level), "Lv %u", static_cast<unsigned>(record.level));
            mark(14, y + 4, markedGameSlots.contains(record.slot));
            Gui::text(record.displayName(), 36, y + 4, FONT_SIZE_11, ink,
                TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SLICE, 205);
            Gui::text(level, 274, y + 4, FONT_SIZE_9, headerBlue,
                TextPosX::CENTER, TextPosY::TOP);
        }
        if (gameRows.empty())
            Gui::text("No Yo-kai in this save", 160, 102, FONT_SIZE_14, ink,
                TextPosX::CENTER, TextPosY::TOP);
    }
    Gui::drawSolidRect(4, 181, 312, 23, PKSM_Color(255, 244, 207, 235));
    Gui::text(status, 9, 187, FONT_SIZE_9, ink,
        TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SQUISH, 302);
    bottomButton(0, PKSM_Color(174, 132, 70, 255), "B", "Back");
    bottomButton(80, PKSM_Color(66, 167, 70, 255), "Y", "Sort");
    bottomButton(160, PKSM_Color(56, 115, 206, 255), "X", "Save");
    bottomButton(240, PKSM_Color(205, 76, 81, 255), "A", "Menu");

    if (menuOpen)
    {
        Gui::drawSolidRect(137, 63, 176, 105, PKSM_Color(91, 66, 43, 255));
        Gui::drawSolidRect(140, 66, 170, 99, PKSM_Color(255, 244, 207, 255));
        Gui::text("What'll you do?", 225, 73, FONT_SIZE_12, ink,
            TextPosX::CENTER, TextPosY::TOP);
        pill(150, 99, 150, 25, menuSelection == 0 ? rowPink : rowBlue);
        pill(150, 132, 150, 25, menuSelection == 1 ? rowPink : rowBlue);
        Gui::text("Mark", 225, 106, FONT_SIZE_11, ink, TextPosX::CENTER, TextPosY::TOP);
        Gui::text("Move", 225, 139, FONT_SIZE_11, ink, TextPosX::CENTER, TextPosY::TOP);
    }
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
        const std::uint64_t id = bankEntry(index)->id;
        if (!markedBankIds.erase(id)) markedBankIds.insert(id);
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
                if (session->save().isPartySlot(record.slot))
                    throw yokai::Error("Move party Yo-kai to reserve slots before banking");
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
            if (markedBankIds.empty() && !bankRows.empty())
                markedBankIds.insert(bankEntry(bankSelection)->id);
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
    if (menuOpen)
    {
        if (down & (KEY_UP | KEY_DOWN)) menuSelection = 1 - menuSelection;
        if (down & KEY_B) menuOpen = false;
        if (touch && (down & KEY_TOUCH))
        {
            if (touch->px >= 150 && touch->px < 300 && touch->py >= 99 && touch->py < 124)
                menuSelection = 0;
            else if (touch->px >= 150 && touch->px < 300 && touch->py >= 132 && touch->py < 157)
                menuSelection = 1;
            else return;
        }
        if (down & (KEY_A | KEY_TOUCH))
        {
            menuOpen = false;
            if (menuSelection == 0) toggleSelected();
            else transfer();
        }
        return;
    }
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
    if (down & KEY_Y) sortActive();
    if (down & KEY_X) commit();
    if (down & KEY_A) menuOpen = true;
    if (down & KEY_B)
    {
        if (session && session->dirty()) discard();
        else ScreenStack::requestPop();
    }
    if (touch && (down & KEY_TOUCH))
    {
        if (touch->py >= 40 && touch->py < 178)
        {
            const std::size_t row = (touch->py - 40) / 23;
            const std::size_t index = gameSelection / 6 * 6 + row;
            if (index < gameRows.size())
            {
                gameSelection = index;
                pane = Pane::Game;
            }
        }
        else if (touch->py >= 208)
        {
            if (touch->px < 80)
            {
                if (session && session->dirty()) discard();
                else ScreenStack::requestPop();
            }
            else if (touch->px < 160) sortActive();
            else if (touch->px < 240) commit();
            else menuOpen = true;
        }
    }
}
