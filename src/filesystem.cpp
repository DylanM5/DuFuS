#include "filesystem.hpp"
#include <algorithm> // std::find, std::sort

namespace dfs {
// utility funcs
namespace {
struct ParsePath {
    std::string_view parent;
    std::string_view name;
};

Result<ParsePath> parse_path(std::string_view path) {
    if (path.empty() || path[0] != '/')
        return std::unexpected(Error::InvalidPath);

    const auto pos = path.find_last_of('/');
    std::string_view name = path.substr(pos + 1);

    if (name.empty())
        return std::unexpected(Error::InvalidPath);

    return ParsePath{pos == 0 ? "/" : path.substr(0, pos), name};
}
} // namespace

FileSystem::FileSystem() {
    inode_table[0] = Inode(InodeType::Dir); // creates root inode
    inode_bmap.set(0);
    directories.emplace(0, Directory()); // add root directory
}

/**
 * Public, but create_* should be private
 * @param name: Must be a unique name in the parent inode and may not contain a forward slash
 */
Result<u32> FileSystem::create_dir(u32 parent_inode, std::string_view name) {
    if (auto v = validate_dir(parent_inode); !v)
        return std::unexpected(v.error());
    if (name.empty() || name.contains('/'))
        return std::unexpected(Error::InvalidPath);

    if (directories.at(parent_inode).contains(std::string{name}))
        return std::unexpected(Error::AlreadyExists);

    Result<u32> inode = alloc_inode(InodeType::Dir);
    if (inode.has_value()) {                                // if an error is not returned by alloc_inode
        directories.emplace(*inode, Directory());           // add to dictionaries
        directories.at(parent_inode).emplace(name, *inode); // name, inode pair of the dir in the added Directory
    }
    return inode;
}

// Creates a directory in root
Result<u32> FileSystem::create_dir(std::string_view name) { return create_dir(0, name); }

Result<u32> FileSystem::create_file(u32 parent_inode, std::string_view name) {
    if (auto v = validate_dir(parent_inode); !v)
        return std::unexpected(v.error());
    if (name.empty() || name.contains('/'))
        return std::unexpected(Error::InvalidPath);

    if (directories.at(parent_inode).contains(std::string{name}))
        return std::unexpected(Error::AlreadyExists);

    Result<u32> inode = alloc_inode(InodeType::File);

    if (inode.has_value())
        directories.at(parent_inode).emplace(name, *inode); // currently always creates at root

    return inode;
}

// Creates a file in root
Result<u32> FileSystem::create_file(std::string_view name) { return create_file(0, name); }

// returns inode number if found
Result<u32> FileSystem::lookup(u32 dir_inode, std::string_view name) const {
    if (auto v = validate_dir(dir_inode); !v)
        return std::unexpected(v.error());

    const Directory &directory = directories.at(dir_inode);
    const Directory::const_iterator key = directory.find(std::string{name});
    if (key == directory.end()) {
        return std::unexpected(Error::NotFound);
    }

    u32 value = key->second;
    return value;
}

/**
 * @param path: The path you want resolved with '/' delimiter. A leading '/' is root. Currently doesn't work with
 * relative paths.
 * @return DfsRet<u32> Inode number of the trailing part of the path if found
 */
Result<u32> FileSystem::resolve_path(std::string_view path) const {
    auto it = path.begin();
    if (path.empty() || path.at(0) != '/')
        return std::unexpected(Error::InvalidPath);

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
Result<std::vector<std::string>> FileSystem::list_dir(std::string_view path) const {
    auto inode = resolve_path(path);
    if (!inode)
        return std::unexpected(inode.error());

    if (inode_table[*inode].type != InodeType::Dir)
        return std::unexpected(Error::IncorrectType);

    std::vector<std::string> ret{};
    for (const auto &[name, node] : directories.at(*inode)) {
        ret.emplace_back(name);
    }

    std::sort(ret.begin(), ret.end()); // sorting not really needed
    return ret;
}

// Shell commands
Result<void> FileSystem::mkdir(std::string_view path) {
    auto p = parse_path(path);
    if (!p)
        return std::unexpected(p.error());

    auto parent = resolve_path(p->parent);
    if (!parent)
        return std::unexpected(parent.error());

    auto result = create_dir(*parent, p->name);

    if (!result)
        return std::unexpected(result.error());

    return {};
}

// Remove empty directory
Result<void> FileSystem::rmdir(std::string_view path) {
    auto p = parse_path(path);
    if (!p)
        return std::unexpected(p.error());

    auto parent = resolve_path(p->parent);
    if (!parent)
        return std::unexpected(parent.error());

    auto target = lookup(*parent, p->name);
    if (!target)
        return std::unexpected(target.error());

    if (inode_table[*target].type != InodeType::Dir)
        return std::unexpected(Error::IncorrectType);
    if (!directories.at(*target).empty())
        return std::unexpected(Error::NotEmpty);

    auto ret = free_inode(*target);
    if (!ret.has_value()) // don't erase from dictionaries if free_node throws
        return ret;

    directories.at(*parent).erase(std::string(p->name));
    directories.erase(*target);
    return ret;
}

Result<void> FileSystem::init_unix_dirs() {
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

// PRIVATE

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
    if (inode == 0 || inode >= NUM_INODES) // can't free root
        return std::unexpected(Error::OutOfRange);

    if (!inode_bmap.test(inode))
        return std::unexpected(Error::AlreadyFree); // block double free

    inode_table[inode] = Inode(InodeType::Free); // set to a new Free Inode
    inode_bmap.reset(inode);                     // mark free in the bitmap
    return {};
}

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
        return std::unexpected(Error::AlreadyFree); // block double free

    data_bmap.reset(block);
    return {}; // return void for expected
}

Result<void> FileSystem::validate_dir(u32 inode) const {
    if (inode >= NUM_INODES)
        return std::unexpected(Error::OutOfRange);

    if (!inode_bmap.test(inode)) // the dir must be allocated
        return std::unexpected(Error::NotFound);

    if (inode_table[inode].type != InodeType::Dir) // must pass a dir inode
        return std::unexpected(Error::IncorrectType);
    return {};
}

// Non-member functions

std::unique_ptr<FileSystem> make_unix_fs() {
    auto fs = std::make_unique<FileSystem>();
    fs->init_unix_dirs();
    return fs;
}

} // namespace dfs
