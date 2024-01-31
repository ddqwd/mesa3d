cmake_minimum_required(VERSION 2.8.2)
project(ProjectName)

#find_package(library)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_COMPILE gcc)
set(CMAKE_CPP_COMPILE g++)


include_directories(
	./include
	./learnopengl
	./include/glm
	)

set (INCLUDE_DIR ./include ./learnopengl)
list(APPEND INCLUDE_DIR $ENV{CPP_INCLUDE_PATH})

#add_subdirectory(src)

add_executable(create_window ../src/glad.c create_window.cpp)
add_executable(texture ../src/glad.c texture.cpp)
add_executable(texture_comb ../src/glad.c textures_combined.cpp)
add_executable(colors ../src/glad.c colors.cpp)
add_executable(basic_light_specular ../src/glad.c basic_light_specular.cpp)
#add_executable(framebuffers ../src/glad.c framebuffers.cpp)
add_executable(advanced_glsl_ubo ../src/glad.c advanced_glsl_ubo.cpp)
add_executable(geometry_shader_houses ../src/glad.c geometry_shader_houses.cpp)
add_executable(shaders_interpolation ../src/glad.c shaders_interpolation.cpp)

target_compile_options(create_window  PRIVATE -g)
target_compile_options(texture  PRIVATE -g)
target_link_libraries(create_window -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(texture -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(texture_comb -g -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(colors -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(basic_light_specular -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
#target_link_libraries(framebuffers -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(advanced_glsl_ubo -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(geometry_shader_houses  -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )
target_link_libraries(shaders_interpolation -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl )


# chatgpt create_window

add_executable(gpt_transform_feedback  gpt_transform_feedback.cpp)


target_link_libraries(gpt_transform_feedback -lglfw -lGL -lGLEW  -lX11 -lpthread -lXrandr -lXi -ldl )



