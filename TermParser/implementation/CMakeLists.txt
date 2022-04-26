cmake_minimum_required(VERSION 3.6)
project( ${LIB_NAME} )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library( ${LIB_NAME} SHARED termparser.cpp )

target_include_directories( ${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )
set_target_properties( ${LIB_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME})
set_target_properties( ${LIB_NAME} PROPERTIES LINKER_LANGUAGE CXX )

target_link_libraries( ${LIB_NAME}
)
