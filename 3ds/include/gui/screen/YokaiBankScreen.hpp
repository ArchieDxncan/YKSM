/* GPL-3.0-or-later */
#ifndef YOKAIBANKSCREEN_HPP
#define YOKAIBANKSCREEN_HPP

#include "Hid.hpp"
#include "Screen.hpp"
#include "yokai/Session.hpp"
#include "yokai/Crypto.hpp"
#include "yokai/YokaiTitleSource.hpp"
#include <filesystem>
#include <array>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class YokaiBankScreen : public Screen
{
public:
    YokaiBankScreen();
    void drawTop() const override;
    void drawBottom() const override;
    void update(touchPosition* touch) override;

private:
    enum class Pane { Game, Bank };

    void loadGame(yokai::Game game);
    void refresh();
    void toggleSelected();
    void selectAll();
    void transfer();
    void commit();
    void discard();
    void cycleGame();
    void cycleSave(int direction);
    [[nodiscard]] std::size_t visibleCount() const;
    [[nodiscard]] std::filesystem::path savePath(yokai::Game game) const;

    Hid<HidDirection::VERTICAL, HidDirection::HORIZONTAL> hid{9, 1};
    std::unique_ptr<yokai::Session> session;
    std::vector<yokai::Record> gameRows;
    std::set<std::size_t> markedGameSlots;
    std::set<std::uint64_t> markedBankIds;
    yokai::Game activeGame = yokai::Game::YW1;
    Pane pane = Pane::Game;
    std::filesystem::path activeSavePath;
    std::vector<yokai::title::Location> installedSaves;
    std::optional<yokai::title::Location> activeInstalledSave;
    std::vector<std::uint8_t> activeOriginalRaw;
    std::vector<std::uint8_t> activeHead;
    std::array<std::size_t, 5> sourceIndex{};
    yokai::crypto::SaveVariant activeVariant = yokai::crypto::SaveVariant::Yw1;
    std::string status;
};

#endif
