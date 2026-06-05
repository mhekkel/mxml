# Copyright (c) 2026 Maarten L. Hekkelman
#
# SPDX-License-Identifier: BSD-2-Clause

CPMFindPackage(NAME Catch2 GIT_REPOSITORY "https://github.com/catchorg/Catch2" VERSION "3.4.0" EXCLUDE_FROM_ALL YES)
CPMFindPackage(NAME mcfp GIT_REPOSITORY "https://forge.hekkelman.net/maarten/mcfp.git" GIT_TAG v2.0.2 EXCLUDE_FROM_ALL YES)

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

function(create_test ZEEM_TEST ZEEM_TEST_SOURCE ZEEM_TEST_ARGS MODULE)
    add_executable(${ZEEM_TEST} ${ZEEM_TEST_SOURCE})

    if(MODULE)
        target_link_libraries(${ZEEM_TEST} PRIVATE mcfp::mcfp-module zeem::zeem-module Catch2::Catch2)
    else()
        target_link_libraries(${ZEEM_TEST} PRIVATE mcfp::mcfp zeem::zeem Catch2::Catch2)
    endif()

    add_dependencies(${ZEEM_TEST} XML_CONF)

    if(MSVC)
        # Specify unwind semantics so that MSVC knowns how to handle exceptions
        target_compile_options(${ZEEM_TEST} PRIVATE /EHsc)
    endif()

    add_test(NAME ${ZEEM_TEST}
        COMMAND $<TARGET_FILE:${ZEEM_TEST}> ${ZEEM_TEST_ARGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)

endfunction()

foreach(TEST IN LISTS ZEEM_TESTS)
    set(ZEEM_TEST "${TEST}-test")
    set(ZEEM_TEST_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${ZEEM_TEST}.cpp")

    if("${TEST}" STREQUAL "parser")
        # Some tests should be skipped
        list(APPEND ZEEM_TEST_ARGS
            ibm-valid-P28-ibm28v02.xml ibm-valid-P29-ibm29v01.xml ibm-valid-P29-ibm29v02.xml
            ibm-1-1-valid-P03-ibm03v09.xml rmt-e2e-34 rmt-e2e-55 rmt-054 rmt-ns10-006 rmt-e3e-13)
        list(TRANSFORM ZEEM_TEST_ARGS PREPEND "--questionable=")
        list(PREPEND ZEEM_TEST_ARGS ${XML_CONF_FILE} "--print-ids")
    else()
        set(ZEEM_TEST_ARGS "")
    endif()

    create_test(${ZEEM_TEST} ${ZEEM_TEST_SOURCE} "${ZEEM_TEST_ARGS}" OFF)

    if(BUILD_CXX_MODULE)
        create_test("${ZEEM_TEST}-m" ${ZEEM_TEST_SOURCE} "${ZEEM_TEST_ARGS}" ON)
    endif()
endforeach()
