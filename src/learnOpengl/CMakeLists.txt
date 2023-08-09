cmake_minimum_required(VERSION 2.8.2)
project(ProjectName)

#find_package(library)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILE gcc)
set(CMAKE_CPP_COMPILE g++)


include_directories(
	./include
	)

#add_subdirectory(src)

add_executable(demo ../src/glad.c create_window.cpp)

target_compile_options(demo  PRIVATE -g)
target_link_libraries(demo -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )

