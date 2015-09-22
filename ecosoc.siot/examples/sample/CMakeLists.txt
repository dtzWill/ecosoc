project(sample)
cmake_minimum_required(VERSION 2.8)

#add_executable( Run 
#    RunT.cpp 
#)
#target_link_libraries(Run pthread)

add_executable(mixed_client mixed_client.c)
add_executable(mixed_server mixed_server.c)

execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/mixed.conf . )
execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/run-config.txt . )
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/mixed_server.c -c -o mixed_server.bc)
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/mixed_client.c -c -o mixed_client.bc)
execute_process(COMMAND ${CMAKE_CURRENT_BUILD_DIR}/siot mixed.conf)


add_executable(loop_client loop_client.c)
add_executable(loop_server loop_server.c)

execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/loop.conf . )
execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/run-config.txt . )
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/loop_server.c -c -o loop_server.bc)
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/loop_client.c -c -o loop_client.bc)
execute_process(COMMAND ${CMAKE_CURRENT_BUILD_DIR}/siot loop.conf)


add_executable(sequential_client sequential_client.c)
add_executable(sequential_server sequential_server.c)

execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/sequential.conf . )
execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/run-config.txt . )
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/sequential_server.c -c -o sequential_server.bc)
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/sequential_client.c -c -o sequential_client.bc)
execute_process(COMMAND ${CMAKE_CURRENT_BUILD_DIR}/siot sequential.conf)

add_executable(ifelse_client ifelse_client.c)
add_executable(ifelse_server ifelse_server.c)

execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/ifelse.conf . )
execute_process(COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/run-config.txt . )
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/ifelse_server.c -c -o ifelse_server.bc)
execute_process(COMMAND clang -g -O0 -emit-llvm ${CMAKE_CURRENT_SOURCE_DIR}/ifelse_client.c -c -o ifelse_client.bc)
execute_process(COMMAND ${CMAKE_CURRENT_BUILD_DIR}/siot ifelse.conf)
