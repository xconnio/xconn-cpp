cmake_minimum_required(VERSION 3.20)
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
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "Force all dependencies to be static" FORCE)
include(GNUInstallDirs)

# -------------------------
# Sources & Library
# -------------------------
file(GLOB_RECURSE XCONN_CPP_SOURCES src/*.cpp)
add_library(xconn STATIC ${XCONN_CPP_SOURCES})

# -------------------------
# Dependencies
# -------------------------
add_subdirectory(vendor/wampproto)

add_library(asio INTERFACE)

target_include_directories(
  asio INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/vendor/asio/include>
                 $<INSTALL_INTERFACE:include>)

target_compile_definitions(asio INTERFACE ASIO_STANDALONE)

# -------------------------
# Target Configuration
# -------------------------
target_include_directories(
  xconn
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/xconn/internal>)

target_link_libraries(xconn PUBLIC wampproto asio)

# -------------------------
# Installation
# -------------------------
install(
  TARGETS xconn asio
  EXPORT xconnTargets # <--- Name of the export set
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT xconnTargets
  FILE xconnTargets.cmake
  NAMESPACE xconn::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xconn)

# -------------------------
# CMake Package Config
# -------------------------

include(CMakePackageConfigHelpers)

# 1. Version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/xconnConfigVersion.cmake"
  VERSION 1.0
  COMPATIBILITY SameMajorVersion)

# 1. Config file (The Glue)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/xconnConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/xconnConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xconn)

# 1. Install Config and Version
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xconnConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/xconnConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xconn)

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
# Formatting and Linting
# -------------------------
if(TARGET xconn_format OR CMAKE_BUILD_TYPE STREQUAL "XFormat")
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
endif()

if(TARGET xconn_lint OR CMAKE_BUILD_TYPE STREQUAL "XLint")
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    set(CMAKE_C_CLANG_TIDY
        "${CLANG_TIDY_EXE};-checks=*,-clang-analyzer-alpha.*")

    file(GLOB_RECURSE ALL_SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp
         ${CMAKE_SOURCE_DIR}/tests/*.cpp)

    add_custom_target(
      xconn_lint
      COMMAND ${CLANG_TIDY_EXE} -p ${CMAKE_BINARY_DIR} ${ALL_SOURCE_FILES}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Running clang-tidy on xconn source files")
  endif()
endif()
