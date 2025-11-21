cmake_minimum_required(VERSION 3.18)
project(
  xconn
  VERSION 1.0
  LANGUAGES CXX)

# -------------------------
# Project Settings
# -------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)

# -------------------------
# Formatting & Linting
# -------------------------
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
    COMMENT "Running clang-format on xconn source files")
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
    COMMENT "Running clang-tidy on xconn source files")
endif()

# -------------------------
# Sources & Library
# -------------------------

file(GLOB_RECURSE XCONN_CPP_SOURCES src/*.cpp)
add_library(xconn STATIC ${XCONN_CPP_SOURCES})

# -------------------------
# Dependencies
# -------------------------

find_package(wampproto REQUIRED)

# -------------------------
# Target Configuration
# -------------------------

target_include_directories(
  xconn
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/xconn/internal>)

target_link_libraries(xconn PUBLIC wampproto)

# -------------------------
# Installation
# -------------------------

install(
  TARGETS xconn
  EXPORT xconnTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# -------------------------
# Tests
# -------------------------
option(XCONN_BUILD_TESTS "Build test runner" ON)
if(XCONN_BUILD_TESTS)
  enable_testing()

  add_executable(test_session tests/test_session.cpp)
  target_link_libraries(test_session PRIVATE xconn)
  target_include_directories(test_session PRIVATE include)
  add_test(NAME test_session COMMAND test_session)

  add_executable(test_base_session tests/test_base_session.cpp)
  target_link_libraries(test_base_session PRIVATE xconn wampproto)
  target_include_directories(test_base_session PRIVATE include)
  add_test(NAME test_base_session COMMAND test_base_session)
endif()

# -------------------------
# CMake Package Config
# -------------------------
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/xconnConfigVersion.cmake"
  VERSION 0.1.0
  COMPATIBILITY SameMajorVersion)

install(
  EXPORT xconnTargets
  FILE xconnTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xconn)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/xconnConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/xconnConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xconn)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xconnConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/xconnConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xconn)
