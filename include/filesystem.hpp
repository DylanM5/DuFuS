#pragma once

#include <array>
#include <bitset>
#include <chrono>  // std::chrono::system_clock::now();
#include <cstdint> // std::uint8_t, std::uint32_t
#include <ctime>   // std::time_t
#include <expected>
#include <memory> // std::unique_ptr
#include <string>
#include <unordered_map>
// #include <utility> // std::pair
#include <vector>

namespace dfs {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using usize = std::size_t;

constexpr usize BLOCK_SIZE = 4096;
constexpr usize NUM_BLOCKS = 64;
constexpr usize NUM_INODES = 128;

enum class DfsError : u8 {
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

template <typename T> using DfsRet = std::expected<T, DfsError>;

enum class InodeType : u8 { Free, File, Dir };

// Metadata of a file
struct Inode {
    InodeType type{InodeType::Free};
    u32 size{0}; // size of the data in bytes
    // block storage
    // permissions
    std::time_t btime{}; // birth time
    std::time_t atime{}; // access time
    std::time_t mtime{}; // modification time

    explicit Inode(InodeType t = InodeType::Free) : type{t} {
        if (t != InodeType::Free) {
            std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

            btime = now;
            atime = now;
            mtime = now;
        }
    }
};

struct Block {
    std::array<std::byte, BLOCK_SIZE> data{};
};

using Directory = std::unordered_map<std::string, u32>; // name, inode number
class FileSystem {
  private:
    std::bitset<NUM_BLOCKS> data_bmap;
    std::bitset<NUM_INODES> inode_bmap;
    std::array<Block, NUM_BLOCKS> blocks;
    std::array<Inode, NUM_INODES> inode_table;
    std::unordered_map<u32, Directory> directories; // map of inodes and their directories

  public:
    FileSystem();
    ~FileSystem() = default;

    FileSystem(const FileSystem &) = delete;
    FileSystem(FileSystem &&) = delete;
    FileSystem &operator=(const FileSystem &) = delete;
    FileSystem &operator=(FileSystem &&) = delete;

    // Create directory in root
    DfsRet<u32> create_directory(const std::string &name);
    DfsRet<u32> create_directory(u32 parent_inode, const std::string &name);

    // Create file in root
    DfsRet<u32> create_file(const std::string &name);
    DfsRet<u32> create_file(u32 parent_inode, const std::string &name);

    DfsRet<u32> lookup(u32 dir_inode, const std::string &name) const;
    DfsRet<u32> resolve_path(const std::string &path) const;

    // syscalls
    // DfsRet<int> creat(const std::string &path);
    int open();
    int close(int fd);
    int read(int fd);
    int write(int fd);

    int lseek();
    int fsync();
    int dup(int fd);
    int mmap();

    // Shell commands
    DfsRet<void> mkdir(const std::string &path);
    DfsRet<void> rmdir(const std::string &path);

    // helpers

    DfsRet<void> init_unix_dirs();
    // used by ls
    DfsRet<std::vector<std::string>> list_dir(const std::string &path) const;

  private:
    DfsRet<u32> alloc_inode(InodeType type);
    DfsRet<void> free_inode(u32 inode);

    DfsRet<u32> alloc_block();
    DfsRet<void> free_block(u32 block);
};

std::unique_ptr<FileSystem> make_unix_fs();

} // namespace dfs
