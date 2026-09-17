/* GPL-3.0-or-later */
#include "yokai/Transfer.hpp"
#include "yokai/SaveImage.hpp"
#include "yokai/Species.hpp"
#include <algorithm>
#include <array>
#include <numeric>

namespace yokai
{
    namespace
    {
        std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t offset)
        {
            if (offset + 4 > bytes.size()) throw Error("Record is truncated");
            return bytes[offset] | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
                   (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
                   (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
        }

        void write32(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t value)
        {
            if (offset + 4 > bytes.size()) throw Error("Record is truncated");
            for (int index = 0; index < 4; index++)
                bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8));
        }

        std::vector<int> scale(std::span<const int> points, int target)
        {
            const int total = std::accumulate(points.begin(), points.end(), 0);
            if (total <= 0) return {};
            std::vector<int> result(points.size());
            std::vector<int> order(points.size());
            std::iota(order.begin(), order.end(), 0);
            for (std::size_t index = 0; index < points.size(); index++)
                result[index] = points[index] * target / total;
            int remaining = target - std::accumulate(result.begin(), result.end(), 0);
            std::stable_sort(order.begin(), order.end(), [&](int left, int right)
            {
                const int leftRemainder = points[left] * target % total;
                const int rightRemainder = points[right] * target % total;
                if (leftRemainder != rightRemainder) return leftRemainder > rightRemainder;
                if (points[left] != points[right]) return points[left] > points[right];
                return left < right;
            });
            for (int index = 0; index < remaining; index++) result[order[index]]++;
            return result;
        }

        std::array<std::uint8_t, 5> ivs(Game game, std::span<const std::uint8_t> raw)
        {
            std::array<std::uint8_t, 5> output{};
            std::size_t offset = 0;
            if (game == Game::YW1)
            {
                offset = 0x45;
                for (int i = 0; i < 5; i++) output[i] = raw[offset + i] & 0x0F;
                return output;
            }
            if (game == Game::YW3) offset = 0x34;
            else offset = 0x40;
            const std::size_t count = game == Game::Blasters ? 4 : 5;
            std::copy_n(raw.begin() + offset, count, output.begin());
            return output;
        }

        std::array<std::uint8_t, 5> convertedIvs(
            Game source, Game target, std::span<const std::uint8_t> raw)
        {
            auto sourceIv = ivs(source, raw);
            if (source == Game::YW1)
            {
                std::array<int, 5> points{};
                std::copy(sourceIv.begin(), sourceIv.end(), points.begin());
                const auto modern = scale(points, 40);
                sourceIv = modern.empty()
                    ? std::array<std::uint8_t, 5>{16, 8, 8, 8, 8}
                    : std::array<std::uint8_t, 5>{static_cast<std::uint8_t>(modern[0] * 2),
                          static_cast<std::uint8_t>(modern[1]), static_cast<std::uint8_t>(modern[2]),
                          static_cast<std::uint8_t>(modern[3]), static_cast<std::uint8_t>(modern[4])};
            }
            if (target == Game::YW1)
            {
                std::array<int, 5> points = {sourceIv[0] / 2, sourceIv[1], sourceIv[2],
                    sourceIv[3], sourceIv[4]};
                const auto old = sourceIv[0] % 2 == 0 ? scale(points, 10) : std::vector<int>{};
                std::array<std::uint8_t, 5> output{2, 2, 2, 2, 2};
                if (!old.empty())
                    for (int i = 0; i < 5; i++) output[i] = static_cast<std::uint8_t>(old[i]);
                return output;
            }
            if (target == Game::Blasters)
            {
                std::array<int, 4> points = {sourceIv[0] / 2, sourceIv[1], sourceIv[2], sourceIv[3]};
                const auto reduced = scale(points, 40);
                return reduced.empty()
                    ? std::array<std::uint8_t, 5>{20, 10, 10, 10, 0}
                    : std::array<std::uint8_t, 5>{static_cast<std::uint8_t>(reduced[0] * 2),
                          static_cast<std::uint8_t>(reduced[1]), static_cast<std::uint8_t>(reduced[2]),
                          static_cast<std::uint8_t>(reduced[3]), 0};
            }
            const int total = sourceIv[0] / 2 + sourceIv[1] + sourceIv[2] + sourceIv[3] + sourceIv[4];
            return sourceIv[0] % 2 == 0 && total == 40
                ? sourceIv
                : std::array<std::uint8_t, 5>{16, 8, 8, 8, 8};
        }

        std::pair<std::uint8_t, std::optional<std::uint8_t>> temperament(
            Game game, std::span<const std::uint8_t> raw)
        {
            if (game == Game::YW1) return {static_cast<std::uint8_t>(raw[0x55] & 0x0F), raw[0x57] ? 0 : 1};
            if (game == Game::Blasters || game == Game::Busters2) return {1, std::nullopt};
            const std::uint8_t packed = raw[game == Game::YW2 ? 0x54 : 0x4C];
            return {static_cast<std::uint8_t>(packed & 0x0F), static_cast<std::uint8_t>(packed >> 4)};
        }

        std::array<std::uint8_t, 4> health(Game game, std::span<const std::uint8_t> raw)
        {
            std::array<std::uint8_t, 4> result{};
            if (game == Game::Blasters || game == Game::Busters2) return result;
            const std::size_t offset = game == Game::YW3 ? 0x50 : 0x58;
            std::copy_n(raw.begin() + offset, 4, result.begin());
            return result;
        }

        void copyOwner(std::span<std::uint8_t> output, Game target,
            std::span<const std::uint8_t> example)
        {
            if (example.size() != layout(target).recordSize) return;
            const std::size_t offset = target == Game::YW3 ? 0x30 : 0x3C;
            if (offset + 4 <= output.size()) std::copy_n(example.begin() + offset, 4, output.begin() + offset);
        }
    }

    bool compatible(Game target, const BankEntry& entry)
    {
        return target == entry.sourceGame || transferSpeciesId(entry.sourceGame, target, entry.species).has_value();
    }

    void validateRecord(Game game, std::span<const std::uint8_t> record)
    {
        if (record.size() != layout(game).recordSize) throw Error("Generated record has the wrong size");
        const std::uint32_t id = read32(record, 4);
        if (speciesName(game, id).empty()) throw Error("Generated record has an invalid species");
        const std::uint8_t level = record[layout(game).levelOffset];
        if (level < 1 || level > 99) throw Error("Generated record has an invalid level");
        const auto values = ivs(game, record);
        if (game == Game::YW1)
        {
            int total = 0;
            for (std::uint8_t value : values) total += value & 0x0F;
            if (total != 10) throw Error("Generated YW1 record has invalid IVs");
        }
        else
        {
            const std::size_t count = game == Game::Blasters ? 4 : 5;
            int total = values[0] / 2;
            for (std::size_t i = 1; i < count; i++) total += values[i];
            if (values[0] % 2 || total != 40) throw Error("Generated record has invalid IVs");
        }
    }

    std::vector<std::uint8_t> convertRecord(const BankEntry& source, Game target,
        std::span<const std::uint8_t> destinationExample)
    {
        if (source.sourceGame == target) return source.raw;
        const auto targetId = transferSpeciesId(source.sourceGame, target, source.species);
        if (!targetId) throw Error(source.species + " does not exist in " + std::string(gameName(target)));
        if (source.raw.size() != layout(source.sourceGame).recordSize) throw Error("Source record has the wrong size");

        std::vector<std::uint8_t> output(layout(target).recordSize);
        write32(output, 4, *targetId);
        const std::size_t nicknameLimit = target == Game::YW1 ? 35 : target == Game::Blasters ? 27 : 23;
        std::copy_n(source.nickname.begin(), std::min(nicknameLimit, source.nickname.size()), output.begin() + 8);
        const auto converted = convertedIvs(source.sourceGame, target, source.raw);
        auto [attitude, loaf] = temperament(source.sourceGame, source.raw);
        if (attitude > 12) attitude = 1;
        const auto hp = health(source.sourceGame, source.raw);

        switch (target)
        {
            case Game::YW1:
                write32(output, 0x38, source.xp);
                std::copy(converted.begin(), converted.end(), output.begin() + 0x45);
                output[0x54] = std::clamp<std::uint8_t>(source.level, 1, 99);
                output[0x55] = attitude;
                std::copy(hp.begin(), hp.end(), output.begin() + 0x58);
                break;
            case Game::YW2:
                write32(output, 0x34, source.xp);
                std::copy(converted.begin(), converted.end(), output.begin() + 0x40);
                output[0x4F] = std::clamp<std::uint8_t>(source.level, 1, 99);
                output[0x54] = static_cast<std::uint8_t>(((loaf.value_or(3) & 0x0F) << 4) | attitude);
                std::copy(hp.begin(), hp.end(), output.begin() + 0x58);
                copyOwner(output, target, destinationExample);
                break;
            case Game::YW3:
                write32(output, 0x28, source.xp);
                std::copy(converted.begin(), converted.end(), output.begin() + 0x34);
                output[0x46] = output[0x47] = output[0x48] = 1;
                output[0x49] = std::clamp<std::uint8_t>(source.level, 1, 99);
                output[0x4C] = static_cast<std::uint8_t>(((loaf.value_or(3) & 0x0F) << 4) | attitude);
                std::copy(hp.begin(), hp.end(), output.begin() + 0x50);
                copyOwner(output, target, destinationExample);
                break;
            case Game::Blasters:
                write32(output, 0x38, source.xp);
                std::copy_n(converted.begin(), 4, output.begin() + 0x40);
                output[0x49] = std::clamp<std::uint8_t>(source.level, 1, 99);
                copyOwner(output, target, destinationExample);
                for (std::size_t index = 0; index < defaultMoves(target, *targetId).size(); index++)
                    write32(output, 0x2C + index * 4, defaultMoves(target, *targetId)[index]);
                break;
            case Game::Busters2:
                std::copy(converted.begin(), converted.end(), output.begin() + 0x40);
                output[0x48] = std::clamp<std::uint8_t>(source.level, 1, 99);
                copyOwner(output, target, destinationExample);
                for (std::size_t index = 0; index < defaultMoves(target, *targetId).size(); index++)
                    write32(output, 0x28 + index * 4, defaultMoves(target, *targetId)[index]);
                break;
        }
        validateRecord(target, output);
        return output;
    }
}
