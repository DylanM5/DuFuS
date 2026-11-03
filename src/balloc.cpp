#include "filesystem.hpp"
// Block allocation and deletion

namespace dfs {
Result<u32> FileSystem::alloc_block() {
    for (usize i{0}; i < data_bmap.size(); ++i) {
        if (!data_bmap.test(i)) {
            blocks[i] = Block{};
            data_bmap.set(i);
            return (u32)i;
        }
    }

    return std::unexpected(Error::NoFreeBlocks);
}

Result<void> FileSystem::free_block(u32 block) {
    if (block >= NUM_BLOCKS)
        return std::unexpected(Error::OutOfRange);

    if (!data_bmap.test(block))
        return std::unexpected(Error::AlreadyFree); // not sure if double freeing matters since resetting 0 is still 0,
                                                    // but good to throw error anyway?

    data_bmap.reset(block);
    return {};
}

} // namespace dfs
