cmake_minimum_required(VERSION 3.14)

project (ds_ecs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra -Wpedantic -Werror")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_FILES
    main.cpp

    ecs/idManager.cpp
)

find_package(raylib REQUIRED)

add_executable(ds_ecs ${SOURCE_FILES})
target_include_directories(ds_ecs PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ds_ecs raylib)