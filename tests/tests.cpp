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
    fs.initialize_unix_fs();
    auto res = fs.resolve_path("/var/cache");
    REQUIRE(res.has_value());
}
