cmake_minimum_required(VERSION 3.7)

project(invaders_emulation)

#find_package(SDL2 REQUIRED)
add_executable(invaders_emulation main.c inv_machine.c intel8080_emulator.c)

INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
include_directories(invaders_emulation ${SDL2_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR})
target_link_libraries(invaders_emulation ${SDL2_LIBRARIES})

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")




