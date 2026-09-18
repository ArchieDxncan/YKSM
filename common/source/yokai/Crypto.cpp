/*
 * Level-5 Yo-kai cipher derived from togenyan/yw_save (MIT, 2016).
 */
#include "yokai/Crypto.hpp"
#include "yokai/SaveImage.hpp"
extern "C"
{
#include "aes.h"
}
#include <algorithm>
#include <array>
#include <map>
#include <string_view>

namespace yokai::crypto
{
    namespace
    {
        std::uint32_t read32(std::span<const std::uint8_t> data, std::size_t offset)
        {
            if (offset + 4 > data.size()) throw Error("Cipher payload is truncated");
            return data[offset] | (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
                   (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
                   (static_cast<std::uint32_t>(data[offset + 3]) << 24);
        }

        void write32(std::span<std::uint8_t> data, std::size_t offset, std::uint32_t value)
        {
            for (int index = 0; index < 4; index++)
                data[offset + index] = static_cast<std::uint8_t>(value >> (index * 8));
        }

        const std::array<std::uint16_t, 256>& oddPrimes()
        {
            static const std::array<std::uint16_t, 256> primes = []
            {
                std::array<std::uint16_t, 256> output{};
                std::size_t count = 0;
                for (int candidate = 3; count < output.size(); candidate += 2)
                {
                    bool prime = true;
                    for (int divisor = 3; divisor * divisor <= candidate; divisor += 2)
                        if (candidate % divisor == 0) { prime = false; break; }
                    if (prime) output[count++] = static_cast<std::uint16_t>(candidate);
                }
                return output;
            }();
            return primes;
        }

        class Xorshift
        {
        public:
            explicit Xorshift(std::uint32_t seed)
                : states{0x6C078966, 0xDD5254A5, 0xB9523B81, 0x03DF95B3}
            {
                if (seed)
                {
                    for (std::uint32_t index = 1; index <= 3; index++)
                    {
                        seed ^= seed >> 30;
                        seed = seed * (0x6C078966 - 1) + index;
                        states[index - 1] = seed;
                    }
                }
            }

            std::uint32_t next(std::uint32_t modulus)
            {
                std::uint32_t x = states[0];
                const std::uint32_t y = states[3];
                states[0] = states[1]; states[1] = states[2]; states[2] = states[3];
                x ^= x << 11; x ^= x >> 8;
                states[3] = x ^ y ^ (y >> 19);
                return modulus ? states[3] % modulus : states[3];
            }

        private:
            std::array<std::uint32_t, 4> states;
        };

        std::vector<std::uint8_t> apply(std::span<const std::uint8_t> input, std::uint32_t seed)
        {
            std::array<std::uint8_t, 256> table{};
            for (int index = 0; index < 256; index++) table[index] = static_cast<std::uint8_t>(index);
            Xorshift random(seed);
            for (int index = 0; index < 0x1000; index++)
            {
                const std::uint32_t value = random.next(0x10000);
                const std::uint8_t r1 = value & 0xFF;
                const std::uint8_t r2 = (value >> 8) & 0xFF;
                if (r1 != r2)
                {
                    const std::uint8_t a = table[r1];
                    const std::uint8_t b = table[r2];
                    std::swap(table[a], table[b]);
                }
            }
            std::vector<std::uint8_t> output(input.size());
            std::uint16_t keyA = 0;
            for (std::size_t index = 0; index < input.size(); index++)
            {
                if (index % 0x100 == 0) keyA = oddPrimes()[table[(index & 0xFF00) >> 8]];
                const std::uint8_t keyB = table[keyA * (index + 1) & 0xFF];
                output[index] = input[index] ^ keyB;
            }
            return output;
        }

        std::array<std::uint8_t, 16> aesBlock(std::span<const std::uint8_t, 16> input,
            std::span<const std::uint8_t, 16> key)
        {
            std::array<std::uint8_t, 16> in{};
            std::array<std::uint8_t, 16> out{};
            std::copy(input.begin(), input.end(), in.begin());
            AES128_ECB_encrypt(in.data(), key.data(), out.data());
            return out;
        }

        std::array<std::uint8_t, 16> counterBlock(
            std::span<const std::uint8_t, 12> nonce, std::uint32_t counter)
        {
            std::array<std::uint8_t, 16> block{};
            block[0] = 2; // L - 1, with a twelve-byte nonce and three-byte length/counter.
            std::copy(nonce.begin(), nonce.end(), block.begin() + 1);
            block[13] = static_cast<std::uint8_t>(counter >> 16);
            block[14] = static_cast<std::uint8_t>(counter >> 8);
            block[15] = static_cast<std::uint8_t>(counter);
            return block;
        }

        std::array<std::uint8_t, 16> ccmMac(std::span<const std::uint8_t> plain,
            std::span<const std::uint8_t, 16> key, std::span<const std::uint8_t, 12> nonce)
        {
            if (plain.size() > 0xFFFFFF) throw Error("CCM payload is too large");
            std::array<std::uint8_t, 16> block{};
            block[0] = 0x3A; // 16-byte tag, no associated data, L=3.
            std::copy(nonce.begin(), nonce.end(), block.begin() + 1);
            block[13] = static_cast<std::uint8_t>(plain.size() >> 16);
            block[14] = static_cast<std::uint8_t>(plain.size() >> 8);
            block[15] = static_cast<std::uint8_t>(plain.size());
            auto state = aesBlock(block, key);
            for (std::size_t offset = 0; offset < plain.size(); offset += 16)
            {
                block.fill(0);
                const std::size_t amount = std::min<std::size_t>(16, plain.size() - offset);
                std::copy_n(plain.begin() + offset, amount, block.begin());
                for (std::size_t index = 0; index < 16; index++) block[index] ^= state[index];
                state = aesBlock(block, key);
            }
            return state;
        }

        std::vector<std::uint8_t> ccmCrypt(std::span<const std::uint8_t> input,
            std::span<const std::uint8_t, 16> key, std::span<const std::uint8_t, 12> nonce)
        {
            std::vector<std::uint8_t> output(input.size());
            for (std::size_t offset = 0, counter = 1; offset < input.size(); offset += 16, counter++)
            {
                const auto stream = aesBlock(counterBlock(nonce, counter), key);
                const std::size_t amount = std::min<std::size_t>(16, input.size() - offset);
                for (std::size_t index = 0; index < amount; index++)
                    output[offset + index] = input[offset + index] ^ stream[index];
            }
            return output;
        }

        struct Section
        {
            std::uint8_t id = 0;
            std::array<std::uint8_t, 8> header{};
            std::vector<std::uint8_t> payload;
            std::array<std::uint8_t, 4> footer{};
            std::vector<Section> children;
            bool container = false;

            [[nodiscard]] std::vector<std::uint8_t> serialize() const
            {
                std::vector<std::uint8_t> output(header.begin(), header.end());
                if (container)
                    for (const auto& child : children)
                    {
                        const auto bytes = child.serialize();
                        output.insert(output.end(), bytes.begin(), bytes.end());
                    }
                else
                    output.insert(output.end(), payload.begin(), payload.end());
                output.insert(output.end(), footer.begin(), footer.end());
                return output;
            }
        };

        Section parseSection(std::span<const std::uint8_t> data, std::size_t offset,
            std::size_t& consumed)
        {
            if (offset + 12 > data.size() || (read32(data, offset) & 0xFFFF) != 0xFFFE)
                throw Error("Invalid Yo-kai section header");
            const std::uint32_t descriptor = read32(data, offset + 4);
            const std::size_t size = descriptor >> 8;
            const std::size_t payloadStart = offset + 8;
            const std::size_t footerStart = payloadStart + size;
            if (footerStart + 4 > data.size() || (read32(data, footerStart) & 0xFFFF) != 0xFEFF)
                throw Error("Invalid Yo-kai section footer");

            Section section;
            section.id = descriptor & 0xFF;
            std::copy_n(data.begin() + offset, 8, section.header.begin());
            section.payload.assign(data.begin() + payloadStart, data.begin() + footerStart);
            std::copy_n(data.begin() + footerStart, 4, section.footer.begin());
            consumed = 12 + size;

            if (section.payload.size() >= 12 && (read32(section.payload, 0) & 0xFFFF) == 0xFFFE)
            {
                try
                {
                    std::size_t childOffset = 0;
                    while (childOffset < section.payload.size())
                    {
                        std::size_t childSize = 0;
                        section.children.push_back(parseSection(section.payload, childOffset, childSize));
                        childOffset += childSize;
                    }
                    section.container = childOffset == section.payload.size();
                    if (!section.container) section.children.clear();
                }
                catch (const Error&)
                {
                    section.children.clear();
                    section.container = false;
                }
            }
            return section;
        }

        Section* findSection(Section& section, std::uint8_t id)
        {
            if (section.id == id) return &section;
            if (section.container)
                for (auto& child : section.children)
                    if (auto* found = findSection(child, id)) return found;
            return nullptr;
        }

        Section* childWithId(Section& section, std::uint8_t id)
        {
            const auto it = std::find_if(section.children.begin(), section.children.end(),
                [id](const Section& child) { return child.id == id; });
            return it == section.children.end() ? nullptr : &*it;
        }

        void shuffleRange(std::vector<std::uint8_t>& order, std::size_t first,
            std::size_t last, Xorshift& random)
        {
            for (std::size_t index = last; index > first; index--)
            {
                const std::size_t position = random.next(index - first + 1) + first;
                std::swap(order[position], order[index]);
            }
        }

        enum class OrderKind { Yw2, Yw3, Blasters, Busters2 };

        std::vector<std::uint8_t> reorder(std::span<const std::uint8_t> decrypted, OrderKind kind)
        {
            if (decrypted.size() < 40) throw Error("Decrypted save is too small");
            const auto body = decrypted.subspan(32, decrypted.size() - 40);
            std::size_t consumed = 0;
            Section root = parseSection(body, 0, consumed);
            if (consumed != body.size()) throw Error("Trailing Yo-kai section data");
            Section* target = findSection(root, 0xF3);
            if (!target || !target->container) throw Error("Yo-kai section container was not found");

            auto serializedCrc = [target](std::uint8_t id)
            {
                const Section* child = childWithId(*target, id);
                if (!child) throw Error("Required Yo-kai shuffle section is missing");
                const auto bytes = child->serialize();
                return crc32(bytes);
            };

            std::vector<std::uint8_t> order;
            if (kind == OrderKind::Yw2)
            {
                order = {0x01,0x03,0x0B,0x0F,0x10,0x11,0x02,0x07,
                         0x08,0x0C,0x0D,0x0E,0x12,0x14,0x15};
                Xorshift first(serializedCrc(0x01));
                Xorshift second(serializedCrc(0x07));
                shuffleRange(order, 1, 6, first);
                shuffleRange(order, 8, 14, second);
            }
            else if (kind == OrderKind::Yw3)
            {
                order = {0x01,0x03,0x0B,0x0F,0x11,0x02,0x17,0x18,0x23,0x07,0x08,
                         0x1D,0x0C,0x0D,0x0E,0x12,0x14,0x15,0x20,0x21,0x22,0x29};
                Xorshift first(serializedCrc(0x01));
                Xorshift second(serializedCrc(0x07));
                shuffleRange(order, 1, 8, first);
                shuffleRange(order, 10, 21, second);
                if (childWithId(*target, 0x2A)) order.push_back(0x2A);
            }
            else if (kind == OrderKind::Busters2)
            {
                order = {0x01,0x03,0x0B,0x0F,0x02,0x17,0x18,0x23,0x07,0x08,
                         0x1D,0x0C,0x0E,0x12,0x14,0x20,0x21,0x22,0x29};
                Xorshift first(serializedCrc(0x01));
                Xorshift second(serializedCrc(0x07));
                shuffleRange(order, 1, 7, first);
                shuffleRange(order, 9, 18, second);
                if (childWithId(*target, 0x2A)) order.push_back(0x2A);
            }
            else
            {
                order = {0x0B,0x0E,0x02,0x08,0x0D,0x12,0x0F,0x0C};
                for (const std::uint8_t seed : {0x01, 0x03, 0x07})
                {
                    Xorshift random(serializedCrc(seed));
                    shuffleRange(order, 0, 7, random);
                }
            }

            std::vector<Section> reordered;
            for (const auto& child : target->children)
                if (std::find(order.begin(), order.end(), child.id) == order.end())
                    reordered.push_back(child);
            for (const std::uint8_t id : order)
                if (const Section* child = childWithId(*target, id)) reordered.push_back(*child);
            target->children = std::move(reordered);

            const auto rebuilt = root.serialize();
            std::vector<std::uint8_t> output(decrypted.begin(), decrypted.begin() + 32);
            output.insert(output.end(), rebuilt.begin(), rebuilt.end());
            output.insert(output.end(), decrypted.end() - 8, decrypted.end());
            return output;
        }

        std::array<std::uint8_t, 16> randomKey(Xorshift& random)
        {
            std::array<std::uint8_t, 16> key{};
            for (auto& byte : key) byte = static_cast<std::uint8_t>(random.next(0x100));
            return key;
        }

        std::vector<std::array<std::uint8_t, 16>> busters2Keys(std::span<const std::uint8_t> rawHead)
        {
            if (rawHead.empty()) throw Error("Busters 2 requires its matching head.yw");
            const auto head = decryptYw(rawHead);
            std::vector<std::array<std::uint8_t, 16>> keys;
            for (std::size_t slot = 0; slot < 2; slot++)
            {
                const std::size_t base = slot * 0xA8 + 0x36F8;
                Xorshift random(read32(head, 0x0C) ^ read32(head, base + 0x38));
                std::uint32_t count = 0;
                for (std::size_t index = 0; index < 6; index++) count += read32(head, base + 0x40 + index * 4);
                for (std::uint32_t index = 0; index < (count & 0xFF); index++) random.next(0);
                keys.push_back(randomKey(random));
            }
            return keys;
        }

        std::vector<std::array<std::uint8_t, 16>> blastersKeys(std::span<const std::uint8_t> rawHead)
        {
            if (rawHead.empty()) throw Error("Blasters requires its matching head.yw or head.yw_g");
            const auto head = decryptYw(rawHead);
            const bool localized = rawHead.size() >= 15180;
            const std::size_t userLength = localized ? 0x80 : 0x78;
            const std::size_t ignoredLength = localized ? 0x1C : 0x18;
            const auto sub = [&](std::size_t field, std::size_t slot)
            {
                return read32(head, slot * userLength + 0x39C8 + ignoredLength + field * 4);
            };
            std::vector<std::array<std::uint8_t, 16>> keys;
            for (std::size_t slot = 0; slot < 3; slot++)
            {
                std::uint32_t seed = read32(head, 0x0C) ^ sub(0x0C, slot);
                if (sub(0, slot) & 0x4000) seed = ~seed;
                Xorshift random(seed);
                for (std::uint32_t index = 0; index < (sub(0x0A, slot) & 0xFF); index++) random.next(0);
                keys.push_back(randomKey(random));
            }
            return keys;
        }

        std::array<std::uint8_t, 16> yw2xKey(std::span<const std::uint8_t> rawHead)
        {
            if (rawHead.empty()) throw Error("YW2 version 2.x requires its matching head.yw");
            const auto head = decryptYw(rawHead);
            Xorshift random(read32(head, 0x0C));
            return randomKey(random);
        }

        std::array<std::uint8_t, 16> yw3Key(std::span<const std::uint8_t> rawHead)
        {
            if (rawHead.empty()) throw Error("YW3 requires its matching head.yw");
            const auto head = decryptYw(rawHead);
            std::uint32_t profile = read32(head, 0x10);
            if (profile) profile--;
            const std::size_t base = profile * 0xA8 + 0x20;
            Xorshift random(read32(head, 0x0C) ^ read32(head, base + 0x38));
            std::uint32_t count = 0;
            for (std::size_t index = 0; index < 6; index++) count += read32(head, base + 0x40 + index * 4);
            for (std::uint32_t index = 0; index < (count & 0xFF); index++) random.next(0);
            return randomKey(random);
        }

        DecryptedSave decryptAuthenticated(std::span<const std::uint8_t> data,
            std::span<const std::array<std::uint8_t, 16>> keys, SaveVariant firstVariant)
        {
            if (data.size() < 40) throw Error("Authenticated Yo-kai save is too small");
            for (std::size_t index = 0; index < keys.size(); index++)
            {
                const auto plain = decryptCcmEnvelope(data, keys[index]);
                if (!plain) continue;
                auto inner = decryptYw(*plain);
                std::vector<std::uint8_t> output(data.begin(), data.begin() + 32);
                output.insert(output.end(), inner.begin(), inner.end());
                return {std::move(output), static_cast<SaveVariant>(static_cast<int>(firstVariant) + index)};
            }
            throw Error("Yo-kai save authentication failed; the save and head files may not match");
        }

        std::vector<std::uint8_t> encryptAuthenticated(std::span<const std::uint8_t> decrypted,
            std::span<const std::uint8_t, 16> key, OrderKind orderKind)
        {
            const auto ordered = reorder(decrypted, orderKind);
            const auto inner = encryptYw(std::span<const std::uint8_t>(ordered).subspan(32));
            std::array<std::uint8_t, 12> nonce{};
            std::copy_n(ordered.begin(), nonce.size(), nonce.begin());
            return encryptCcmEnvelope(inner, key, nonce);
        }
    }

    std::uint32_t crc32(std::span<const std::uint8_t> data)
    {
        std::uint32_t crc = 0xFFFFFFFF;
        for (std::uint8_t byte : data)
        {
            crc ^= byte;
            for (int bit = 0; bit < 8; bit++)
                crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
        return ~crc;
    }

    bool hasValidYwChecksum(std::span<const std::uint8_t> data)
    {
        return data.size() >= 8 && crc32(data.first(data.size() - 8)) == read32(data, data.size() - 8);
    }

    std::vector<std::uint8_t> decryptYw(std::span<const std::uint8_t> data)
    {
        if (!hasValidYwChecksum(data)) throw Error("Yo-kai save checksum does not match");
        const std::uint32_t seed = read32(data, data.size() - 4);
        auto output = apply(data.first(data.size() - 8), seed);
        output.insert(output.end(), data.end() - 8, data.end());
        return output;
    }

    std::vector<std::uint8_t> encryptYw(std::span<const std::uint8_t> data)
    {
        if (data.size() < 8) throw Error("Yo-kai save is too small");
        const std::uint32_t seed = read32(data, data.size() - 4);
        auto output = apply(data.first(data.size() - 8), seed);
        output.insert(output.end(), data.end() - 8, data.end());
        write32(output, output.size() - 8, crc32(std::span<const std::uint8_t>(output).first(output.size() - 8)));
        return output;
    }

    std::optional<std::vector<std::uint8_t>> decryptCcmEnvelope(
        std::span<const std::uint8_t> data, std::span<const std::uint8_t, 16> key)
    {
        if (data.size() < 32) return std::nullopt;
        std::array<std::uint8_t, 12> nonce{};
        std::copy_n(data.begin(), nonce.size(), nonce.begin());
        const auto plain = ccmCrypt(data.subspan(32), key, nonce);
        const auto mac = ccmMac(plain, key, nonce);
        const auto s0 = aesBlock(counterBlock(nonce, 0), key);
        std::array<std::uint8_t, 16> expected{};
        for (std::size_t index = 0; index < expected.size(); index++) expected[index] = mac[index] ^ s0[index];
        if (!std::equal(expected.begin(), expected.end(), data.begin() + 16)) return std::nullopt;
        return plain;
    }

    std::vector<std::uint8_t> encryptCcmEnvelope(std::span<const std::uint8_t> plain,
        std::span<const std::uint8_t, 16> key, std::span<const std::uint8_t, 12> nonce)
    {
        const auto mac = ccmMac(plain, key, nonce);
        const auto s0 = aesBlock(counterBlock(nonce, 0), key);
        std::vector<std::uint8_t> output;
        output.reserve(32 + plain.size());
        output.insert(output.end(), nonce.begin(), nonce.end());
        output.insert(output.end(), 4, 0);
        for (std::size_t index = 0; index < 16; index++) output.push_back(mac[index] ^ s0[index]);
        const auto cipher = ccmCrypt(plain, key, nonce);
        output.insert(output.end(), cipher.begin(), cipher.end());
        return output;
    }

    DecryptedSave decryptSave(Game game, std::span<const std::uint8_t> data,
        std::span<const std::uint8_t> head)
    {
        if (game == Game::YW1) return {decryptYw(data), SaveVariant::Yw1};
        if (game == Game::YW2)
        {
            constexpr std::array<std::uint8_t, 16> fixed = {
                '5','+','N','I','8','W','V','q','0','9','V','7','L','I','5','w'};
            if (const auto plain = decryptCcmEnvelope(data, fixed))
            {
                auto inner = decryptYw(*plain);
                std::vector<std::uint8_t> output(data.begin(), data.begin() + 32);
                output.insert(output.end(), inner.begin(), inner.end());
                return {std::move(output), SaveVariant::Yw2};
            }
            const auto key = yw2xKey(head);
            return decryptAuthenticated(data, std::span<const std::array<std::uint8_t, 16>>(&key, 1),
                SaveVariant::Yw2X);
        }
        if (game == Game::YW3)
        {
            const auto key = yw3Key(head);
            return decryptAuthenticated(data, std::span<const std::array<std::uint8_t, 16>>(&key, 1),
                SaveVariant::Yw3);
        }
        if (game == Game::Blasters)
        {
            const auto keys = blastersKeys(head);
            return decryptAuthenticated(data, keys, SaveVariant::Blasters1);
        }
        const auto keys = busters2Keys(head);
        return decryptAuthenticated(data, keys, SaveVariant::Busters2Slot1);
    }

    std::vector<std::uint8_t> encryptSave(Game game, std::span<const std::uint8_t> decrypted,
        SaveVariant variant, std::span<const std::uint8_t> head)
    {
        if (game == Game::YW1 && variant == SaveVariant::Yw1) return encryptYw(decrypted);
        if (game == Game::YW2)
        {
            if (variant == SaveVariant::Yw2)
            {
                constexpr std::array<std::uint8_t, 16> fixed = {
                    '5','+','N','I','8','W','V','q','0','9','V','7','L','I','5','w'};
                return encryptAuthenticated(decrypted, fixed, OrderKind::Yw2);
            }
            if (variant == SaveVariant::Yw2X)
            {
                const auto key = yw2xKey(head);
                return encryptAuthenticated(decrypted, key, OrderKind::Yw2);
            }
        }
        if (game == Game::YW3 && variant == SaveVariant::Yw3)
        {
            const auto key = yw3Key(head);
            return encryptAuthenticated(decrypted, key, OrderKind::Yw3);
        }
        if (game == Game::Blasters && variant >= SaveVariant::Blasters1 && variant <= SaveVariant::Blasters3)
        {
            const auto keys = blastersKeys(head);
            return encryptAuthenticated(decrypted, keys[static_cast<int>(variant) - static_cast<int>(SaveVariant::Blasters1)],
                OrderKind::Blasters);
        }
        if (game == Game::Busters2 && variant >= SaveVariant::Busters2Slot1 && variant <= SaveVariant::Busters2Slot2)
        {
            const auto keys = busters2Keys(head);
            return encryptAuthenticated(decrypted, keys[static_cast<int>(variant) - static_cast<int>(SaveVariant::Busters2Slot1)],
                OrderKind::Busters2);
        }
        throw Error("Save variant does not match the selected game");
    }

    const char* variantName(SaveVariant variant)
    {
        switch (variant)
        {
            case SaveVariant::Yw1: return "yw1";
            case SaveVariant::Yw2: return "yw2";
            case SaveVariant::Yw2X: return "yw2x";
            case SaveVariant::Yw3: return "yw3";
            case SaveVariant::Blasters1: return "blasters:1";
            case SaveVariant::Blasters2: return "blasters:2";
            case SaveVariant::Blasters3: return "blasters:3";
            case SaveVariant::Busters2Slot1: return "busters2:1";
            case SaveVariant::Busters2Slot2: return "busters2:2";
        }
        return "unknown";
    }
}
