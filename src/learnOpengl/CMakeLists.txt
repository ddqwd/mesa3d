cmake_minimum_required(VERSION 2.8.2)
project(ProjectName)

#find_package(library)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILE gcc)
set(CMAKE_CPP_COMPILE g++)


include_directories(
	./include
	./learnopengl
	)

set (INCLUDE_DIR ./include ./learnopengl)
list(APPEND INCLUDE_DIR $ENV{CPP_INCLUDE_PATH})

#add_subdirectory(src)

add_executable(demo ../src/glad.c create_window.cpp)
add_executable(texture ../src/glad.c texture.cpp)

target_compile_options(demo  PRIVATE -g)
target_compile_options(texture  PRIVATE -g)
target_link_libraries(demo -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(texture -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )

