/* GPL-3.0-or-later */
#include "YokaiTitleSelectScreen.hpp"
#include "gui.hpp"
#include "ScreenStack.hpp"
#include "Title.hpp"
#include "YokaiSaveOverviewScreen.hpp"
#include <algorithm>
#include <cstdio>

namespace
{
    constexpr const char* root = "/3ds/YKSM";

    PKSM_Color tileColor(yokai::Game game)
    {
        switch (game)
        {
            case yokai::Game::YW1: return PKSM_Color(43, 117, 196, 255);
            case yokai::Game::YW2: return PKSM_Color(190, 55, 72, 255);
            case yokai::Game::YW3: return PKSM_Color(221, 139, 35, 255);
            case yokai::Game::Blasters: return PKSM_Color(58, 151, 105, 255);
            case yokai::Game::Busters2: return PKSM_Color(119, 71, 169, 255);
        }
        return COLOR_HIGHBLUE;
    }
}

YokaiTitleSelectScreen::YokaiTitleSelectScreen()
    : Screen("D-Pad Choose title and save\nA Load\nY Manual saves\nSTART Exit")
{
    locations = yokai::title::discover();
    rebuild();
}

std::string YokaiTitleSelectScreen::fullName(yokai::Game game)
{
    switch (game)
    {
        case yokai::Game::YW1: return "Yo-kai Watch";
        case yokai::Game::YW2: return "Yo-kai Watch 2";
        case yokai::Game::YW3: return "Yo-kai Watch 3";
        case yokai::Game::Blasters: return "Yo-kai Watch Blasters";
        case yokai::Game::Busters2: return "Yo-kai Watch Busters 2";
    }
    return "Yo-kai Watch";
}

void YokaiTitleSelectScreen::rebuild()
{
    groups.clear();
    selectedGroup = 0;
    selectedSave = 0;
    if (!manualMode)
    {
        for (std::size_t index = 0; index < locations.size(); index++)
        {
            const auto& location = locations[index];
            auto found = std::find_if(groups.begin(), groups.end(), [&](const Group& group)
                { return group.game == location.game && group.media == location.media &&
                         group.titleId == location.titleId; });
            if (found == groups.end())
            {
                Group group{location.game, location.media, location.titleId, true, {index}, {}, {}};
                group.title = std::make_shared<Title>();
                if (!group.title->load(location.titleId, location.media, CARD_CTR))
                    group.title.reset();
                groups.push_back(std::move(group));
            }
            else
            {
                found->locations.push_back(index);
            }
        }
    }
    else
    {
        for (int value = 0; value < 5; value++)
        {
            const auto game = static_cast<yokai::Game>(value);
            Group group{game, MEDIATYPE_SD, 0, false, {}, {}, {}};
            const auto directory = std::filesystem::path(root) / "saves" /
                std::string(yokai::gameName(game));
            for (const char* name : {"game1.yw", "game1.yw_g", "game2.yw", "game3.yw", "game.yw"})
            {
                const auto candidate = directory / name;
                if (std::filesystem::exists(candidate)) group.exports.push_back(candidate);
            }
            if (group.exports.empty()) group.exports.push_back(directory / "game1.yw");
            groups.push_back(std::move(group));
        }
    }
}

std::size_t YokaiTitleSelectScreen::saveCount(const Group& group) const
{
    return group.installed ? group.locations.size() : group.exports.size();
}

std::string YokaiTitleSelectScreen::saveLabel(const Group& group, std::size_t index) const
{
    if (group.installed) return locations[group.locations[index]].saveFile.substr(1);
    return group.exports[index].filename().string();
}

std::size_t YokaiTitleSelectScreen::sourceIndex(const Group& group, std::size_t index) const
{
    if (!group.installed) return index;
    const auto target = group.locations[index];
    std::size_t result = 0;
    for (std::size_t location = 0; location < target; location++)
        if (locations[location].game == group.game) result++;
    return result;
}

void YokaiTitleSelectScreen::openSelected()
{
    if (groups.empty()) return;
    const auto& group = groups[selectedGroup];
    ScreenStack::push(std::make_unique<YokaiSaveOverviewScreen>(
        group.game, sourceIndex(group, std::min(selectedSave, saveCount(group) - 1)),
        group.installed));
}

void YokaiTitleSelectScreen::drawTop() const
{
    const PKSM_Color navy(15, 22, 89, 255);
    const PKSM_Color panel(31, 43, 132, 255);
    Gui::drawSolidRect(0, 0, 400, 240, navy);
    Gui::text(manualMode ? "Choose an exported save. Press Y for installed games."
                         : "Choose a save to open. Press Y for manual saves.",
        200, 8, FONT_SIZE_11, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP,
        TextWidthAction::SQUISH, 394);
    Gui::drawSolidRect(5, 29, 120, 185, panel);
    Gui::drawSolidRect(129, 29, 266, 185, panel);

    std::size_t installedOrdinal = 0;
    bool cardDrawn = false;
    for (std::size_t index = 0; index < groups.size(); index++)
    {
        const auto& group = groups[index];
        int x = 0;
        int y = 0;
        if (!manualMode && group.media == MEDIATYPE_GAME_CARD && !cardDrawn)
        {
            x = 37; y = 83; cardDrawn = true;
        }
        else
        {
            x = 145 + static_cast<int>(installedOrdinal % 4) * 60;
            y = 62 + static_cast<int>(installedOrdinal / 4) * 72;
            installedOrdinal++;
        }
        if (index == selectedGroup) Gui::drawSolidRect(x - 4, y - 4, 56, 56, COLOR_YELLOW);
        if (group.title && group.title->icon().tex)
        {
            Gui::drawImageAt(group.title->icon(), x, y, nullptr, 1.0f, 1.0f);
        }
        else
        {
            Gui::drawSolidRect(x, y, 48, 48, tileColor(group.game));
            Gui::drawSolidRect(x + 4, y + 4, 40, 40, navy);
            Gui::text(std::string(yokai::gameName(group.game)), x + 24, y + 17, FONT_SIZE_9,
                COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP, TextWidthAction::SQUISH, 38);
        }
    }

    if (!cardDrawn && !manualMode)
    {
        Gui::drawSolidRect(37, 83, 48, 48, PKSM_Color(35, 39, 72, 255));
        Gui::text(
            "CARD", 61, 100, FONT_SIZE_9, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    }
    if (groups.empty())
        Gui::text("No supported installed saves found", 262, 105, FONT_SIZE_14, COLOR_WHITE,
            TextPosX::CENTER, TextPosY::TOP);
    Gui::text("Game Card", 65, 218, FONT_SIZE_12, navy, TextPosX::CENTER, TextPosY::TOP);
    Gui::text(manualMode ? "Manual / Exported Saves" : "Installed Games", 262, 218,
        FONT_SIZE_12, navy, TextPosX::CENTER, TextPosY::TOP);
}

void YokaiTitleSelectScreen::drawBottom() const
{
    const PKSM_Color navy(15, 22, 89, 255);
    const PKSM_Color blue(31, 43, 132, 255);
    Gui::drawSolidRect(0, 0, 320, 240, navy);
    if (groups.empty())
    {
        Gui::text("Press Y to choose a manual save", 160, 104, FONT_SIZE_14, COLOR_WHITE,
            TextPosX::CENTER, TextPosY::TOP);
        Gui::text("START to exit", 160, 222, FONT_SIZE_9, COLOR_LIGHTBLUE,
            TextPosX::CENTER, TextPosY::TOP);
        return;
    }

    const auto& group = groups[selectedGroup];
    const std::string titleName = group.title && !group.title->name().empty()
                                      ? group.title->name()
                                      : fullName(group.game);
    Gui::text(titleName, 27, 20, FONT_SIZE_14, COLOR_WHITE,
        TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SQUISH, 220);
    char id[24];
    std::snprintf(id, sizeof(id), "ID: %08lX", static_cast<unsigned long>(group.titleId & 0xFFFFFFFF));
    Gui::text(group.installed ? id : "Path: /3ds/YKSM/saves", 27, 42, FONT_SIZE_9,
        COLOR_LIGHTBLUE, TextPosX::LEFT, TextPosY::TOP);
    const char* media = !group.installed
                          ? "Exported file"
                          : (group.media == MEDIATYPE_GAME_CARD ? "Media Type: Game Card"
                                                               : "Media Type: SD");
    Gui::text(media, 27, 56, FONT_SIZE_9, COLOR_LIGHTBLUE, TextPosX::LEFT, TextPosY::TOP);

    Gui::drawSolidRect(22, 88, 176, 108, blue);
    Gui::drawSolidRect(200, 88, 96, 108, PKSM_Color(19, 28, 99, 255));
    Gui::text(group.installed ? "Game Save File" : "Exported Save File", 29, 91,
        FONT_SIZE_9, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
    const std::size_t count = saveCount(group);
    for (std::size_t index = 0; index < count && index < 5; index++)
    {
        const int y = 111 + static_cast<int>(index) * 16;
        if (index == selectedSave) Gui::drawSolidRect(25, y - 1, 170, 15, navy);
        Gui::text(saveLabel(group, index), 29, y, FONT_SIZE_9, COLOR_WHITE,
            TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SLICE, 162);
    }
    Gui::text("Load  A", 248, 134, FONT_SIZE_14, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
    Gui::text("Move your D-Pad. Press A to continue. START to exit.", 160, 218,
        FONT_SIZE_9, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP,
        TextWidthAction::SQUISH, 316);
}

void YokaiTitleSelectScreen::update(touchPosition* touch)
{
    const u32 down = hidKeysDown();
    if (down & KEY_START) Gui::exitMainLoop();
    if (down & KEY_Y)
    {
        manualMode = !manualMode;
        rebuild();
        return;
    }
    if (groups.empty()) return;
    if (down & KEY_LEFT)
    {
        selectedGroup = selectedGroup == 0 ? groups.size() - 1 : selectedGroup - 1;
        selectedSave = 0;
    }
    if (down & KEY_RIGHT)
    {
        selectedGroup = (selectedGroup + 1) % groups.size();
        selectedSave = 0;
    }
    const std::size_t count = saveCount(groups[selectedGroup]);
    if (down & KEY_UP) selectedSave = selectedSave == 0 ? count - 1 : selectedSave - 1;
    if (down & KEY_DOWN) selectedSave = (selectedSave + 1) % count;
    if (down & KEY_A) openSelected();
    if (touch && (down & KEY_TOUCH) && touch->px >= 200 && touch->px <= 296 &&
        touch->py >= 88 && touch->py <= 196)
        openSelected();
}
