cmake_minimum_required(VERSION 3.15)

project(experiment)

include(CTest)

add_subdirectory(src/shared)
add_subdirectory(src/shared_test)
add_subdirectory(src/main)