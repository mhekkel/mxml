add_executable(unit-test-1 ${CMAKE_CURRENT_SOURCE_DIR}/unit-test-1.cpp)

find_package(Catch2 REQUIRED)

target_link_libraries(unit-test-1 mxml::mxml Catch2::Catch2)

add_test(NAME unit-test-1
	COMMAND $<TARGET_FILE:unit-test-1> --data-dir ${CMAKE_CURRENT_SOURCE_DIR})

if(${Catch2_VERSION} VERSION_GREATER_EQUAL 3.0.0)
	target_compile_definitions(unit-test-1 PUBLIC CATCH22=0)
else()
	target_compile_definitions(unit-test-1 PUBLIC CATCH22=1)
endif()
