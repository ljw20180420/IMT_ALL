cmake_minimum_required(VERSION 2.6)
project(IMTXDO)

add_executable(IMTXDO main.cpp)
set(CMAKE_BUILD_TYPE "Debug")
target_compile_options(IMTXDO PRIVATE -O0 -W -Wall -std=c++17)
# set(CMAKE_BUILD_TYPE "Release")
# target_compile_options(IMTXDO PRIVATE -O3 -W -Wall -std=c++17)
target_link_libraries(IMTXDO PRIVATE stdc++fs xdo)

install(TARGETS IMTXDO RUNTIME DESTINATION bin)