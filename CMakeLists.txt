cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(
  sched
  VERSION 0.1.0
  LANGUAGES C)

macro(set_rpath)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  # Set RPATH only if it's not a system directory
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
       "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endmacro()

set_rpath()

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(XXD_FOUND "xxd")
if(XXD_FOUND)
  message(STATUS "Looking for xxd - found")
else()
  message(FATAL_ERROR "Looking for xxd - not found")
endif()

add_custom_command(
  OUTPUT schema.c
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
          ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql schema schema.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
          ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql
  VERBATIM)

add_library(
  ${PROJECT_NAME} STATIC
  schema.c
  src/db.c
  src/job.c
  src/logger.c
  src/prod.c
  src/sched.c
  src/seq.c
  src/seq_queue.c
  src/sqlite3/sqlite3.c
  src/stmt.c
  src/strlcpy.c
  src/to.c
  src/tok.c
  src/xfile.c
  src/xsql.c)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION
                                                   TRUE)
endif()

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION
                                                 ${PROJECT_VERSION_MAJOR})
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)

include(CheckSymbolExists)
check_symbol_exists(strlcpy "string.h" HAVE_STRLCPY)
target_compile_definitions(${PROJECT_NAME}
                           PRIVATE $<$<BOOL:${HAVE_STRLCPY}>:HAVE_STRLCPY>)
target_compile_definitions(${PROJECT_NAME} PRIVATE XXH_STATIC_LINKING_ONLY)
target_compile_definitions(${PROJECT_NAME} PRIVATE XXH_IMPLEMENTATION)

add_subdirectory(test)
