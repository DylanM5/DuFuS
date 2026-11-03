#include "filesystem.hpp"
// Inode allocation and deletion

namespace dfs {
Result<u32> FileSystem::alloc_inode(InodeType type) {
    for (usize i{1}; i < inode_bmap.size(); ++i) {
        if (!inode_bmap.test(i)) {        // at the first 0 bit
            inode_table[i] = Inode(type); // add the Inode to the table
            inode_bmap.set(i);            // toggle the bmap value at i to 1
            return (u32)i;                // return the inode index
        }
    }

    return std::unexpected(Error::NoFreeInodes); // no 0 bits in inode_bmap
}

Result<void> FileSystem::free_inode(u32 inode) {
    if (inode == ROOT_INODE || inode >= NUM_INODES) // can't free root
        return std::unexpected(Error::OutOfRange);

    if (!inode_bmap.test(inode))
        return std::unexpected(Error::AlreadyFree); // block double free

    inode_table[inode] = Inode(InodeType::Free); // set to a new Free Inode
    inode_bmap.reset(inode);                     // mark free in the bitmap
    return {};
}

} // namespace dfs
