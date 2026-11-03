#pragma once
#include <cstdint> // std::uint8_t, std::uint32_t
#include <ctime>   // std::time_t
#include <expected>
#include <functional> // std::hash
#include <string>
#include <string_view>
#include <unordered_map>

namespace dfs {
// types
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using usize = std::size_t;

// constants
constexpr usize BLOCK_SIZE = 4096;
constexpr usize NUM_BLOCKS = 64;
constexpr usize NUM_INODES = 128;
constexpr u32 ROOT_INODE = 0;

static_assert(ROOT_INODE < NUM_INODES);

enum class Error : u8 {
    NoFreeInodes,
    NoFreeBlocks,
    NotFound,
    NotEmpty,
    OutOfRange,
    IncorrectType,
    InvalidPath,
    AlreadyExists,
    AlreadyFree,
};

template <typename T> using Result = std::expected<T, Error>;

// https://www.geeksforgeeks.org/cpp/implement-custom-hash-functions-for-user-defined-types-in-std-unordered_map/ ->
// https://www.fluentcpp.com/2017/06/09/search-set-another-type-key/
// custom hash functor so we don't unnecessarily copy strings
struct StringHash {
    using is_transparent = void; // enable heterogeneous lookup
    usize operator()(std::string_view str) const noexcept { return std::hash<std::string_view>{}(str); }
};
using Directory =
    std::unordered_map<std::string, u32, StringHash, std::equal_to<>>;

} // namespace dfs
