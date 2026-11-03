#include "filesystem.hpp"
#include <algorithm> // std::find, std::sort

// Directory validation, entry lookup, and directory listing
namespace dfs {
// TODO: list mtime, length, and mode
Result<std::vector<std::string>> FileSystem::list_dir(std::string_view path) const {
    auto inode = resolve_path(path);
    if (!inode)
        return std::unexpected(inode.error());

    if (inode_table[*inode].type != InodeType::Dir)
        return std::unexpected(Error::IncorrectType);

    std::vector<std::string> ret{};
    for (const auto &[name, node] : directories.at(*inode))
        ret.emplace_back(name);

    std::sort(ret.begin(), ret.end()); // sorting not really needed, but it's nice
    return ret;
}

// PRIVATE
Result<void> FileSystem::validate_dir(u32 inode) const {
    if (inode >= NUM_INODES)
        return std::unexpected(Error::OutOfRange);

    if (!inode_bmap.test(inode)) // the dir must be allocated
        return std::unexpected(Error::NotFound);

    if (inode_table[inode].type != InodeType::Dir) // must pass a dir inode
        return std::unexpected(Error::IncorrectType);
    return {};
}

} // namespace dfs
