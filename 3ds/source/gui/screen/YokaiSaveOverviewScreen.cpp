/* GPL-3.0-or-later */
#include "YokaiSaveOverviewScreen.hpp"
#include "YokaiBankScreen.hpp"
#include "ScreenStack.hpp"
#include "gui.hpp"
#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
#include "yokai/YokaiTitleSource.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>

namespace
{
    constexpr const char* root = "/3ds/YKSM";

    std::vector<std::uint8_t> readFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) throw yokai::Error("Could not open " + path.string());
        return {(std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>()};
    }

    std::filesystem::path exportedSave(yokai::Game game, std::size_t index)
    {
        const auto directory = std::filesystem::path(root) / "saves" /
            std::string(yokai::gameName(game));
        std::vector<std::filesystem::path> candidates;
        for (const char* name : {"game1.yw", "game1.yw_g", "game2.yw", "game3.yw", "game.yw"})
        {
            const auto candidate = directory / name;
            if (std::filesystem::exists(candidate)) candidates.push_back(candidate);
        }
        if (candidates.empty()) return directory / "game1.yw";
        return candidates[index % candidates.size()];
    }

    std::string formatPlayTime(std::uint64_t seconds)
    {
        std::ostringstream text;
        text << seconds / 3600 << ':' << std::setw(2) << std::setfill('0') << (seconds / 60) % 60;
        return text.str();
    }
}

YokaiSaveOverviewScreen::YokaiSaveOverviewScreen(
    yokai::Game game, std::size_t sourceIndex, bool useInstalledSave)
    : Screen("A Open Yo-Kai\nB Back\nSTART Exit"), game(game), sourceIndex(sourceIndex),
      useInstalledSave(useInstalledSave)
{
    loadProfile();
}

void YokaiSaveOverviewScreen::loadProfile()
{
    try
    {
        const auto locations = yokai::title::discover();
        std::vector<yokai::title::Location> matches;
        std::copy_if(locations.begin(), locations.end(), std::back_inserter(matches),
            [this](const auto& location) { return location.game == game; });

        std::vector<std::uint8_t> raw;
        std::vector<std::uint8_t> head;
        if (useInstalledSave && !matches.empty())
        {
            const auto& location = matches[sourceIndex % matches.size()];
            raw = yokai::title::read(location, location.saveFile);
            if (game != yokai::Game::YW1) head = yokai::title::read(location, location.headFile);
        }
        else
        {
            const auto path = exportedSave(game, sourceIndex);
            raw = readFile(path);
            if (game != yokai::Game::YW1)
            {
                const bool moonRabbit = path.extension() == ".yw_g";
                auto headPath = path.parent_path() / (moonRabbit ? "head.yw_g" : "head.yw");
                if (!std::filesystem::exists(headPath))
                    headPath = path.parent_path() / (moonRabbit ? "head.yw" : "head.yw_g");
                head = readFile(headPath);
            }
        }

        auto decrypted = yokai::crypto::decryptSave(game, raw, head);
        const yokai::SaveImage save(game, std::move(decrypted.bytes));
        const std::string decodedName = save.playerName();
        if (!decodedName.empty()) playerName = decodedName;
        if (const auto seconds = save.playTimeSeconds()) playTime = formatPlayTime(*seconds);
        status = "Save loaded";
        loaded = true;
    }
    catch (const std::exception& error)
    {
        status = error.what();
        loaded = false;
    }
}

void YokaiSaveOverviewScreen::drawTop() const
{
    const PKSM_Color navy(15, 22, 89, 255);
    Gui::drawSolidRect(0, 0, 400, 240, navy);
    Gui::text(playerName, 14, 14, FONT_SIZE_18, COLOR_WHITE,
        TextPosX::LEFT, TextPosY::TOP, TextWidthAction::SQUISH, 360);
    Gui::text("Play time: " + playTime, 14, 43, FONT_SIZE_12, COLOR_LIGHTBLUE,
        TextPosX::LEFT, TextPosY::TOP);
}

void YokaiSaveOverviewScreen::drawBottom() const
{
    const PKSM_Color navy(15, 22, 89, 255);
    const PKSM_Color blue(31, 43, 132, 255);
    Gui::drawSolidRect(0, 0, 320, 240, navy);
    Gui::drawSolidRect(36, 68, 248, 82, loaded ? blue : COLOR_DARKGREY);
    Gui::drawSolidRect(40, 72, 240, 74, PKSM_Color(24, 35, 111, 255));
    Gui::text("Yo-Kai", 160, 96, FONT_SIZE_18, COLOR_WHITE,
        TextPosX::CENTER, TextPosY::TOP);
    Gui::text(loaded ? "A  Open" : status, 160, 168, FONT_SIZE_11,
        loaded ? COLOR_LIGHTBLUE : COLOR_YELLOW, TextPosX::CENTER, TextPosY::TOP,
        TextWidthAction::SQUISH, 294);
    Gui::text("B Back", 12, 220, FONT_SIZE_9, COLOR_WHITE, TextPosX::LEFT, TextPosY::TOP);
}

void YokaiSaveOverviewScreen::openYokai()
{
    if (loaded)
        ScreenStack::push(std::make_unique<YokaiBankScreen>(game, sourceIndex, useInstalledSave));
}

void YokaiSaveOverviewScreen::update(touchPosition* touch)
{
    const u32 down = hidKeysDown();
    if (down & KEY_START) Gui::exitMainLoop();
    if (down & KEY_B) ScreenStack::requestPop();
    if (down & KEY_A) openYokai();
    if (touch && (down & KEY_TOUCH) && touch->px >= 36 && touch->px <= 284 &&
        touch->py >= 68 && touch->py <= 150)
        openYokai();
}
