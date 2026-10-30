#pragma once

#include <array>
#include <bitset>
#include <chrono>  // std::chrono::system_clock::now();
#include <cstdint> // std::uint8_t, std::uint32_t
#include <ctime>   // std::time_t
#include <expected>
#include <string>
#include <unordered_map>

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
    OutOfRange,
    IncorrectType,
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

    int open();
    int close(int fd);
    int read(int fd);
    int write(int fd);

    int lseek();
    int fsync();
    int dup(int fd);
    int mmap();

    // Create directory in root
    DfsRet<u32> create_directory(const std::string &name);
    DfsRet<u32> create_directory(u32 parent_inode, const std::string &name);

    // Create file in root
    DfsRet<u32> create_file(const std::string &name);
    DfsRet<u32> create_file(u32 parent_inode, const std::string &name);

    DfsRet<u32> lookup(u32 dir_inode, const std::string &name) const;
    DfsRet<u32> resolve_path(const std::string &path) const;

    void initialize_unix_fs();

    // POSIX interface
    DfsRet<void> mkdir(const std::string &path);
    DfsRet<int> creat(const std::string &path);

  private:
    DfsRet<u32> create_inode(InodeType type);
    DfsRet<u32> allocate_block();
    DfsRet<void> free_block(u32 block);
};

} // namespace dfs
