add_library(input_server)
add_library(inputtino::server ALIAS input_server)


# FetchContent added in CMake 3.11, downloads during the configure step
include(FetchContent)

FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.15.3)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
        immer
        GIT_REPOSITORY https://github.com/arximboldi/immer.git
        GIT_TAG v0.8.1)
set(immer_BUILD_TESTS OFF CACHE INTERNAL "")
set(immer_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(immer_BUILD_DOCS OFF CACHE INTERNAL "")
set(immer_BUILD_EXTRAS OFF CACHE INTERNAL "")

set(FPHSA_NAME_MISMATCHED on) # see: https://github.com/arximboldi/immer/issues/204
FetchContent_MakeAvailable(immer)
unset(FPHSA_NAME_MISMATCHED)

target_link_libraries(input_server
        PUBLIC
        immer
        nlohmann_json::nlohmann_json
        httplib::httplib
        inputtino::libinputtino)

target_sources(input_server PUBLIC
        server/data_model.hpp
        server/json_serialization.hpp
        server/utils.hpp
        server/rest.hpp)

target_include_directories(input_server PUBLIC .)

# All users of this library will need at least C++20
target_compile_features(input_server PUBLIC cxx_std_20)
set_target_properties(input_server PROPERTIES LINKER_LANGUAGE CXX)

####################
# Executable
add_executable(input-server main.cpp)
target_link_libraries(input-server inputtino::server)
target_compile_features(input-server PRIVATE cxx_std_20)
