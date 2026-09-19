/* GPL-3.0-or-later */
#ifndef YOKAIBANKSCREEN_HPP
#define YOKAIBANKSCREEN_HPP

#include "Screen.hpp"
#include "yokai/YokaiSaveContext.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class YokaiBankScreen : public Screen
{
public:
    explicit YokaiBankScreen(std::shared_ptr<yokai::SaveContext> context);
    void drawTop() const override;
    void drawBottom() const override;
    void update(touchPosition* touch) override;

private:
    enum class Pane { Game, Bank };

    void refresh();
    void toggleSelected();
    void selectAll();
    void transfer();
    void commit();
    void discard();
    [[nodiscard]] std::size_t visibleCount() const;
    [[nodiscard]] std::size_t bankPageCount() const;
    [[nodiscard]] std::size_t bankPageSize() const;
    [[nodiscard]] std::size_t selectedIndex() const;

    std::shared_ptr<yokai::SaveContext> context;
    yokai::Session* session = nullptr;
    std::vector<yokai::Record> gameRows;
    std::set<std::size_t> markedGameSlots;
    std::set<std::uint64_t> markedBankIds;
    yokai::Game activeGame = yokai::Game::YW1;
    Pane pane = Pane::Game;
    std::size_t gameSelection = 0;
    std::size_t bankSelection = 0;
    std::size_t bankPage = 0;
    std::filesystem::path activeSavePath;
    std::string status;
};

#endif
