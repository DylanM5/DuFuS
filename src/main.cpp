#include "filesystem.hpp"
#include "shell.hpp"
#include "types.hpp"
#include <iostream>

int main() {
    dfs::FileSystem fs;
    dfs::Directory d;

    std::cout << "Size of FileSystem: " << sizeof(fs) << " B\n";
    std::cout << "Size of a Dictionary: " << sizeof(d) << " B\n";

    dfs::start_shell(fs);

    return 0;
}
