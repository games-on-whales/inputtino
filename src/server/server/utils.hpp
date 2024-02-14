#pragma once

#include <ranges>
#include <vector>
#include <algorithm>
#include <inputtino/input.hpp>

/**
 * Copies out a range into a vector
 * see: https://stackoverflow.com/a/63116423
 */
template<std::ranges::range R>
auto to_vector(R &&r) {
    std::vector<std::ranges::range_value_t<decltype(r)>> v;

    if constexpr (std::ranges::sized_range<decltype(r)>) {
        v.reserve(std::ranges::size(r));
    }

    std::ranges::copy(r, std::back_inserter(v));
    return v;
}

inline const char *get_env(const char *tag, const char *def = nullptr) noexcept {
    const char *ret = std::getenv(tag);
    return ret ? ret : def;
}

inline std::string to_lower(std::string_view str) {
    std::string result(str);
    std::transform(str.begin(), str.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * Since we can't switch() on strings we use hashes instead.
 * Adapted from https://stackoverflow.com/a/46711735/3901988
 */
constexpr uint32_t hash(const std::string_view data) noexcept {
    uint32_t hash = 5385;
    for (const auto &e: data)
        hash = ((hash << 5) + hash) + e;
    return hash;
}