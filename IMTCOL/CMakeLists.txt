cmake_minimum_required(VERSION 2.6)
project(IMTCOL)

add_executable(IMTCOL main.cpp)
set(CMAKE_BUILD_TYPE "Debug")
target_compile_options(IMTCOL PRIVATE -O0 -W -Wall -std=c++17)
# set(CMAKE_BUILD_TYPE "Release")
# target_compile_options(IMTCOL PRIVATE -O3 -W -Wall -std=c++17)
target_link_libraries(IMTCOL PRIVATE stdc++fs)

install(TARGETS IMTCOL RUNTIME DESTINATION bin)
