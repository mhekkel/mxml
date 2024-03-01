find_package(Catch2 REQUIRED)
find_package(libmcfp REQUIRED)

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
list(APPEND MXML_TESTS
	unit
	parser
	serializer
	xpath)

foreach(TEST IN LISTS MXML_TESTS)
	set(MXML_TEST "${TEST}-test")
	set(MXML_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${MXML_TEST}.cpp")

	add_executable(${MXML_TEST} ${MXML_TEST_SOURCE})

	target_link_libraries(${MXML_TEST} PRIVATE libmcfp::libmcfp mxml::mxml)

	if(${Catch2_VERSION} VERSION_GREATER_EQUAL 3.0.0)
		target_compile_definitions(${MXML_TEST} PUBLIC CATCH22=0)
	else()
		target_compile_definitions(${MXML_TEST} PUBLIC CATCH22=1)
	endif()

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(${MXML_TEST} PRIVATE /EHsc)
	endif()

	if("${TEST}" STREQUAL "parser")
		# Some tests should be skipped
		list(APPEND MXML_TEST_ARGS
			ibm-valid-P28-ibm28v02.xml ibm-valid-P29-ibm29v01.xml ibm-valid-P29-ibm29v02.xml
			ibm-1-1-valid-P03-ibm03v09.xml rmt-e2e-34 rmt-e2e-55 rmt-054 rmt-ns10-006 rmt-e3e-13)
		list(TRANSFORM MXML_TEST_ARGS PREPEND "--questionable=")
		list(PREPEND MXML_TEST_ARGS ${XML_CONF_FILE} "--print-ids")
		add_dependencies(${MXML_TEST} XML_CONF)
	else()
		set(MXML_TEST_ARGS "")
	endif()

	add_test(NAME ${MXML_TEST}
		COMMAND $<TARGET_FILE:${MXML_TEST}> ${MXML_TEST_ARGS}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)
endforeach()