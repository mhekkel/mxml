# SPDX-License-Identifier: BSD-2-Clause
# 
# Copyright (c) 2026 Maarten L. Hekkelman
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.23)

project(zeem VERSION 2.1.3 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CMakePackageConfigHelpers)
include(CPM)
include(CTest)
include(CheckCXXSourceCompiles)
include(VersionString)

option(ZEEM_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEM_BUILD_EXAMPLES "Build example executables" ON)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

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

try_run(TEST_STD_CHRONO_FROM_STREAM_R TEST_STD_CHRONO_FROM_STREAM_C
	SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-chrono_from_stream.cpp
	CXX_STANDARD 20)

if ("${TEST_STD_CHRONO_FROM_STREAM_R}" EQUAL "0")
	set(USE_STD_CHRONO_FROM_STREAM TRUE)
else()
	set(USE_STD_CHRONO_FROM_STREAM FALSE)
endif()

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

file(GLOB zeem_src src/*.cpp)
file(GLOB_RECURSE zeem_hdr include/*.hpp)

if(STD_CHARCONV_COMPILING)
	list(FILTER zeem_src EXCLUDE REGEX src/charconv-fast_float.cpp)
endif()

add_library(zeem ${zeem_src})
add_library(zeem::zeem ALIAS zeem)

target_sources(zeem
	PUBLIC
	FILE_SET zeem_headers TYPE HEADERS
	BASE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
	${zeem_hdr})

target_compile_features(zeem PUBLIC cxx_std_20)

if(NOT STD_CHARCONV_COMPILING)
	target_include_directories(zeem PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

if(NOT USE_STD_CHRONO_FROM_STREAM)
	target_link_libraries(zeem PUBLIC date::date-tz)
	set(FIND_DEPENDENCIES "find_dependency(date REQUIRED)")
endif()

set(zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(zeem PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	INTERFACE_zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
	COMPATIBLE_INTERFACE_STRING zeem_MAJOR_VERSION)

install(TARGETS zeem
	EXPORT zeem-targets
	FILE_SET zeem_headers DESTINATION include)

install(EXPORT zeem-targets
	NAMESPACE zeem::
	DESTINATION lib/cmake/zeem)

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

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()

if(ZEEM_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(ZEEM_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()
