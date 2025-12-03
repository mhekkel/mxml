# Copyright Maarten L. Hekkelman 2025
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.23)

project(zeem VERSION 2.0.4 LANGUAGES CXX)

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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wsuggest-override")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

set(CPM_USE_LOCAL_PACKAGES ON)

try_compile(STD_CHARCONV_COMPILING
	SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-charconv.cpp)

if(NOT STD_CHARCONV_COMPILING)
	message(NOTICE "zeem: Using fast_float for std::from_chars")
	CPMFindPackage(
		NAME fastfloat
		GIT_REPOSITORY "https://github.com/fastfloat/fast_float"
		GIT_TAG v8.0.2
		EXCLUDE_FROM_ALL YES)
endif()

try_run(TEST_STD_CHRONO_FROM_STREAM_R TEST_STD_CHRONO_FROM_STREAM_C
	SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-chrono_from_stream.cpp
	CXX_STANDARD 20
)

if ("${TEST_STD_CHRONO_FROM_STREAM_R}" EQUAL "0")
	set(USE_STD_CHRONO_FROM_STREAM TRUE)
else()
	set(USE_STD_CHRONO_FROM_STREAM FALSE)
endif()

message(NOTICE "Output from test: ${USE_STD_CHRONO_FROM_STREAM}")

if(NOT USE_STD_CHRONO_FROM_STREAM)
	message(NOTICE "zeem: Using Howard Hinnants date for from_stream")
	CPMFindPackage(
		NAME date
		GIT_REPOSITORY "https://github.com/HowardHinnant/date"
		VERSION 3.0.3
		EXCLUDE_FROM_ALL YES
		OPTIONS
		"USE_SYSTEM_TZ_DB ON"
		"BUILD_TZ_LIB ON")
	set(ZEEM_USE_DATE_H ON)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/zeem/config.hpp.in
	${CMAKE_CURRENT_SOURCE_DIR}/include/zeem/config.hpp @ONLY)

# Version info
write_version_header("${CMAKE_CURRENT_SOURCE_DIR}/src" LIB_NAME "libzeem")

add_library(zeem)
add_library(zeem::zeem ALIAS zeem)

list(APPEND sources
	src/doctype.cpp
	src/document.cpp
	src/html-named-characters.cpp
	src/node.cpp
	src/parser.cpp
	src/text.cpp
	src/xpath.cpp
	src/revision.hpp
)

if(NOT STD_CHARCONV_COMPILING)
	list(APPEND sources src/charconv-fast_float.cpp)
endif()

target_sources(zeem
	PRIVATE ${sources}
	PUBLIC
	FILE_SET public_modules TYPE HEADERS
	BASE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
	include/zeem.hpp
	include/zeem/config.hpp
	include/zeem/detail/charconv.hpp
	include/zeem/doctype.hpp
	include/zeem/document.hpp
	include/zeem/error.hpp
	include/zeem/node.hpp
	include/zeem/parser.hpp
	include/zeem/serialize.hpp
	include/zeem/text.hpp
	include/zeem/version.hpp
	include/zeem/xpath.hpp
)

target_compile_features(zeem PUBLIC cxx_std_20)

if(NOT STD_CHARCONV_COMPILING)
	target_include_directories(zeem PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

if(NOT USE_STD_CHRONO_FROM_STREAM)
	target_link_libraries(zeem PUBLIC date::date)
	target_compile_definitions(zeem PUBLIC USE_DATE_H=1)
	set(FIND_DEPENDENCIES "find_dependency(date REQUIRED)")
endif()

target_compile_definitions(zeem PUBLIC $<$<BOOL:MSVC>:NOMINMAX=1>)

set(CMAKE_DEBUG_POSTFIX d)
set_target_properties(zeem PROPERTIES DEBUG_POSTFIX "d")

set(zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(zeem PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	INTERFACE_zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
	COMPATIBLE_INTERFACE_STRING zeem_MAJOR_VERSION
)

install(TARGETS zeem
	EXPORT zeem
	FILE_SET public_modules DESTINATION include)

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
	COMPATIBILITY AnyNewerVersion
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/zeem-config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/zeem-config-version.cmake
	DESTINATION lib/cmake/zeem)

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()

if(ZEEM_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(ZEEM_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()
