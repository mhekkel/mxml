cmake_minimum_required(VERSION 3.28)

project(mxml VERSION 1.0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CTest)
include(GNUInstallDirs)

option(MXML_BUILD_EXAMPLES "Build example executables" ON)
option(MXML_BUILD_DOCUMENTATION "Build documentation" OFF)

find_package(date QUIET)

add_library(mxml STATIC)
add_library(mxml::mxml ALIAS mxml)

target_sources(mxml
	PRIVATE
	src/doctype.cpp
	src/document.cpp
	src/html-named-characters.cpp
	src/node.cpp
	src/parser.cpp
	src/text.cpp
	src/xpath.cpp
	PUBLIC
	FILE_SET public_modules TYPE HEADERS
	BASE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/include
	FILES
	include/mxml.hpp
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

if(MSVC)
	target_compile_definitions(mxml PUBLIC NOMINMAX=1)
endif()

if (date_FOUND)
	target_link_libraries(mxml date::date)
endif()

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

configure_file(${PROJECT_SOURCE_DIR}/cmake/mxml-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake
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
