cmake_minimum_required(VERSION 3.14)

project (ds_ecs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra -Wpedantic -Werror")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Debug build option
option(BUILD_DEBUG "Enable debug tests and utilities" OFF)

# The ECS is header-only; main.cpp is the only translation unit.
set(SOURCE_FILES
    main.cpp
)

find_package(raylib REQUIRED)

add_executable(ds_ecs ${SOURCE_FILES})
target_include_directories(ds_ecs PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ds_ecs raylib)

if(BUILD_DEBUG)
    target_compile_definitions(ds_ecs PRIVATE BUILD_DEBUG)
endif()