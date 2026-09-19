/* GPL-3.0-or-later */
#ifndef YOKAI_SAVE_OVERVIEW_SCREEN_HPP
#define YOKAI_SAVE_OVERVIEW_SCREEN_HPP

#include "Screen.hpp"
#include "ClickButton.hpp"
#include "yokai/YokaiSaveContext.hpp"
#include <memory>
#include <string>

class YokaiSaveOverviewScreen : public Screen
{
public:
    explicit YokaiSaveOverviewScreen(yokai::SaveSource source);
    void drawTop() const override;
    void drawBottom() const override;
    void update(touchPosition* touch) override;

private:
    void loadProfile();
    void openYokai();

    yokai::SaveSource source;
    std::shared_ptr<yokai::SaveContext> context;
    std::unique_ptr<ClickButton> yokaiButton;
    std::string playerName = "Player";
    std::string playTime = "Not available";
    std::string status;
    bool loaded = false;
};

#endif
