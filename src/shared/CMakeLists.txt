project(shared)

file(GLOB_RECURSE sources *.c)
add_library(shared ${sources})