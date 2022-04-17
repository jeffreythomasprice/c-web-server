project(main)

file(GLOB_RECURSE sources *.c)
add_executable(experiment ${sources})
target_link_libraries(experiment shared pthread)