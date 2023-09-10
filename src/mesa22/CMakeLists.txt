cmake_minimum_required(VERSION 3.8)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(draw draw.cpp)

target_link_libraries(draw GL glut)


