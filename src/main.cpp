#include "filesystem.hpp"
#include "shell.hpp"
#include <bitset>
#include <iostream>

int main() {
    dfs::FileSystem fs;

    // std::cout << sizeof(fs) << "\n";
    dfs::start_shell(fs);

    return 0;
}
