project(shared_test)

add_executable(test_base64 base64.c)
target_link_libraries(test_base64 shared)
add_test(NAME test_base64 COMMAND test_base64)
