# Copyright (c) 2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.28)

# Experimental import std support
# if(${CMAKE_VERSION} VERSION_EQUAL 4.3.3)
#     set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")

#     set(CMAKE_CXX_STANDARD_REQUIRED OFF)
#     set(CMAKE_CXX_MODULE_STD 1)
# endif()

project(zeem VERSION 2.4.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CPM)
include(CMakePackageConfigHelpers)
include(CTest)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)
include(VersionString)

option(ZEEM_BUILD_DOCUMENTATION "Build documentation" OFF)
option(ZEEM_BUILD_EXAMPLES "Build example executables" ON)
option(ZEEM_BUILD_CXX_MODULE "Build the library as a C++ 20 module library" ON)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if(ZEEM_BUILD_CXX_MODULE)
        message(WARNING "Sorry, macOS has no proper support for module libraries at the moment")
        set(ZEEM_BUILD_CXX_MODULE OFF)
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if(ZEEM_BUILD_CXX_MODULE AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15)
        message(WARNING "Sorry, your compiler is too old, not building the module library")
        set(ZEEM_BUILD_CXX_MODULE OFF)
    endif()
    add_compile_options(-Wall -Wextra -Wno-bidi-chars)
elseif(MSVC)
    add_compile_options(/W4)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    if(ZEEM_BUILD_CXX_MODULE AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 20)
        message(WARNING "Sorry, your compiler is too old, not building the module library")
        set(ZEEM_BUILD_CXX_MODULE OFF)
    endif()
endif()

try_compile(STD_CHARCONV_COMPILING
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-charconv.cpp)

if(NOT STD_CHARCONV_COMPILING)
    message(NOTICE "zeem: Using fast_float for std::from_chars")
    CPMFindPackage(
        NAME FastFloat
        GIT_REPOSITORY "https://github.com/fastfloat/fast_float"
        GIT_TAG v8.0.2
        EXCLUDE_FROM_ALL YES)
endif()

try_run(TEST_STD_CHRONO_FROM_STREAM_R TEST_STD_CHRONO_FROM_STREAM_C
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-chrono_from_stream.cpp
    CXX_STANDARD 23)

if("${TEST_STD_CHRONO_FROM_STREAM_R}" EQUAL "0")
    set(USE_STD_CHRONO_FROM_STREAM TRUE)
else()
    set(USE_STD_CHRONO_FROM_STREAM FALSE)
endif()

if(NOT USE_STD_CHRONO_FROM_STREAM)
    message(NOTICE "zeem: Using Howard Hinnants date for from_stream")
    CPMFindPackage(
        NAME date
        GIT_REPOSITORY "https://github.com/HowardHinnant/date"
        VERSION 3.0.3
        EXCLUDE_FROM_ALL YES
        OPTIONS
        "USE_SYSTEM_TZ_DB ON"
        "BUILD_TZ_LIB ON")
    set(ZEEM_USE_DATE_H ON)
endif()

# Config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/zeem/config.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/zeem/config.hpp"
    @ONLY)

# Version info
write_version_header("${CMAKE_CURRENT_SOURCE_DIR}/src" LIB_NAME "libzeem")

# Library rules
file(GLOB_RECURSE zeem_headers include/*.hpp)
file(GLOB_RECURSE zeem_modules include/*.cppm)

file(GLOB_RECURSE zeem_cxx src/*.cxx)
file(GLOB_RECURSE zeem_cpp src/*.cpp)

if(STD_CHARCONV_COMPILING)
    list(FILTER zeem_cpp EXCLUDE REGEX src/charconv-fast_float\.cpp)
    list(FILTER zeem_cxx EXCLUDE REGEX src/charconv-fast_float\.cxx)
endif()

add_library(zeem ${zeem_cpp})
add_library(zeem::zeem ALIAS zeem)

target_compile_features(zeem PUBLIC cxx_std_23)

if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(zeem PRIVATE ZEEM_SHARED_BUILD)
    set_target_properties(zeem PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(NOT STD_CHARCONV_COMPILING)
    target_include_directories(zeem PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(zeem PRIVATE USE_FAST_FLOAT)
endif()

if(NOT USE_STD_CHRONO_FROM_STREAM)
    target_link_libraries(zeem PUBLIC date::date-tz)
    set(FIND_DEPENDENCIES "find_dependency(date REQUIRED)")
endif()

# The classic target
target_sources(zeem
    PUBLIC
    FILE_SET zeem_classic TYPE HEADERS
    BASE_DIRS include
    FILES ${zeem_headers})

set_target_properties(zeem PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(zeem PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    INTERFACE_zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
    COMPATIBLE_INTERFACE_STRING zeem_MAJOR_VERSION)

install(TARGETS zeem
    EXPORT zeem-targets
    FILE_SET zeem_classic
    DESTINATION include)

install(EXPORT zeem-targets
    NAMESPACE zeem::
    DESTINATION lib/cmake/zeem)

# Remove old header file
file(GLOB OLD_HEADER ${CMAKE_INSTALL_FULL_INCLUDEDIR}/zeem.hpp)
if(OLD_HEADER)
    message(NOTICE "zeem: Installation will remove old header file: ${OLD_HEADER}")
    install(CODE "file(REMOVE ${OLD_HEADER})")
endif()

if(ZEEM_BUILD_CXX_MODULE)
    if(BUILD_SHARED_LIBS)
        add_library(zeem-module SHARED ${zeem_cxx})

        if(WIN32)
            target_compile_definitions(zeem-module PRIVATE ZEEM_SHARED_BUILD)
            set_target_properties(zeem-module PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
        endif()
    else()
        add_library(zeem-module STATIC ${zeem_cxx})
    endif()
    add_library(zeem::zeem-module ALIAS zeem-module)

    target_compile_features(zeem-module PUBLIC cxx_std_23)

    if(NOT STD_CHARCONV_COMPILING)
		target_include_directories(zeem-module PRIVATE $<TARGET_PROPERTY:FastFloat::fast_float,INTERFACE_INCLUDE_DIRECTORIES>)
    endif()

    target_sources(zeem-module
        PUBLIC
        FILE_SET zeem_module TYPE CXX_MODULES
        BASE_DIRS include
        FILES ${zeem_modules})

    target_compile_definitions(zeem-module PUBLIC ZEEM_CXX_MODULE)

    set_target_properties(zeem-module PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        INTERFACE_zeem_MAJOR_VERSION ${PROJECT_VERSION_MAJOR}
        COMPATIBLE_INTERFACE_STRING zeem_MAJOR_VERSION)

    target_include_directories(zeem-module PUBLIC
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/zeem>)

    install(TARGETS zeem-module
        EXPORT zeem-module-targets
        CXX_MODULES_BMI DESTINATION "lib/cxx/bmi"
        FILE_SET zeem_module DESTINATION "lib/cxx/miu")

    install(EXPORT zeem-module-targets
        NAMESPACE zeem::
        DESTINATION lib/cmake/zeem
        FILE "zeem-module-targets.cmake")
endif()

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/zeem-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/zeem-config.cmake
    INSTALL_DESTINATION lib/cmake/zeem)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/zeem-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/zeem-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/zeem-config-version.cmake"
    DESTINATION lib/cmake/zeem)

set(generator -G "${CMAKE_GENERATOR}")

if(CMAKE_GENERATOR_TOOLSET)
    list(APPEND generator -T "${CMAKE_GENERATOR_TOOLSET}")
endif()

if(CMAKE_GENERATOR_PLATFORM)
    list(APPEND generator -A "${CMAKE_GENERATOR_PLATFORM}")
endif()

if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(test)
endif()

if(ZEEM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ZEEM_BUILD_DOCUMENTATION)
    add_subdirectory(docs)
endif()
