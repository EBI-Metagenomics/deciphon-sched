cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(
  sched
  VERSION 0.1.0
  LANGUAGES C)

# set(SQLite3_USE_STATIC_LIBS ON) find_package(SQLite3 3.31.1 REQUIRED)
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
  src/stmt.c
  src/to.c
  src/tok.c
  src/xsql.c)

# target_include_directories( ${PROJECT_NAME} PUBLIC
# $<INSTALL_INTERFACE:include>
# $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
# $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

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

# target_link_libraries(${PROJECT_NAME} PUBLIC sqlite3)

add_subdirectory(test)
