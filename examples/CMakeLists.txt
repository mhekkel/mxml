cmake_minimum_required(VERSION 3.23)

project(mxml-examples LANGUAGES CXX)

# Only try to find package if we're not included from the main mxml CMakeLists.txt
get_property(PARENT_DIRECTORY DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" PROPERTY PARENT_DIRECTORY)

if("${PARENT_DIRECTORY}" STREQUAL "")
	find_package(mxml REQUIRED)
endif()

list(APPEND examples mxml-synopsis mxml-serialize mxml-validating-xml mxml-xpath-sample clavichord-example)

foreach(EXAMPLE IN LISTS examples)
	add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
	target_link_libraries(${EXAMPLE} mxml::mxml)
endforeach()
