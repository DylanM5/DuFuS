# DuFuS: Dylan's File System

File system project I'm making while learning from OSTEP. It's similar to the vsfs from the book. It doesn't use std::filesystem. To start, we will have 64 blocks, each with a size of 4KB. Inodes will be 256 bytes, so 16 inodes per block.

The project structure is inspired by the Linux kernel's ext2 implementation.

## Building and Testing

```bash
cmake -S . -B build \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++-14 \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_SANITIZERS=ON

cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Profiling

### [Heap Profiling](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/6/html/performance_tuning_guide/ch05s03s03)

```bash
cmake -S . -B build-profiling \
    -DCMAKE_CXX_COMPILER=/usr/bin/g++-14 \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DENABLE_SANITIZERS=OFF

cmake --build build-profiling -j

valgrind --tool=massif --stacks=yes ./build-profiling/tests

ms_print massif.out.*
```

## Architecture (not final)

```mermaid
erDiagram
    FileSystem ||--|{ Inode : "stores"
    FileSystem ||--|{ Block : "stores"
    FileSystem ||--o{ Directory : "indexes"
    Inode ||--o| Directory
    Directory ||--o{ DirectoryEntry : "contains"
    Inode ||--o{ DirectoryEntry : "referenced by"

    FileSystem {
        bitset inode_bmap "128 bits"
        bitset data_bmap "64 bits"
        array inode_table "128 Inodes"
        array blocks "64 Blocks"
        unordered_map directories "u32 to Directory"
    }

    Inode {
        InodeType type "Free, File, Dir"
        u32 size "bytes"
        time_t btime
        time_t atime
        time_t mtime
        time_t ctime
    }

    Block {
        byte_array data "4096 bytes"
    }

    Directory {
        unordered_map entries "string to u32"
    }

    DirectoryEntry {
        string name "map key"
        u32 inode_number "map value"
    }

```

## Resources

- https://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf
- https://docs.kernel.org/filesystems/vfs.html
- https://github.com/torvalds/linux/tree/master
- https://github.com/torvalds/linux/blob/master/fs/ext2/ext2.h
- https://en.wikipedia.org/wiki/POSIX
- https://en.wikipedia.org/wiki/File_descriptor
