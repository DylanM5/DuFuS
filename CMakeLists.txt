
cmake_minimum_required(VERSION 3.22)

project(file_system LANGUAGES CXX)

# --------------------------------------------------
# C++ standard
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Options
# --------------------------------------------------

option(ENABLE_WERROR "Treat warnings as errors" OFF)
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
option(USE_CCACHE "Use ccache if available" ON)

# --------------------------------------------------
# ccache
# --------------------------------------------------

if(USE_CCACHE)
    find_program(CCACHE_EXECUTABLE ccache)

    if(CCACHE_EXECUTABLE)
        set(CMAKE_CXX_COMPILER_LAUNCHER
            "${CCACHE_EXECUTABLE}"
        )
        message(STATUS "Using ccache: ${CCACHE_EXECUTABLE}")
    endif()
endif()

# --------------------------------------------------
# Compiler options
# --------------------------------------------------

function(configure_target target)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")

        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )

        if(ENABLE_WERROR)
            target_compile_options(${target} PRIVATE
                -Werror
            )
        endif()

        if(ENABLE_SANITIZERS)

            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
                -g
            )

            get_target_property(TARGET_TYPE ${target} TYPE)

            if(NOT TARGET_TYPE STREQUAL "STATIC_LIBRARY")
                target_link_options(${target} PRIVATE
                    -fsanitize=address,undefined
                )
            endif()

        endif()
    endif()

endfunction()

# --------------------------------------------------
# File system library
# --------------------------------------------------

add_library(file_system_core STATIC
    src/filesystem.cpp src/shell.cpp
)

target_include_directories(file_system_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

configure_target(file_system_core)

# --------------------------------------------------
# Main application
# --------------------------------------------------

add_executable(main
    src/main.cpp
)

target_link_libraries(main PRIVATE
    file_system_core
)

configure_target(main)

# --------------------------------------------------
# Testing
# --------------------------------------------------

include(CTest)

if(BUILD_TESTING)

    include(FetchContent)

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.16.1
    )

    FetchContent_MakeAvailable(Catch2)

    add_executable(tests
        tests/tests.cpp
    )

    target_link_libraries(tests PRIVATE
        file_system_core
        Catch2::Catch2WithMain
    )

    configure_target(tests)

    list(APPEND CMAKE_MODULE_PATH
        "${catch2_SOURCE_DIR}/extras"
    )

    include(Catch)
    catch_discover_tests(tests)

endif()
