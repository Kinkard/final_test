cmake_minimum_required(VERSION 2.8)
project(final)

find_package(Boost COMPONENTS system REQUIRED)
if(NOT Boost_FOUND)
	message(SEND_ERROR "Failed to find boost.")
	return()
else()
	include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  set_property(final final PROPERTY COMPILE_OPTIONS "-pthread")
  set_property(final final PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
endif()
if(NOT CMAKE_THREAD_LIBS_INIT)
  return()
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread")

set(SOURCE_FILES main.cpp reply.cpp request_parser.cpp)

add_executable(final ${SOURCE_FILES})

target_link_libraries(final ${Boost_LIBRARIES} ${CMAKE_THREADS_LIBS_INIT})