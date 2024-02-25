cmake_minimum_required(VERSION 3.28)

project(mxml VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(mxml STATIC)
add_library(mxml::mxml ALIAS mxml)

target_sources(mxml
	PRIVATE
	# FILE_SET private_modules TYPE CXX_MODULES
	# BASE_DIRS
	# ${CMAKE_CURRENT_SOURCE_DIR}/src
	# FILES
	src/node.cxx
	PUBLIC
	FILE_SET public_modules TYPE CXX_MODULES
	BASE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/modules
	FILES
	modules/mxml.ixx
	modules/node.ixx
	modules/error.ixx
)

target_compile_features(mxml PUBLIC cxx_std_20)

install(TARGETS mxml
	EXPORT CXXModules
	FILE_SET public_modules DESTINATION "lib/cxx/miu")

install(EXPORT CXXModules
	NAMESPACE CXXModules::
	DESTINATION "lib/cmake/mxml"
	FILE "mxml-targets.cmake")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mxml-config.cmake"
	"include(\"\${CMAKE_CURRENT_LIST_DIR}/mxml-targets.cmake\")
set(\${CMAKE_FIND_PACKAGE_NAME}_FOUND 1)
")

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

add_subdirectory(test)