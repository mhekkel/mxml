cmake_minimum_required(VERSION 3.28)

project(mxml VERSION 1.0.0 LANGUAGES CXX)

if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "This project requires the Ninja generator. Refers to https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html#generator-support")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CTest)
include(GNUInstallDirs)

option(MXML_BUILD_EXAMPLES "Build example executables" ON)
option(MXML_BUILD_DOCUMENTATION "Build documentation" OFF)

find_package(date)

add_library(mxml STATIC)
add_library(mxml::mxml ALIAS mxml)

target_sources(mxml
	PRIVATE
	src/doctype.cxx
	src/document.cxx
	src/html-named-characters.cxx
	src/node.cxx
	src/parser.cxx
	src/text.cxx
	src/xpath.cxx
	# PUBLIC
	# FILE_SET public_modules TYPE HEADERS
	# BASE_DIRS
	# ${CMAKE_CURRENT_SOURCE_DIR}/modules
	# FILES
	# modules/mxml.ixx
	# modules/mxml/doctype.ixx
	# modules/mxml/document.ixx
	# modules/mxml/error.ixx
	# modules/mxml/node.ixx
	# modules/mxml/parser.ixx
	# modules/mxml/serialize.ixx
	# modules/mxml/text.ixx
	# modules/mxml/version.ixx
	# modules/mxml/xpath.ixx


	PUBLIC
	FILE_SET public_modules TYPE CXX_MODULES
	BASE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/modules
	FILES
	modules/mxml.ixx
	modules/mxml/doctype.ixx
	modules/mxml/document.ixx
	modules/mxml/error.ixx
	modules/mxml/node.ixx
	modules/mxml/parser.ixx
	modules/mxml/serialize.ixx
	modules/mxml/text.ixx
	modules/mxml/version.ixx
	modules/mxml/xpath.ixx
)

target_compile_features(mxml PUBLIC cxx_std_20)

if (date_FOUND)
	target_link_libraries(mxml date::date)
endif()

install(TARGETS mxml
	EXPORT mxml
	CXX_MODULES_BMI DESTINATION "lib/cxx/bmi"
	FILE_SET public_modules DESTINATION "lib/cxx/miu")

install(EXPORT mxml
	NAMESPACE mxml::
	DESTINATION "lib/cmake/mxml"
	FILE "mxml-targets.cmake")

if(date_FOUND)
	set(FIND_DEPENDENCIES "find_dependency(date REQUIRED)")
endif()

# file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake"
# 	"
# ")
configure_file(${PROJECT_SOURCE_DIR}/cmake/mxml-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake"
	DESTINATION "lib/cmake/mxml")

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

# add_test(NAME mxml_build
# 	COMMAND
# 	"${CMAKE_COMMAND}"
# 	"-Dexpected_dir=${CMAKE_INSTALL_PREFIX}/lib/cxx/miu"
# 	"-Dmxml_DIR=${CMAKE_INSTALL_PREFIX}/lib/cmake/mxml"
# 	${generator}
# 	-S "${CMAKE_CURRENT_SOURCE_DIR}/test"
# 	-B "${CMAKE_CURRENT_BINARY_DIR}/test")

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

if(MXML_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(MXML_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()
