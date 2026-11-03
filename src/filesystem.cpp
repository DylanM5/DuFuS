#include "filesystem.hpp"

// Initialization

namespace dfs {
void FileSystem::init_unix_dirs() {
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

// Non-member functions

std::unique_ptr<FileSystem> create_unix_fs() {
    auto fs = std::make_unique<FileSystem>();
    fs->init_unix_dirs();
    return fs;
}

} // namespace dfs
