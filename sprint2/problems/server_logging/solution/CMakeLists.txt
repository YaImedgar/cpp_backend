cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
# Debug message
message(STATUS "${importTargets}") 

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(include)

add_executable(game_server
    src/errors.cpp
    src/logs.cpp
    src/main.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/model.cpp
    src/boost_json.cpp
    src/request_handler.cpp
)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror -Wextra")

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost Threads::Threads)
