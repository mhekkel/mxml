# Copyright Maarten L. Hekkelman 2025
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.28)

project(zeem VERSION 2.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakePackageConfigHelpers)
include(CPM)
include(CTest)
include(CheckCXXSourceCompiles)
include(VersionString)

option(ZEEM_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEM_BUILD_EXAMPLES "Build example executables" ON)

# When building with ninja-multiconfig, build both debug and release by default
if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
	set(CMAKE_CROSS_CONFIGS "Debug;Release")
	set(CMAKE_DEFAULT_CONFIGS "Debug;Release")
endif()

# New policy for Boost, set it here
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
	cmake_policy(SET CMP0167 NEW)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wsuggest-override -fmodules-ts")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

set(CPM_USE_LOCAL_PACKAGES ON)

try_compile(STD_CHARCONV_COMPILING
	SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-charconv.cpp)

if(NOT STD_CHARCONV_COMPILING)
	message(NOTICE "zeem: Using fast_float for std::from_chars")
	CPMFindPackage(
		NAME fast_float
		GIT_REPOSITORY "https://github.com/fastfloat/fast_float"
		GIT_TAG v8.0.2
		EXCLUDE_FROM_ALL YES)
endif()

# Version info
write_version_header("${CMAKE_CURRENT_SOURCE_DIR}/src" LIB_NAME "libzeem")

file(GLOB_RECURSE zeem_cpp CONFIGURE_DEPENDS src/*.cpp)

if(STD_CHARCONV_COMPILING)
	list(FILTER zeem_cpp EXCLUDE REGEX src/charconv-fast_float\.cpp)
endif()

file(GLOB_RECURSE zeem_cppm src/*.cppm)

add_library(zeem STATIC ${zeem_cpp})
add_library(zeem::zeem ALIAS zeem)

target_sources(zeem
	PUBLIC
	FILE_SET public_modules TYPE CXX_MODULES
	FILES
	${zeem_cppm})

target_compile_features(zeem PUBLIC cxx_std_23)

if(NOT STD_CHARCONV_COMPILING)
	target_include_directories(zeem PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

target_compile_definitions(zeem PUBLIC $<$<BOOL:MSVC>:NOMINMAX=1>)

set(zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(zeem PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	INTERFACE_zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
	COMPATIBLE_INTERFACE_STRING zeem_MAJOR_VERSION
)

install(TARGETS zeem
	EXPORT zeem
	CXX_MODULES_BMI DESTINATION "lib/cxx/bmi"
	FILE_SET public_modules DESTINATION "lib/cxx/miu")

install(EXPORT zeem
	NAMESPACE zeem::
	DESTINATION lib/cmake/zeem
	FILE zeem-targets.cmake)

configure_package_config_file(
	${PROJECT_SOURCE_DIR}/cmake/zeem-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/zeem-config.cmake
	INSTALL_DESTINATION lib/cmake/zeem)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/zeem-config-version.cmake"
	COMPATIBILITY AnyNewerVersion)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/zeem-config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/zeem-config-version.cmake
	DESTINATION lib/cmake/zeem)

set(generator
	-G "${CMAKE_GENERATOR}")

if(CMAKE_GENERATOR_TOOLSET)
	list(APPEND generator
		-T "${CMAKE_GENERATOR_TOOLSET}")
endif()

if(CMAKE_GENERATOR_PLATFORM)
	list(APPEND generator
		-A "${CMAKE_GENERATOR_PLATFORM}")
endif()

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()

if(ZEEM_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(ZEEM_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()
