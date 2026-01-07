#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <expected>
#include <cstring>
#include <bit>
#include "protocol.hpp"

namespace dandy {

// Byte buffer writer (little-endian)
class ByteWriter {
public:
    ByteWriter() = default;
    explicit ByteWriter(size_t reserve) { buffer_.reserve(reserve); }

    void write_u8(uint8_t value) {
        buffer_.push_back(value);
    }

    void write_u16(uint16_t value) {
        if constexpr (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }
        buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    void write_u32(uint32_t value) {
        if constexpr (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }
        buffer_.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        buffer_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    void write_u64(uint64_t value) {
        if constexpr (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }
        for (int i = 0; i < 8; ++i) {
            buffer_.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
        }
    }

    void write_i16(int16_t value) {
        write_u16(static_cast<uint16_t>(value));
    }

    void write_string(const std::string& str) {
        write_u8(static_cast<uint8_t>(std::min(str.size(), size_t{255})));
        for (size_t i = 0; i < std::min(str.size(), size_t{255}); ++i) {
            buffer_.push_back(static_cast<uint8_t>(str[i]));
        }
    }

    void write_bytes(std::span<const uint8_t> data) {
        buffer_.insert(buffer_.end(), data.begin(), data.end());
    }

    void write_coord(const Coord& c) {
        write_i16(c.x);
        write_i16(c.y);
    }

    [[nodiscard]] std::vector<uint8_t> take() { return std::move(buffer_); }
    [[nodiscard]] const std::vector<uint8_t>& data() const { return buffer_; }
    [[nodiscard]] size_t size() const { return buffer_.size(); }

private:
    std::vector<uint8_t> buffer_;
};

// Byte buffer reader (little-endian)
class ByteReader {
public:
    explicit ByteReader(std::span<const uint8_t> data) : data_(data) {}

    [[nodiscard]] bool has_bytes(size_t n) const {
        return pos_ + n <= data_.size();
    }

    [[nodiscard]] size_t remaining() const {
        return data_.size() - pos_;
    }

    [[nodiscard]] std::expected<uint8_t, ErrorCode> read_u8() {
        if (!has_bytes(1)) return std::unexpected(ErrorCode::InvalidFormat);
        return data_[pos_++];
    }

    [[nodiscard]] std::expected<uint16_t, ErrorCode> read_u16() {
        if (!has_bytes(2)) return std::unexpected(ErrorCode::InvalidFormat);
        uint16_t value = static_cast<uint16_t>(data_[pos_]) |
                        (static_cast<uint16_t>(data_[pos_ + 1]) << 8);
        pos_ += 2;
        if constexpr (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }
        return value;
    }

    [[nodiscard]] std::expected<uint32_t, ErrorCode> read_u32() {
        if (!has_bytes(4)) return std::unexpected(ErrorCode::InvalidFormat);
        uint32_t value = static_cast<uint32_t>(data_[pos_]) |
                        (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                        (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                        (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
        pos_ += 4;
        if constexpr (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }
        return value;
    }

    [[nodiscard]] std::expected<uint64_t, ErrorCode> read_u64() {
        if (!has_bytes(8)) return std::unexpected(ErrorCode::InvalidFormat);
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value |= static_cast<uint64_t>(data_[pos_ + i]) << (i * 8);
        }
        pos_ += 8;
        if constexpr (std::endian::native == std::endian::big) {
            value = std::byteswap(value);
        }
        return value;
    }

    [[nodiscard]] std::expected<int16_t, ErrorCode> read_i16() {
        auto result = read_u16();
        if (!result) return std::unexpected(result.error());
        return static_cast<int16_t>(*result);
    }

    [[nodiscard]] std::expected<std::string, ErrorCode> read_string() {
        auto len_result = read_u8();
        if (!len_result) return std::unexpected(len_result.error());
        uint8_t len = *len_result;
        if (!has_bytes(len)) return std::unexpected(ErrorCode::InvalidFormat);
        std::string str(reinterpret_cast<const char*>(&data_[pos_]), len);
        pos_ += len;
        return str;
    }

    [[nodiscard]] std::expected<std::span<const uint8_t>, ErrorCode> read_bytes(size_t n) {
        if (!has_bytes(n)) return std::unexpected(ErrorCode::InvalidFormat);
        auto result = data_.subspan(pos_, n);
        pos_ += n;
        return result;
    }

    [[nodiscard]] std::expected<Coord, ErrorCode> read_coord() {
        auto x = read_i16();
        if (!x) return std::unexpected(x.error());
        auto y = read_i16();
        if (!y) return std::unexpected(y.error());
        return Coord{*x, *y};
    }

private:
    std::span<const uint8_t> data_;
    size_t pos_{0};
};

} // namespace dandy
