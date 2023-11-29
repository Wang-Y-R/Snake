cmake_minimum_required(VERSION 3.26)
project(Snake C)

set(CMAKE_C_STANDARD 11)

add_executable(Snake Snake.c)
target_link_libraries(Snake mingw32)
target_link_libraries(Snake SDL2main)
target_link_libraries(Snake SDL2)
target_link_libraries(Snake SDL2_ttf)