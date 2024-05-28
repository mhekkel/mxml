cmake_minimum_required(VERSION 3.28)

project(mxml VERSION 1.0.3 LANGUAGES CXX)

if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "This project requires the Ninja generator. Refers to https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html#generator-support")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(VersionString)

option(MXML_BUILD_EXAMPLES "Build example executables" ON)
option(MXML_BUILD_DOCUMENTATION "Build documentation" OFF)

if(NOT TARGET date)
	find_package(date QUIET)
endif()

# Version info
write_version_header("${CMAKE_CURRENT_SOURCE_DIR}/src" LIB_NAME "libmxml")

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
	src/revision.hpp
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

if(MSVC)
	target_compile_definitions(mxml PUBLIC NOMINMAX=1)
endif()

if(TARGET date OR date_FOUND)
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

if(BUILD_TESTING)
	add_subdirectory(test)
endif()

if(MXML_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(MXML_BUILD_DOCUMENTATION)
	add_subdirectory(docs)
endif()
