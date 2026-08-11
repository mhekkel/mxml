# SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.28)

project(zeem-examples LANGUAGES CXX)

# Only try to find package if we're not included from the main zeem CMakeLists.txt
if(NOT TARGET zeem)
	find_package(zeem REQUIRED)
endif()

list(APPEND examples zeem-synopsis zeem-serialize zeem-validating-xml zeem-xpath-sample clavichord-example)

foreach(EXAMPLE IN LISTS examples)
	add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
	target_link_libraries(${EXAMPLE} zeem::zeem)
endforeach()
