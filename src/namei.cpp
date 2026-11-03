#include "filesystem.hpp"

// Pathname and namespace operations

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
void FileSystem::update_dir_time(u32 parent) {
    auto &parent_inode = inode_table[parent];
    parent_inode.mtime = parent_inode.ctime = time_now();
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

    auto &dir = directories.at(*parent);
    auto it = dir.find(p->name);
    dir.erase(it);
    directories.erase(*target);
    update_dir_time(*parent);

    return ret;
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

    if (directories.at(parent_inode).contains(name))
        return std::unexpected(Error::AlreadyExists);

    Result<u32> inode = alloc_inode(InodeType::Dir);
    if (inode.has_value()) {                                // if an error is not returned by alloc_inode
        directories.emplace(*inode, Directory());           // add to dictionaries
        directories.at(parent_inode).emplace(name, *inode); // name, inode pair of the dir in the added Directory
        update_dir_time(parent_inode);
    }
    return inode;
}

// Creates a directory in root
Result<u32> FileSystem::create_dir(std::string_view name) { return create_dir(ROOT_INODE, name); }

Result<u32> FileSystem::create_file(u32 parent_inode, std::string_view name) {
    if (auto v = validate_dir(parent_inode); !v)
        return std::unexpected(v.error());
    if (name.empty() || name.contains('/'))
        return std::unexpected(Error::InvalidPath);

    if (directories.at(parent_inode).contains(name))
        return std::unexpected(Error::AlreadyExists);

    Result<u32> inode = alloc_inode(InodeType::File);

    if (inode.has_value()) {
        directories.at(parent_inode).emplace(name, *inode);
        update_dir_time(parent_inode);
    }

    return inode;
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

        std::string_view sec(it, next); // section of the string

        auto res = lookup(inode, sec);

        if (!res.has_value())
            return std::unexpected(res.error());

        inode = *res;
        it = next;
    }

    return inode;
}

/**
 * @return inode number if found
 */
Result<u32> FileSystem::lookup(u32 dir_inode, std::string_view name) const {
    if (auto v = validate_dir(dir_inode); !v)
        return std::unexpected(v.error());

    const Directory &directory = directories.at(dir_inode);
    const Directory::const_iterator key = directory.find(name);
    if (key == directory.end()) {
        return std::unexpected(Error::NotFound);
    }

    u32 value = key->second;
    return value;
}

// Creates a file in root
Result<u32> FileSystem::create_file(std::string_view name) { return create_file(0, name); }

} // namespace dfs
