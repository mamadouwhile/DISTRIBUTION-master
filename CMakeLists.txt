cmake_minimum_required(VERSION 3.10)

project(Arene_light)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wpedantic)

find_package(Threads REQUIRED)

add_subdirectory(joueurs)

set(PREBUILT_OBJECTS
        ${CMAKE_CURRENT_SOURCE_DIR}/jeu.o
)

set_source_files_properties(${PREBUILT_OBJECTS} PROPERTIES
        EXTERNAL_OBJECT TRUE
        GENERATED TRUE
)

add_executable(Arene_light
        main.cpp
        arbitre.cpp
        ${PREBUILT_OBJECTS}
)

target_link_libraries(Arene_light
        joueurs
        Threads::Threads
)
