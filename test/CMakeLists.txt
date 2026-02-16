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

CPMFindPackage(NAME Catch2 GIT_REPOSITORY "https://github.com/catchorg/Catch2" VERSION "3.4.0" EXCLUDE_FROM_ALL YES)
CPMFindPackage(NAME mcfp GIT_REPOSITORY "https://forge.hekkelman.net/maarten/mcfp.git" GIT_TAG v1.4.2 EXCLUDE_FROM_ALL YES)

# data files for the parser test
set(XML_CONF_TAR ${CMAKE_CURRENT_SOURCE_DIR}/XML-Test-Suite.tbz)
set(XML_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/XML-Test-Suite/xmlconf/xmlconf.xml)

if(NOT EXISTS ${XML_CONF_FILE})
	if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
		find_program(TAR tar)

		if(TAR)
			add_custom_command(OUTPUT ${XML_CONF_FILE}
				COMMAND ${TAR} xf ${XML_CONF_TAR}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib-xml/test/)
		else()
			message(FATAL_ERROR "Please extract the archive ${XML_CONF_TAR} manually or update your cmake version to at least 3.18")
		endif()
	else()
		file(ARCHIVE_EXTRACT INPUT ${XML_CONF_TAR}
			DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})
	endif()
endif()

add_custom_target(XML_CONF ALL DEPENDS ${XML_CONF_FILE})

# add_test(NAME unit-test-1
# COMMAND $<TARGET_FILE:unit-test-1> --data-dir ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND ZEEM_TESTS
	unit
	parser
	serializer
	xpath)

foreach(TEST IN LISTS ZEEM_TESTS)
	set(ZEEM_TEST "${TEST}-test")
	set(ZEEM_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${ZEEM_TEST}.cpp")

	add_executable(${ZEEM_TEST} ${ZEEM_TEST_SOURCE})

	target_link_libraries(${ZEEM_TEST} PRIVATE mcfp::mcfp zeem::zeem Catch2::Catch2)

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(${ZEEM_TEST} PRIVATE /EHsc)
	endif()

	if("${TEST}" STREQUAL "parser")
		# Some tests should be skipped
		list(APPEND ZEEM_TEST_ARGS
			ibm-valid-P28-ibm28v02.xml ibm-valid-P29-ibm29v01.xml ibm-valid-P29-ibm29v02.xml
			ibm-1-1-valid-P03-ibm03v09.xml rmt-e2e-34 rmt-e2e-55 rmt-054 rmt-ns10-006 rmt-e3e-13)
		list(TRANSFORM ZEEM_TEST_ARGS PREPEND "--questionable=")
		list(PREPEND ZEEM_TEST_ARGS ${XML_CONF_FILE} "--print-ids")
		add_dependencies(${ZEEM_TEST} XML_CONF)
	else()
		set(ZEEM_TEST_ARGS "")
	endif()

	add_test(NAME ${ZEEM_TEST}
		COMMAND $<TARGET_FILE:${ZEEM_TEST}> ${ZEEM_TEST_ARGS}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)
endforeach()
