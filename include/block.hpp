#pragma once
#include "types.hpp"
#include <array>

namespace dfs {
struct Block {
    std::array<std::byte, BLOCK_SIZE> data{};
};
} // namespace dfs
