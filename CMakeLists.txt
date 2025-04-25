# Copyright Maarten L. Hekkelman 2025
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.23)

project(mxml VERSION 2.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(VersionString)
include(CPM)

option(MXML_BUILD_EXAMPLES "Build example executables" ON)
option(MXML_BUILD_DOCUMENTATION "Build documentation" OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wsuggest-override")
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

set(CPM_USE_LOCAL_PACKAGES ON)

CPMAddPackage("gh:HowardHinnant/date@3.0.3")

try_compile(STD_CHARCONV_COMPILING
	SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-charconv.cpp)

if(NOT STD_CHARCONV_COMPILING)
	message(NOTICE "Using fast_float for std::from_chars")
	CPMAddPackage("gh:fastfloat/fast_float@8.0.2")
endif()

# Version info
write_version_header("${CMAKE_CURRENT_SOURCE_DIR}/src" LIB_NAME "libmxml")

add_library(mxml STATIC)
add_library(mxml::mxml ALIAS mxml)

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
	list(APPEND sources
		src/charconv-fast_float.cpp
	)
endif()


target_sources(mxml
	PRIVATE ${sources}
	PUBLIC
	FILE_SET public_modules TYPE HEADERS
	BASE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
	include/mxml.hpp
	include/mxml/detail/charconv.hpp
	include/mxml/doctype.hpp
	include/mxml/document.hpp
	include/mxml/error.hpp
	include/mxml/node.hpp
	include/mxml/parser.hpp
	include/mxml/serialize.hpp
	include/mxml/text.hpp
	include/mxml/version.hpp
	include/mxml/xpath.hpp
)

target_compile_features(mxml PUBLIC cxx_std_20)

target_link_libraries(mxml PRIVATE date::date)
if(NOT STD_CHARCONV_COMPILING)
	target_link_libraries(mxml PRIVATE fast_float)
endif()

target_compile_definitions(mxml PUBLIC $<$<BOOL:MSVC>:NOMINMAX=1>)

install(TARGETS mxml
	EXPORT mxml
	FILE_SET public_modules DESTINATION include)

install(EXPORT mxml
	NAMESPACE mxml::
	DESTINATION lib/cmake/mxml
	FILE mxml-targets.cmake)

if(date_FOUND)
	set(FIND_DEPENDENCIES "find_dependency(date REQUIRED)")
endif()

configure_package_config_file(
	${PROJECT_SOURCE_DIR}/cmake/mxml-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake
	INSTALL_DESTINATION lib/cmake/mxml)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/mxml-config-version.cmake"
	COMPATIBILITY AnyNewerVersion
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/mxml-config-version.cmake
	DESTINATION lib/cmake/mxml)

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

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

if(MXML_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(MXML_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()
