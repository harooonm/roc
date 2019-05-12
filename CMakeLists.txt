cmake_minimum_required(VERSION 2.8.9)
project(roc)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wfatal-errors -D_POSIX_C_SOURCE=200112L -std=c11")

if(CMAKE_BUILD_TYPE EQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Og")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
endif()

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)
include_directories(include)
add_executable(roc src/main.c src/helpers.c src/runner.c src/watcher.c)
target_link_libraries(roc sighandler btree)
