# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/shiji/mesa3d/mesa3d/src/learnOpengl

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/shiji/mesa3d/mesa3d/src/learnOpengl/build

# Include any dependencies generated for this target.
include CMakeFiles/basic_light_specular.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/basic_light_specular.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/basic_light_specular.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/basic_light_specular.dir/flags.make

CMakeFiles/basic_light_specular.dir/src/glad.c.o: CMakeFiles/basic_light_specular.dir/flags.make
CMakeFiles/basic_light_specular.dir/src/glad.c.o: ../src/glad.c
CMakeFiles/basic_light_specular.dir/src/glad.c.o: CMakeFiles/basic_light_specular.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/shiji/mesa3d/mesa3d/src/learnOpengl/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/basic_light_specular.dir/src/glad.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/basic_light_specular.dir/src/glad.c.o -MF CMakeFiles/basic_light_specular.dir/src/glad.c.o.d -o CMakeFiles/basic_light_specular.dir/src/glad.c.o -c /home/shiji/mesa3d/mesa3d/src/learnOpengl/src/glad.c

CMakeFiles/basic_light_specular.dir/src/glad.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/basic_light_specular.dir/src/glad.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/shiji/mesa3d/mesa3d/src/learnOpengl/src/glad.c > CMakeFiles/basic_light_specular.dir/src/glad.c.i

CMakeFiles/basic_light_specular.dir/src/glad.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/basic_light_specular.dir/src/glad.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/shiji/mesa3d/mesa3d/src/learnOpengl/src/glad.c -o CMakeFiles/basic_light_specular.dir/src/glad.c.s

CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o: CMakeFiles/basic_light_specular.dir/flags.make
CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o: ../basic_light_specular.cpp
CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o: CMakeFiles/basic_light_specular.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/shiji/mesa3d/mesa3d/src/learnOpengl/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o -MF CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o.d -o CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o -c /home/shiji/mesa3d/mesa3d/src/learnOpengl/basic_light_specular.cpp

CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/shiji/mesa3d/mesa3d/src/learnOpengl/basic_light_specular.cpp > CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.i

CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/shiji/mesa3d/mesa3d/src/learnOpengl/basic_light_specular.cpp -o CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.s

# Object files for target basic_light_specular
basic_light_specular_OBJECTS = \
"CMakeFiles/basic_light_specular.dir/src/glad.c.o" \
"CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o"

# External object files for target basic_light_specular
basic_light_specular_EXTERNAL_OBJECTS =

basic_light_specular: CMakeFiles/basic_light_specular.dir/src/glad.c.o
basic_light_specular: CMakeFiles/basic_light_specular.dir/basic_light_specular.cpp.o
basic_light_specular: CMakeFiles/basic_light_specular.dir/build.make
basic_light_specular: CMakeFiles/basic_light_specular.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/shiji/mesa3d/mesa3d/src/learnOpengl/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable basic_light_specular"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/basic_light_specular.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/basic_light_specular.dir/build: basic_light_specular
.PHONY : CMakeFiles/basic_light_specular.dir/build

CMakeFiles/basic_light_specular.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/basic_light_specular.dir/cmake_clean.cmake
.PHONY : CMakeFiles/basic_light_specular.dir/clean

CMakeFiles/basic_light_specular.dir/depend:
	cd /home/shiji/mesa3d/mesa3d/src/learnOpengl/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/shiji/mesa3d/mesa3d/src/learnOpengl /home/shiji/mesa3d/mesa3d/src/learnOpengl /home/shiji/mesa3d/mesa3d/src/learnOpengl/build /home/shiji/mesa3d/mesa3d/src/learnOpengl/build /home/shiji/mesa3d/mesa3d/src/learnOpengl/build/CMakeFiles/basic_light_specular.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/basic_light_specular.dir/depend
