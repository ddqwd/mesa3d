


# Prevent mixed compile with GCC and Clang
if (NOT (CMAKE_C_COMPILER_ID MATCHES "GNU") EQUAL (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
	message(FATAL_ERROR "CMake C and CXX compilers do not match. Both or neither must be GNU.")
elseif (NOT (CMAKE_C_COMPILER_ID MATCHES "Clang") EQUAL (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
	message(FATAL_ERROR "CMake C and CXX compilers do not match. Both or neither must be Clang.")
endif ()

# Compiler detection
if (NOT DEFINED DE_COMPILER)
	if ((CMAKE_C_COMPILER_ID MATCHES "MSVC") OR MSVC)
		set(DE_COMPILER "DE_COMPILER_MSC")
	elseif (CMAKE_C_COMPILER_ID MATCHES "GNU")
		set(DE_COMPILER "DE_COMPILER_GCC")
	elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
		set(DE_COMPILER "DE_COMPILER_CLANG")

	# Guess based on OS
	elseif (DE_OS_IS_WIN32)
		set(DE_COMPILER "DE_COMPILER_MSC")
	elseif (DE_OS_IS_UNIX OR DE_OS_IS_ANDROID)
		set(DE_COMPILER "DE_COMPILER_GCC")
	elseif (DE_OS_IS_OSX OR DE_OS_IS_IOS)
		set(DE_COMPILER "DE_COMPILER_CLANG")

	else ()
		set(DE_COMPILER "DE_COMPILER_VANILLA")
	endif ()
endif ()



# Pointer size detection
if (NOT DEFINED DE_PTR_SIZE)
	if (DEFINED CMAKE_SIZEOF_VOID_P)
		set(DE_PTR_SIZE ${CMAKE_SIZEOF_VOID_P})
	else ()
		set(DE_PTR_SIZE 4)
	endif ()
endif ()

# CPU detection
if (NOT DEFINED DE_CPU)
	if (DE_PTR_SIZE EQUAL 8)
		set(DE_CPU "DE_CPU_X86_64")
	else ()
		set(DE_CPU "DE_CPU_X86")
	endif ()
endif ()



# Debug definitions
if (NOT DEFINED DE_DEBUG)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithAsserts")
		set(DE_DEBUG 1)
	else ()
		set(DE_DEBUG 0)
	endif ()
endif ()


# Debug definitions
if (NOT DEFINED DE_DEBUG)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithAsserts")
		set(DE_DEBUG 1)
	else ()
		set(DE_DEBUG 0)
	endif ()
endif ()


