cmake_minimum_required(VERSION 2.8)
project(numurmon C)

if(NOT UMURMUR_ROOT_PATH)
  if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../umurmur/CMakeLists.txt")
    set(UMURMUR_ROOT_PATH "${CMAKE_CURRENT_LIST_DIR}/../umurmur")
  elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../CMakeLists.txt")
    set(UMURMUR_ROOT_PATH "${CMAKE_CURRENT_LIST_DIR}/../..")
  else()
    message(FATAL_ERROR "Could not determine the umurmur source path. Please specify UMURMUR_ROOT_PATH")
  endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${UMURMUR_ROOT_PATH}/cmake/Modules")

include(CheckLibraryExists)
include(CheckFunctionExists)
include(SelectRTBackend)

if(UMURMUR_BINARY_DIR)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UMURMUR_BINARY_DIR})
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

find_package(Curses REQUIRED)

include_directories("${UMURMUR_ROOT_PATH}/src" ${CURSES_INCLUDE_DIR})
add_subdirectory(src)
