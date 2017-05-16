add_executable(clock clock.c)
target_link_libraries(clock fev)

add_executable(test_loop test_loop.c)
target_link_libraries(test_loop fev)

add_executable(http_server http_server.c)
target_link_libraries(http_server fev)
