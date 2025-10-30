cmake_minimum_required(VERSION 3.18)
project(
  xconn_cpp
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE XCONN_CPP_SOURCES src/*.cpp)

# Add the library
add_library(xconn_cpp STATIC ${XCONN_CPP_SOURCES})
# target_include_directories(xconn_cpp PUBLIC
# ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(
  xconn_cpp
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/xconn_cpp/internal)

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
  message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")

  file(
    GLOB_RECURSE
    ALL_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_SOURCE_DIR}/tests/*.hpp)

  add_custom_target(
    xconn_format
    COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format on xconn_cpp source files")
endif()

find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
  set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*,-clang-analyzer-alpha.*")

  file(GLOB_RECURSE ALL_SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp
       ${CMAKE_SOURCE_DIR}/tests/*.cpp)

  add_custom_target(
    xconn_lint
    COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-tidy on xconn_cpp source files")
endif()

include(FetchContent)
FetchContent_Declare(
  wampproto
  GIT_REPOSITORY https://github.com/xconnio/wampproto-c.git
  GIT_TAG tinycbor)

set(WAMMPROTO_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(wampproto)
target_link_libraries(xconn_cpp PRIVATE wampproto)

# Test runner / entrypoint
option(XCONN_BUILD_TESTS "Build test runner" ON)
if(XCONN_BUILD_TESTS)
  enable_testing()

  add_executable(test_session tests/test_session.cpp)
  target_link_libraries(test_session PRIVATE xconn_cpp)
  target_include_directories(test_session PRIVATE include)
  add_test(NAME test_session COMMAND test_session)

  add_executable(test_base_session tests/test_base_session.cpp)
  target_link_libraries(test_base_session PRIVATE xconn_cpp wampproto)
  target_include_directories(test_base_session PRIVATE include)
  add_test(NAME test_base_session COMMAND test_base_session)
endif()
