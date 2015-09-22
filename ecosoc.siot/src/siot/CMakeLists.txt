project(sample)
cmake_minimum_required(VERSION 2.8)

add_executable( siot 
    RunT.cpp 
)
add_executable( siot-abc Instrumentador.cpp )
target_link_libraries(siot pthread)
