/* GPL-3.0-or-later */
#include "YokaiSaveOverviewScreen.hpp"
#include "YokaiBankScreen.hpp"
#include "ScreenStack.hpp"
#include "gui.hpp"
#include <iomanip>
#include <sstream>
#include <utility>

namespace
{
    std::string formatPlayTime(std::uint64_t seconds)
    {
        std::ostringstream text;
        text << seconds / 3600 << ':' << std::setw(2) << std::setfill('0')
             << (seconds / 60) % 60;
        return text.str();
    }
}

YokaiSaveOverviewScreen::YokaiSaveOverviewScreen(yokai::SaveSource source)
    : Screen("A Open Yo-Kai\nB Back\nSTART Exit"), source(std::move(source))
{
    loadProfile();
    yokaiButton = std::make_unique<ClickButton>(90, 78, 140, 53,
        [this]()
        {
            openYokai();
            return true;
        },
        ui_sheet_mainmenu_button_idx, "Yo-Kai", FONT_SIZE_15, COLOR_WHITE);
}

void YokaiSaveOverviewScreen::loadProfile()
{
    try
    {
        context = yokai::SaveContext::load(source);
        const std::string decodedName = context->session.save().playerName();
        playerName = decodedName.empty() ? "Not available" : decodedName;
        if (const auto seconds = context->session.save().playTimeSeconds())
            playTime = formatPlayTime(*seconds);
        status = "Save loaded";
        loaded = true;
    }
    catch (const std::exception& error)
    {
        context.reset();
        status = error.what();
        loaded = false;
    }
}

void YokaiSaveOverviewScreen::drawTop() const
{
    Gui::sprite(ui_sheet_emulated_bg_top_blue, 0, 0);
    Gui::sprite(ui_sheet_bg_style_top_idx, 0, 0);
    Gui::sprite(ui_sheet_bar_arc_top_blue_idx, 0, 0);
    Gui::backgroundAnimatedTop();
    Gui::sprite(ui_sheet_textbox_hidden_power_idx, 137, 3);
    for (int y = 34; y < 156; y += 40) Gui::sprite(ui_sheet_stripe_info_row_idx, 0, y);
    for (int y = 40; y < 140; y += 20) Gui::sprite(ui_sheet_point_big_idx, 1, y);

    Gui::text("Player:", 10, 36, FONT_SIZE_12, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
    Gui::text(playerName, 64, 36, FONT_SIZE_12, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP,
        TextWidthAction::SQUISH, 326);
    Gui::text("Play time:", 10, 56, FONT_SIZE_12, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
    Gui::text(playTime, 82, 56, FONT_SIZE_12, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
    Gui::text("Game:", 10, 76, FONT_SIZE_12, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
    Gui::text(yokai::gameName(source.game).data(), 56, 76, FONT_SIZE_12, COLOR_BLACK,
        TextPosX::LEFT, TextPosY::TOP);
    Gui::text("Source:", 10, 96, FONT_SIZE_12, COLOR_BLACK, TextPosX::LEFT, TextPosY::TOP);
    Gui::text(context ? context->sourceLabel : "Unavailable", 65, 96, FONT_SIZE_12, COLOR_BLACK,
        TextPosX::LEFT, TextPosY::TOP,
        TextWidthAction::SQUISH, 325);
    Gui::text(loaded ? "Ready" : status, 10, 116, FONT_SIZE_12,
        loaded ? COLOR_BLACK : COLOR_UNSELECTRED, TextPosX::LEFT, TextPosY::TOP,
        TextWidthAction::SQUISH, 380);
    Gui::text("YKSM", 282, 16, FONT_SIZE_14, COLOR_WHITE,
        TextPosX::RIGHT, TextPosY::CENTER);
}

void YokaiSaveOverviewScreen::drawBottom() const
{
    Gui::sprite(ui_sheet_emulated_bg_bottom_blue, 0, 0);
    Gui::sprite(ui_sheet_bg_style_bottom_idx, 0, 0);
    Gui::sprite(ui_sheet_bar_arc_bottom_blue_idx, 0, 206);
    Gui::backgroundAnimatedBottom();
    if (loaded)
        yokaiButton->draw();
    else
        Gui::text(status, 160, 96, FONT_SIZE_11, COLOR_YELLOW, TextPosX::CENTER,
            TextPosY::TOP, TextWidthAction::WRAP, 290);
    Gui::text("A: open   B: back   START: exit", 160, 222, FONT_SIZE_11, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
}

void YokaiSaveOverviewScreen::openYokai()
{
    if (loaded && context)
        ScreenStack::push(std::make_unique<YokaiBankScreen>(context));
}

void YokaiSaveOverviewScreen::update(touchPosition* touch)
{
    const u32 down = hidKeysDown();
    if (down & KEY_START) Gui::exitMainLoop();
    if (down & KEY_B) ScreenStack::requestPop();
    if (down & KEY_A) openYokai();
    if (loaded) yokaiButton->update(touch);
}
