project(shared_test)

add_executable(test_base64 base64.c)
target_link_libraries(test_base64 shared)
add_test(NAME test_base64 COMMAND test_base64)

add_executable(test_buffer buffer.c)
target_link_libraries(test_buffer shared)
add_test(NAME test_buffer COMMAND test_buffer)

add_executable(test_http http.c)
target_link_libraries(test_http shared)
add_test(NAME test_http COMMAND test_http)

add_executable(test_io io.c)
target_link_libraries(test_io shared)
add_test(NAME test_io COMMAND test_io)

add_executable(test_string string.c)
target_link_libraries(test_string shared)
add_test(NAME test_string COMMAND test_string)

add_executable(test_tcp_socket_wrapper tcp_socket_wrapper.c)
target_link_libraries(test_tcp_socket_wrapper shared)
target_link_libraries(test_tcp_socket_wrapper shared pthread)
add_test(NAME test_tcp_socket_wrapper COMMAND test_tcp_socket_wrapper)

add_executable(test_worker_thread_pool worker_thread_pool.c)
target_link_libraries(test_worker_thread_pool shared pthread)
add_test(NAME test_worker_thread_pool COMMAND test_worker_thread_pool)