/* GPL-3.0-or-later */
#include "YokaiTitleSelectScreen.hpp"
#include "gui.hpp"
#include "ScreenStack.hpp"
#include "Title.hpp"
#include "YokaiSaveOverviewScreen.hpp"
#include <algorithm>
#include <cstdio>
#include <utility>

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

void YokaiTitleSelectScreen::openSelected()
{
    if (groups.empty()) return;
    const auto& group = groups[selectedGroup];
    const std::size_t index = std::min(selectedSave, saveCount(group) - 1);
    yokai::SaveSource source;
    source.game = group.game;
    if (group.installed)
        source.installed = locations[group.locations[index]];
    else
        source.exported = group.exports[index];
    ScreenStack::push(std::make_unique<YokaiSaveOverviewScreen>(std::move(source)));
}

void YokaiTitleSelectScreen::drawTop() const
{
    const PKSM_Color navy(15, 22, 89, 255);
    Gui::drawSolidRect(0, 0, 400, 240, navy);
    Gui::sprite(manualMode ? ui_sheet_emulated_gameselector_bg_solid_idx
                           : ui_sheet_emulated_gameselector_bg_idx,
        4, 29);
    if (!manualMode) Gui::sprite(ui_sheet_gameselector_cart_idx, 35, 93);
    Gui::text(manualMode ? "Choose an exported save. Press Y for installed games."
                         : "Choose a save to open. Press Y for manual saves.",
        200, 8, FONT_SIZE_11, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP,
        TextWidthAction::SQUISH, 394);
    std::size_t installedOrdinal = 0;
    const std::size_t installedCount = static_cast<std::size_t>(std::count_if(groups.begin(),
        groups.end(), [this](const Group& group)
        { return manualMode || group.media != MEDIATYPE_GAME_CARD; }));
    bool cardDrawn = false;
    for (std::size_t index = 0; index < groups.size(); index++)
    {
        const auto& group = groups[index];
        int x = 0;
        int y = 0;
        if (!manualMode && group.media == MEDIATYPE_GAME_CARD && !cardDrawn)
        {
            x = 40; y = 98; cardDrawn = true;
        }
        else
        {
            x = 150 + static_cast<int>(installedOrdinal % 4) * 60;
            y = installedCount > 8 ? 38 + static_cast<int>(installedOrdinal / 4) * 60
                                   : (installedCount > 4
                                           ? 68 + static_cast<int>(installedOrdinal / 4) * 60
                                           : 98);
            installedOrdinal++;
        }
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
        if (index == selectedGroup) Gui::drawSelector(x - 1, y - 1);
    }

    if (!cardDrawn && !manualMode)
    {
        Gui::text("No card", 64, 111, FONT_SIZE_9, COLOR_WHITE,
            TextPosX::CENTER, TextPosY::TOP);
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
    Gui::backgroundBottom(true);
    Gui::drawSolidRect(0, 0, 320, 20, PKSM_Color(40, 53, 147, 255));
    Gui::text(manualMode ? "MANUAL SAVES" : "INSTALLED GAMES", 160, 3, FONT_SIZE_11,
        COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP);
    if (groups.empty())
    {
        Gui::text("Press Y to choose a manual save", 160, 104, FONT_SIZE_14, COLOR_WHITE,
            TextPosX::CENTER, TextPosY::TOP);
        Gui::text("START to exit", 160, 222, FONT_SIZE_9, COLOR_LIGHTBLUE,
            TextPosX::CENTER, TextPosY::TOP);
        return;
    }

    Gui::sprite(ui_sheet_gameselector_savebox_idx, 22, 94);
    Gui::saveboxDivider(146);
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

    if (group.title && group.title->icon().tex)
    {
        Gui::drawSolidRect(243, 21, 52, 52, navy);
        Gui::drawImageAt(group.title->icon(), 245, 23, nullptr, 1.0f, 1.0f);
    }
    const std::size_t count = saveCount(group);
    for (std::size_t index = 0; index < count && index < 5; index++)
    {
        const int y = 97 + static_cast<int>(index) * 17;
        if (index == selectedSave) Gui::drawSolidRect(24, y - 1, 174, 16, navy);
        Gui::text(saveLabel(group, index), 29, y, FONT_SIZE_11, COLOR_WHITE,
            TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SLICE, 162);
    }
    Gui::text("Load", 248, 120, FONT_SIZE_14, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::CENTER);
    Gui::text("D-Pad: move   A: continue   Y: source   START: exit", 160, 223,
        FONT_SIZE_11, COLOR_WHITE, TextPosX::CENTER, TextPosY::TOP,
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
