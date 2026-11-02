#include "shell.hpp"

namespace dfs {
void start_shell(FileSystem &fs) {
    std::string line;

    while (true) {
        std::cout << "> ";

        if (!std::getline(std::cin, line))
            break;

        std::istringstream input(line);
        std::string command;
        input >> command;

        if (command == "exit")
            break;

        else if (command == "mkdir") {
            std::string path;

            if (!(input >> path)) {
                std::cerr << "mkdir: missing arguments\n";
                continue;
            }

            auto res = fs.mkdir(path);

            if (!res)
                std::cerr << "mkdir: error " << static_cast<int>(res.error()) << '\n';
        }

        else if (command == "ls") {
            std::string path;

            if (!(input >> path)) {
                path = "/";
            }

            auto res = fs.list_dir(path);

            if (!res) {
                std::cerr << "ls: error " << static_cast<int>(res.error()) << '\n';
                continue;
            }

            for (const auto &name : *res)
                std::cout << name << "\n";
        }

        else if (command == "init")
            fs.init_unix_dirs();

        else if (command == "help")
            std::cout << "no\n";

        else
            std::cerr << "command '" << command << "' not found. 'help' for options\n";
    }
}

} // namespace dfs
