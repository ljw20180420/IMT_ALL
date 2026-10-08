cmake_minimum_required(VERSION 2.6)
project(IMTDIS)

add_executable(IMTDIS main.cpp)
set(CMAKE_BUILD_TYPE "Debug")
target_compile_options(IMTDIS PRIVATE -O0 -W -Wall -std=c++17)
# set(CMAKE_BUILD_TYPE "Release")
# target_compile_options(IMTDIS PRIVATE -O3 -W -Wall -std=c++17)
target_link_libraries(IMTDIS PRIVATE stdc++fs)

install(TARGETS IMTDIS RUNTIME DESTINATION bin)
