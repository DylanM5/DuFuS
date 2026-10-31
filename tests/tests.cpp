#include "filesystem.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Duplicate directory does not allocate an inode") {
    dfs::FileSystem fs;

    auto first = fs.create_directory("home");
    REQUIRE(first.has_value());

    auto duplicate = fs.create_directory("home");
    REQUIRE_FALSE(duplicate.has_value());

    auto second = fs.create_directory("documents");
    REQUIRE(second.has_value());

    REQUIRE(*second == 2);
}

TEST_CASE("Directories cannot have a file and directory with the same names") {
    dfs::FileSystem fs;

    auto file = fs.create_file("hello.txt");
    REQUIRE(file.has_value());

    auto dir = fs.create_directory("hello.txt");

    REQUIRE_FALSE(dir.has_value());
    REQUIRE(dir.error() == dfs::DfsError::AlreadyExists);
}

TEST_CASE("lookup() with existing files") {
    dfs::FileSystem fs;
    auto file = fs.create_file("hello.txt");
    auto res = fs.lookup(0, "hello.txt");
    REQUIRE(res == 1);
}

TEST_CASE("lookup() without existing files") {
    dfs::FileSystem fs;
    auto res = fs.lookup(0, "hello.txt");
    REQUIRE(res.error() == dfs::DfsError::NotFound);
}

TEST_CASE("resolve_path() that exists") {
    dfs::FileSystem fs;
    auto home = fs.create_directory("home");
    auto res = fs.resolve_path("/home");
    REQUIRE(res == *home);

    auto docs = fs.create_directory(*home, "documents");
    res = fs.resolve_path("/home/documents");
    REQUIRE(res == *docs);
}

TEST_CASE("Path creation with matching names in different directories") {
    dfs::FileSystem fs;

    auto dir = fs.create_directory("home");
    auto hello = fs.create_file("hello.txt");
    auto hi = fs.create_file(*dir, "hello.txt");

    auto res = fs.resolve_path("/home/hello.txt");
    REQUIRE(*res == *hi);
}

TEST_CASE("init unix fs") {
    dfs::FileSystem fs;
    fs.init_unix_dirs();
    auto res = fs.resolve_path("/var/cache");
    REQUIRE(res.has_value());
}

TEST_CASE("Test mkdir") {
    dfs::FileSystem fs;
    fs.mkdir("/var");
    auto res = fs.lookup(0, "var");
    REQUIRE(res.has_value());
    REQUIRE(res == 1);

    fs.mkdir("/var/log");
    res = fs.resolve_path("/var/log");
    REQUIRE(res.has_value());
    REQUIRE(res == 2);
}

TEST_CASE("rmdir removes an empty directory") {
    auto fs = dfs::make_unix_fs();
    REQUIRE(fs->rmdir("/usr/include"));
    REQUIRE(fs->resolve_path("/usr/include").error() == dfs::DfsError::NotFound);
    REQUIRE(fs->resolve_path("/usr/bin"));
}

TEST_CASE("rmdir fails on non-empty and on files") {
    auto fs = dfs::make_unix_fs();
    REQUIRE(fs->rmdir("/usr").error() == dfs::DfsError::NotEmpty);

    REQUIRE(fs->create_file("hi"));
    REQUIRE(fs->rmdir("/hi").error() == dfs::DfsError::IncorrectType);
}
