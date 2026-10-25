#pragma once // Ensures this file is only included once per translation unit

#include <cstddef>
#include <cstdint>

// Project-wide type aliases (putting them in your own namespace is best
// practice)
namespace dakerr {
using usize = std::size_t;
using u64 = std::uint64_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;

// Lightweight cast helper
template <typename T> [[nodiscard]] constexpr usize sz(T v) noexcept {
    return static_cast<usize>(v);
}
} // namespace dakerr