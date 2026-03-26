cmake_minimum_required(VERSION 3.10)

project(Arene_light)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wpedantic)

find_package(Threads REQUIRED)

add_subdirectory(joueurs)

add_executable(Arene_light
        main.cpp
        arbitre.cpp
        jeu.cpp
)

target_link_libraries(Arene_light
        joueurs
        Threads::Threads
)
