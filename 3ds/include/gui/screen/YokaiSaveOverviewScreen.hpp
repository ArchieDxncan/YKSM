/* GPL-3.0-or-later */
#ifndef YOKAI_SAVE_OVERVIEW_SCREEN_HPP
#define YOKAI_SAVE_OVERVIEW_SCREEN_HPP

#include "Screen.hpp"
#include "yokai/Yokai.hpp"
#include <cstddef>
#include <string>

class YokaiSaveOverviewScreen : public Screen
{
public:
    YokaiSaveOverviewScreen(yokai::Game game, std::size_t sourceIndex, bool useInstalledSave);
    void drawTop() const override;
    void drawBottom() const override;
    void update(touchPosition* touch) override;

private:
    void loadProfile();
    void openYokai();

    yokai::Game game;
    std::size_t sourceIndex;
    bool useInstalledSave;
    std::string playerName = "Player";
    std::string playTime = "--:--";
    std::string status;
    bool loaded = false;
};

#endif
