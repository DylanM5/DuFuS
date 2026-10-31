# DuFuS: Dylan's File System

File system project I'm making while learning from OSTEP. It's similar to the vsfs from the book.
To start, we will have 64 blocks, each with a size of 4KB. Inodes will be 256 bytes, so 16 inodes per block.

## Building and Testing

```bash
cmake -S . -B build \
 -DCMAKE_BUILD_TYPE=Debug \
 -DENABLE_SANITIZERS=ON

cmake --build build -j

./build/main

ctest --test-dir build --output-on-failure

./build/tests
```

```mermaid
erDiagram
    FileSystem ||--|{ Inode : "stores"
    FileSystem ||--o{ Directory : "indexes"
    Inode ||--o| Directory : "backs"
    Directory ||--o{ DirectoryEntry : "contains"
    Inode ||--o{ DirectoryEntry : "referenced by"

    FileSystem {
        bitset inode_bmap "128 bits"
        bitset data_bmap "64 bits"
        array inode_table "128 Inodes"
        unordered_map directories "u32 to Directory"
    }

    Inode {
        InodeType type "Free, Dir, File"
        u32 size "bytes"
    }

    Directory {
        unordered_map entries "string to u32"
    }

    DirectoryEntry {
        string name "key"
        u32 inode_number "value"
    }
```

## Resources

- https://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf
- https://docs.kernel.org/filesystems/vfs.html
- https://github.com/torvalds/linux/tree/master
- https://en.wikipedia.org/wiki/POSIX
- https://en.wikipedia.org/wiki/File_descriptor
