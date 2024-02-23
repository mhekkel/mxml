add_executable(unit-test-1 ${CMAKE_CURRENT_SOURCE_DIR}/unit-test-1.cpp)

target_link_libraries(unit-test-1 mxml::mxml)

add_test(NAME unit-test-1
	COMMAND $<TARGET_FILE:unit-test-1> -- ${CMAKE_CURRENT_SOURCE_DIR})
