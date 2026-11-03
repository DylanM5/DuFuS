#pragma once
#include "block.hpp"
#include "inode.hpp"
#include "types.hpp"

#include <bitset>
#include <memory> // std::unique_ptr, std::make_unique
#include <string>
#include <vector>

// Dylan's File System
namespace dfs {

class FileSystem {
  private:
    std::bitset<NUM_INODES> inode_bmap;
    std::array<Inode, NUM_INODES> inode_table;

    std::bitset<NUM_BLOCKS> data_bmap;
    std::array<Block, NUM_BLOCKS> blocks;

    std::unordered_map<u32, Directory> directories; // map of inodes and their directories

  public:
    FileSystem() {
        inode_table[ROOT_INODE] = Inode(InodeType::Dir);
        inode_bmap.set(ROOT_INODE);
        directories.emplace(ROOT_INODE, Directory());
    };
    ~FileSystem() = default;

    FileSystem(const FileSystem &) = delete;
    FileSystem(FileSystem &&) = delete;
    FileSystem &operator=(const FileSystem &) = delete;
    FileSystem &operator=(FileSystem &&) = delete;

    // Create directory in root
    Result<u32> create_dir(std::string_view name);
    Result<u32> create_dir(u32 parent_inode, std::string_view name);

    // Create file in root
    Result<u32> create_file(std::string_view name);
    Result<u32> create_file(u32 parent_inode, std::string_view name);

    Result<u32> lookup(u32 dir_inode, std::string_view name) const;
    Result<u32> resolve_path(std::string_view path) const;

    // syscalls
    // DfsRet<int> creat(std::string_view path);
    int open();
    int close(int fd);
    int read(int fd);
    int write(int fd);

    int lseek();
    int fsync();
    int dup(int fd);
    int mmap();

    // Shell commands
    Result<void> mkdir(std::string_view path);
    Result<void> rmdir(std::string_view path);

    // helpers
    void init_unix_dirs();
    void update_dir_time(u32 parent);
    // used by ls
    Result<std::vector<std::string>> list_dir(std::string_view path) const;

  private:
    // ialloc.cpp
    Result<u32> alloc_inode(InodeType type);
    Result<void> free_inode(u32 inode);

    // balloc.cpp
    Result<u32> alloc_block();
    Result<void> free_block(u32 block);
    // dir.cpp
    Result<void> validate_dir(u32 inode) const;
};

std::unique_ptr<FileSystem> create_unix_fs();

} // namespace dfs
