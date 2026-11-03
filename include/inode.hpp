#pragma once
#include "types.hpp"

namespace dfs {
inline std::time_t time_now() noexcept { return std::time(nullptr); }

enum class InodeType : u8 { Free, File, Dir };

struct Inode {
    InodeType type{InodeType::Free};
    u32 size{0};         // size of the data in bytes
    std::time_t btime{}; // birth time
    std::time_t atime{}; // access time
    std::time_t mtime{}; // modification time
    std::time_t ctime{}; // metadata change time

    explicit Inode(InodeType t = InodeType::Free) : type{t} {
        if (t != InodeType::Free) {
            atime = mtime = btime = ctime = time_now();
        }
    }
};

} // namespace dfs
