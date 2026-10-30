#include "filesystem.hpp"

namespace dfs {

FileSystem::FileSystem() {
    inode_table[0] = Inode(InodeType::Dir); // creates root inode
    inode_bmap.set(0);                      // 0th node is used
    directories.emplace(0, Directory());    // add root directory
}

DfsRet<u32> FileSystem::create_directory(u32 parent_inode, const std::string &name) {
    if (directories.at(parent_inode).contains(name))
        return std::unexpected(DfsError::AlreadyExists);

    DfsRet<u32> inode = create_inode(InodeType::Dir);
    if (inode.has_value()) {                                // if an error is not returned by create_inode
        directories.emplace(*inode, Directory());           // add to dictionaries
        directories.at(parent_inode).emplace(name, *inode); // name, inode pair of the dir in the added Directory
    }
    return inode;
}

DfsRet<u32> FileSystem::create_directory(const std::string &name) { return create_directory(0, name); }

DfsRet<u32> FileSystem::create_file(u32 parent_inode, const std::string &name) {
    if (directories.at(parent_inode).contains(name))
        return std::unexpected(DfsError::AlreadyExists);

    DfsRet<u32> inode = create_inode(InodeType::File);

    if (inode.has_value())
        directories.at(parent_inode).emplace(name, *inode); // currently always creates at root

    return inode;
}

DfsRet<u32> FileSystem::create_file(const std::string &name) { return create_file(0, name); }

DfsRet<u32> FileSystem::create_inode(InodeType type) {
    for (usize i{1}; i < inode_bmap.size(); ++i) {
        if (!inode_bmap.test(i)) {        // at the first 0 bit
            inode_table[i] = Inode(type); // add the Inode to the table
            inode_bmap.set(i);            // toggle the bmap value at i to 1
            return (u32)i;                // return the inode index
        }
    }

    return std::unexpected(DfsError::NoFreeInodes); // no 0 bits in inode_bmap
}

DfsRet<u32> FileSystem::allocate_block() {
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
 * @param path: The path you want resolved with '/' delimiter. A leading '/' is root.
 * @return DfsRet<u32> Inode number of the trailing part of the path if found
 */
DfsRet<u32> FileSystem::resolve_path(const std::string &path) const {
    auto it = path.begin();
    u32 inode = 0; // TODO: relative paths. currently always starts at root

    while (it != path.end()) {

        if (*it == '/') {
            ++it;
            continue;
        }
        auto next = std::find(it, path.end(), '/');

        std::string sec(it, next); // section of the string

        auto res = lookup(inode, sec);

        if (!res.has_value())
            return std::unexpected(res.error());

        inode = *res;
        it = next;
    }

    return inode;
}

DfsRet<void> FileSystem::mkdir(const std::string &path) {
    auto pos = path.find_last_of('/');

    const std::string parent_path = path.substr(0, pos);
    const std::string name = path.substr(pos + 1);

    auto parent = resolve_path(parent_path);

    if (!parent)
        return std::unexpected(parent.error());

    auto result = create_directory(*parent, name);

    if (!result)
        return std::unexpected(result.error());

    return {};
}

void FileSystem::initialize_unix_fs() {
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
}

} // namespace dfs
