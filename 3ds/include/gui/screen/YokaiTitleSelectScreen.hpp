/* GPL-3.0-or-later */
#ifndef YOKAITITLESELECTSCREEN_HPP
#define YOKAITITLESELECTSCREEN_HPP

#include "Screen.hpp"
#include "yokai/YokaiTitleSource.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Title;

class YokaiTitleSelectScreen final : public Screen
{
public:
    YokaiTitleSelectScreen();
    void drawTop() const override;
    void drawBottom() const override;
    void update(touchPosition* touch) override;

private:
    struct Group
    {
        yokai::Game game = yokai::Game::YW1;
        FS_MediaType media = MEDIATYPE_SD;
        std::uint64_t titleId = 0;
        bool installed = false;
        std::vector<std::size_t> locations;
        std::vector<std::filesystem::path> exports;
        std::shared_ptr<Title> title;
    };

    void rebuild();
    void openSelected();
    [[nodiscard]] std::size_t saveCount(const Group& group) const;
    [[nodiscard]] std::string saveLabel(const Group& group, std::size_t index) const;
    [[nodiscard]] std::size_t sourceIndex(const Group& group, std::size_t index) const;
    [[nodiscard]] static std::string fullName(yokai::Game game);

    std::vector<yokai::title::Location> locations;
    std::vector<Group> groups;
    std::size_t selectedGroup = 0;
    std::size_t selectedSave = 0;
    bool manualMode = false;
};

#endif
