cmake_minimum_required(VERSION 3.23)

project(zeem-examples LANGUAGES CXX)

# Only try to find package if we're not included from the main zeem CMakeLists.txt
get_property(PARENT_DIRECTORY DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" PROPERTY PARENT_DIRECTORY)

if("${PARENT_DIRECTORY}" STREQUAL "")
	find_package(zeem REQUIRED)
endif()

list(APPEND examples zeem-synopsis zeem-serialize zeem-validating-xml zeem-xpath-sample clavichord-example)

foreach(EXAMPLE IN LISTS examples)
	add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
	target_link_libraries(${EXAMPLE} zeem::zeem)
endforeach()
