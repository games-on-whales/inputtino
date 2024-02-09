cmake_minimum_required(VERSION 3.13...3.24)

# Project name and a few useful settings. Other commands can pick up the results
project(inputtino
        VERSION 0.1
        DESCRIPTION "A virtual input library"
        LANGUAGES CXX)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    # Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here

    set(CMAKE_CXX_STANDARD 17)

    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Let's nicely support folders in IDEs
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    option(BUILD_SHARED_LIBS "Build libs as shared" ON)

    include(CTest)
endif ()

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif (CCACHE_FOUND)


# FetchContent added in CMake 3.11, downloads during the configure step
include(FetchContent)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

# Make an automatic library - will be static or dynamic based on user setting
add_library(libinputtino)
add_library(inputtino::libinputtino ALIAS libinputtino)

target_include_directories(libinputtino PUBLIC include)
set_target_properties(libinputtino PROPERTIES PUBLIC_HEADER include)
set_target_properties(libinputtino PROPERTIES OUTPUT_NAME "libinputtino")

if (UNIX AND NOT APPLE)
    find_package(PkgConfig)
    pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
    pkg_check_modules(LIBUDEV REQUIRED IMPORTED_TARGET libudev)
    target_link_libraries(libinputtino
            PUBLIC
            PkgConfig::LIBEVDEV
            PkgConfig::LIBUDEV)

    file(GLOB SRC_LIST SRCS src/uinput/*.cpp)
    target_sources(libinputtino PRIVATE ${SRC_LIST} "src/uinput/keyboard.hpp")
    target_include_directories(libinputtino PUBLIC "src/uinput/include")
endif ()

# All users of this library will need at least C++17
target_compile_features(libinputtino PUBLIC cxx_std_17)
set_target_properties(libinputtino PROPERTIES LINKER_LANGUAGE CXX)

# Testing only available if this is the main app
# Emergency override MODERN_CMAKE_BUILD_TESTING provided as well
if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR MODERN_CMAKE_BUILD_TESTING)
        AND BUILD_TESTING)
    add_subdirectory(tests)
endif ()