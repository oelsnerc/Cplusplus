cmake_minimum_required(VERSION 3.6)
project( ${TEST_NAME} )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()
find_package(GTest REQUIRED)

add_executable( ${TEST_NAME} Test_KeyValueNode.cpp Test_TermParser.cpp)

target_include_directories( ${TEST_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

set_target_properties( ${TEST_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
set_target_properties( ${TEST_NAME} PROPERTIES LINKER_LANGUAGE CXX )

target_link_libraries( ${TEST_NAME}
    PUBLIC ${LIB_NAME}
    gtest gtest_main
)

add_test(AllTestsInFoo foo)
