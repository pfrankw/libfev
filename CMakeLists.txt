cmake_minimum_required(VERSION 2.8)

project("libfev" C)

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
SET(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall")
SET(CMAKE_C_FLAGS_DEBUG  "${CMAKE_C_FLAGS_DEBUG}")
SET(CMAKE_C_FLAGS_RELEASE  "${CMAKE_C_FLAGS_RELEASE}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RELEASE")
endif()

message("Current CMake build type is ${CMAKE_BUILD_TYPE}")

include_directories("include" "${CMAKE_SOURCE_DIR}/ext/libflist/include")

add_subdirectory(src)
add_subdirectory(programs)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/fev" DESTINATION include)
