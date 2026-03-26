cmake_minimum_required(VERSION 3.10)

project(joueurs)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wpedantic)

find_package(Threads REQUIRED)

add_library(joueurs
        joueur.cpp
        joueur_random.cpp
        joueur_brutal.cpp
        joueur_MCTS.cpp
)

target_link_libraries(joueurs
        Threads::Threads
)
