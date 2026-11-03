#include "filesystem.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace dfs;

TEST_CASE("Duplicate directory does not allocate an inode") {
    FileSystem fs;

    auto first = fs.create_dir("home");
    REQUIRE(first.has_value());

    auto duplicate = fs.create_dir("home");
    REQUIRE_FALSE(duplicate.has_value());

    auto second = fs.create_dir("documents");
    REQUIRE(second.has_value());

    REQUIRE(*second == 2);
}

TEST_CASE("Directories cannot have a file and directory with the same names") {
    FileSystem fs;

    auto file = fs.create_file("hello.txt");
    REQUIRE(file.has_value());

    auto dir = fs.create_dir("hello.txt");

    REQUIRE_FALSE(dir.has_value());
    REQUIRE(dir.error() == Error::AlreadyExists);
}

TEST_CASE("lookup() with existing files") {
    FileSystem fs;
    REQUIRE(fs.create_file("hello.txt"));
    auto res = fs.lookup(0, "hello.txt");
    REQUIRE(res == 1);
}

TEST_CASE("lookup() without existing files") {
    FileSystem fs;
    auto res = fs.lookup(0, "hello.txt");
    REQUIRE(res.error() == Error::NotFound);
}

TEST_CASE("OutOfRange lookup()") {
    FileSystem fs;
    REQUIRE(fs.create_file("hello.txt"));
    auto res = fs.lookup(NUM_INODES, "hello.txt");
    REQUIRE(res.error() == Error::OutOfRange);
}

TEST_CASE("resolve_path() that exists") {
    FileSystem fs;
    auto home = fs.create_dir("home");
    auto res = fs.resolve_path("/home");
    REQUIRE(res == *home);

    auto docs = fs.create_dir(*home, "documents");
    res = fs.resolve_path("/home/documents");
    REQUIRE(res == *docs);
}

TEST_CASE("Path creation with matching names in different directories") {
    FileSystem fs;

    auto dir = fs.create_dir("home");
    REQUIRE(dir);
    REQUIRE(fs.create_file("hello.txt"));
    auto hi = fs.create_file(*dir, "hello.txt");

    auto res = fs.resolve_path("/home/hello.txt");
    REQUIRE(*res == *hi);
}

TEST_CASE("init unix fs") {
    FileSystem fs;
    fs.init_unix_dirs();
    auto res = fs.resolve_path("/var/cache");
    REQUIRE(res.has_value());
}

TEST_CASE("Test mkdir") {
    FileSystem fs;
    REQUIRE(fs.mkdir("/var"));
    auto res = fs.lookup(0, "var");
    REQUIRE(res.has_value());
    REQUIRE(res == 1);

    REQUIRE(fs.mkdir("/var/log"));
    res = fs.resolve_path("/var/log");
    REQUIRE(res.has_value());
    REQUIRE(res == 2);
}

TEST_CASE("mkdir with a file as the parent") {
    FileSystem fs;
    REQUIRE(fs.create_file("info"));
    auto res = fs.mkdir("/info/more_info");
    REQUIRE_FALSE(res);
    REQUIRE(res.error() == Error::IncorrectType);
}

TEST_CASE("mkdir: duplicate directories") {
    FileSystem fs;
    REQUIRE(fs.mkdir("/etc"));
    auto res = fs.mkdir("/etc");
    REQUIRE(res.error() == Error::AlreadyExists);
    REQUIRE(fs.rmdir("/etc"));
    REQUIRE(fs.mkdir("/etc"));
    auto t = fs.lookup(0, "etc");
    REQUIRE(*t == 1);
}

TEST_CASE("rmdir removes an empty directory") {
    auto fs = create_unix_fs();
    REQUIRE(fs->rmdir("/usr/include"));
    REQUIRE(fs->resolve_path("/usr/include").error() == Error::NotFound);
    REQUIRE(fs->resolve_path("/usr/bin"));
}

TEST_CASE("rmdir fails on non-empty and on files") {
    auto fs = create_unix_fs();
    REQUIRE(fs->rmdir("/usr").error() == Error::NotEmpty);

    REQUIRE(fs->create_file("hi"));
    REQUIRE(fs->rmdir("/hi").error() == Error::IncorrectType);
}

TEST_CASE("make and remove directory 1000 times") {
    FileSystem fs;
    for (int i{0}; i < 1000; ++i) {
        REQUIRE(fs.mkdir("/etc"));
        REQUIRE(fs.rmdir("/etc"));
    }
}

TEST_CASE("make and remove directory with long name 1000 times") {
    FileSystem fs;
    for (int i{0}; i < 1000; ++i) {
        REQUIRE(fs.mkdir("/reallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyre"
                         "allyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreall"
                         "yreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyre"
                         "allyreallyreallyreallyreallyreallyreallyreallyreallylongname"));
        REQUIRE(fs.rmdir("/reallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyre"
                         "allyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreall"
                         "yreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyre"
                         "allyreallyreallyreallyreallyreallyreallyreallyreallylongname"));
    }
}
