#include "filesystem.hpp"
#include <algorithm> // std::find, std::sort

namespace dfs {

FileSystem::FileSystem() {
    inode_table[0] = Inode(InodeType::Dir); // creates root inode
    inode_bmap.set(0);                      // 0th node is used
    directories.emplace(0, Directory());    // add root directory
}

DfsRet<u32> FileSystem::create_directory(u32 parent_inode, const std::string &name) {
    if (directories.at(parent_inode).contains(name))
        return std::unexpected(DfsError::AlreadyExists);

    DfsRet<u32> inode = alloc_inode(InodeType::Dir);
    if (inode.has_value()) {                                // if an error is not returned by alloc_inode
        directories.emplace(*inode, Directory());           // add to dictionaries
        directories.at(parent_inode).emplace(name, *inode); // name, inode pair of the dir in the added Directory
    }
    return inode;
}

// Creates a directory in root
DfsRet<u32> FileSystem::create_directory(const std::string &name) { return create_directory(0, name); }

DfsRet<u32> FileSystem::create_file(u32 parent_inode, const std::string &name) {
    if (directories.at(parent_inode).contains(name))
        return std::unexpected(DfsError::AlreadyExists);

    DfsRet<u32> inode = alloc_inode(InodeType::File);

    if (inode.has_value())
        directories.at(parent_inode).emplace(name, *inode); // currently always creates at root

    return inode;
}

// Creates a file in root
DfsRet<u32> FileSystem::create_file(const std::string &name) { return create_file(0, name); }

// returns inode number if found
DfsRet<u32> FileSystem::lookup(u32 dir_inode, const std::string &name) const {
    if (dir_inode >= NUM_INODES)
        return std::unexpected(DfsError::OutOfRange);

    if (!inode_bmap.test(dir_inode)) // the dir must be allocated
        return std::unexpected(DfsError::NotFound);

    if (inode_table[dir_inode].type != InodeType::Dir) // must pass a dir inode
        return std::unexpected(DfsError::IncorrectType);

    const dfs::Directory &directory = directories.at(dir_inode);
    const dfs::Directory::const_iterator key = directory.find(name);
    if (key == directory.end()) {
        return std::unexpected(DfsError::NotFound);
    }

    return key->second; // the inode number associated with the name
}

/**
 * @param path: The path you want resolved with '/' delimiter. A leading '/' is root. Currently doesn't work with
 * relative paths.
 * @return DfsRet<u32> Inode number of the trailing part of the path if found
 */
DfsRet<u32> FileSystem::resolve_path(const std::string &path) const {
    auto it = path.begin();
    if (path.empty() || path.at(0) != '/')
        return std::unexpected(DfsError::InvalidPath);

    u32 inode = 0;

    while (it != path.end()) {

        if (*it == '/') {
            ++it;
            continue; // skip '/'
        }
        auto next = std::find(it, path.end(), '/');

        std::string sec(it, next); // section of the string

        auto res = lookup(inode, sec); // ret inode idx

        if (!res.has_value())
            return std::unexpected(res.error());

        inode = *res;
        it = next;
    }

    return inode;
}

// TODO: list mtime, length, and mode (when added)
DfsRet<std::vector<std::string>> FileSystem::list_dir(const std::string &path) const {
    auto inode = resolve_path(path);
    if (!inode)
        return std::unexpected(inode.error());

    if (inode_table[*inode].type != InodeType::Dir)
        return std::unexpected(DfsError::IncorrectType);

    std::vector<std::string> ret{};
    for (const auto &[name, node] : directories.at(*inode)) {
        ret.emplace_back(name);
    }

    std::sort(ret.begin(), ret.end()); // sorting not really needed
    return ret;
};

// Shell commands
DfsRet<void> FileSystem::mkdir(const std::string &path) {
    auto pos = path.find_last_of('/');

    const std::string parent_path = (pos == 0) ? "/" : path.substr(0, pos);
    const std::string name = path.substr(pos + 1);

    auto parent = resolve_path(parent_path);

    if (!parent)
        return std::unexpected(parent.error());

    auto result = create_directory(*parent, name);

    if (!result)
        return std::unexpected(result.error());

    return {};
}

// Remove empty directory
DfsRet<void> FileSystem::rmdir(const std::string &path) {
    auto pos = path.find_last_of('/');

    if (path.empty() || pos == std::string::npos)
        return std::unexpected(DfsError::InvalidPath);

    const std::string name = path.substr(pos + 1);
    auto parent = resolve_path(pos == 0 ? "/" : path.substr(0, pos));

    auto target = lookup(*parent, name);
    if (!target)
        return std::unexpected(target.error());

    if (inode_table[*target].type != InodeType::Dir)
        return std::unexpected(DfsError::IncorrectType);
    if (!directories.at(*target).empty())
        return std::unexpected(DfsError::NotEmpty);

    directories.erase(*target);
    directories.at(*parent).erase(name);
    return free_inode(*target);
}

DfsRet<void> FileSystem::init_unix_dirs() {
    mkdir("/bin");
    mkdir("/boot");
    mkdir("/etc");
    mkdir("/home");
    mkdir("/lib");
    mkdir("/media");
    mkdir("/mnt");
    mkdir("/opt");
    mkdir("/root");
    mkdir("/sbin");
    mkdir("/srv");
    mkdir("/tmp");
    mkdir("/usr");
    mkdir("/var");

    mkdir("/usr/bin");
    mkdir("/usr/include");
    mkdir("/usr/lib");
    mkdir("/usr/sbin");

    mkdir("/var/cache");
    mkdir("/var/log");
    mkdir("/var/spool");
    mkdir("/var/tmp");

    return {};
}

DfsRet<u32> FileSystem::alloc_inode(InodeType type) {
    for (usize i{1}; i < inode_bmap.size(); ++i) {
        if (!inode_bmap.test(i)) {        // at the first 0 bit
            inode_table[i] = Inode(type); // add the Inode to the table
            inode_bmap.set(i);            // toggle the bmap value at i to 1
            return (u32)i;                // return the inode index
        }
    }

    return std::unexpected(DfsError::NoFreeInodes); // no 0 bits in inode_bmap
}

DfsRet<void> FileSystem::free_inode(u32 inode) {
    if (inode == 0 || inode >= NUM_INODES) // can't free root
        return std::unexpected(DfsError::OutOfRange);

    if (!inode_bmap.test(inode))
        return std::unexpected(DfsError::AlreadyFree); // block double free

    inode_table[inode] = Inode(InodeType::Free); // set to a new Free Inode
    inode_bmap.reset(inode);                     // mark free in the bitmap
    return {};
}

DfsRet<u32> FileSystem::alloc_block() {
    for (usize i{0}; i < data_bmap.size(); ++i) {
        if (!data_bmap.test(i)) {
            blocks[i] = Block{};
            data_bmap.set(i);
            return (u32)i;
        }
    }

    return std::unexpected(DfsError::NoFreeBlocks);
}

DfsRet<void> FileSystem::free_block(u32 block) {
    if (block >= NUM_BLOCKS)
        return std::unexpected(DfsError::OutOfRange);

    if (!data_bmap.test(block))
        return std::unexpected(DfsError::AlreadyFree); // block double free

    data_bmap.reset(block); // set bit at block to 0
    return {};              // return void for expected
}

// Non-member functions

std::unique_ptr<FileSystem> make_unix_fs() {
    auto fs = std::make_unique<FileSystem>();
    fs->init_unix_dirs();
    return fs;
}

} // namespace dfs
