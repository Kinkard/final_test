cmake_minimum_required(VERSION 2.8)
project(final)

find_package(Boost COMPONENTS system REQUIRED)
if(NOT Boost_FOUND)
	message(SEND_ERROR "Failed to find boost.")
	return()
else()
	include_directories(${Boost_INCLUDE_DIRS})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp reply.cpp request_parser.cpp)

add_executable(final ${SOURCE_FILES})

target_link_libraries(final ${Boost_LIBRARIES} pthread)