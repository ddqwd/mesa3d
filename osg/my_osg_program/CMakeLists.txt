cmake_minimum_required(VERSION 3.20.0)

project(Name)


find_package(OpenThreads REQUIRED)
find_package(osg REQUIRED)
find_package(osgDB REQUIRED)
find_package(osgUtil REQUIRED)
find_package(osgViewer REQUIRED)

macro(config_project PROJNAME LIBNAME)
	include_directories(${${LIBNAME}}_INLCUDE_DIR)
	target_link_libraries(${PROJNAME} ${${LIBNAME}_LIBRARY)
endmacro()
add_executable(Name main.cpp)
config_project(Name OPENTHREADS)
config_project(Name OSG)
config_project(Name OSGDB)
config_project(Name OSGUTIL)
config_project(Name OSGVIEWER)









