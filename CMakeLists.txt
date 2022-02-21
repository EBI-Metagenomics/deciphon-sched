cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(
  sched
  VERSION 0.1.0
  LANGUAGES C)


# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/compiler-options.cmake)
include(cmake/sanitizers.cmake)

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

find_package(Threads REQUIRED)

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
add_library(SCHED::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set(EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/export.h)
include(GenerateExportHeader)
generate_export_header(
  ${PROJECT_NAME}
  BASE_NAME
  SCHED
  INCLUDE_GUARD_NAME
  SCHED_EXPORT_H
  EXPORT_MACRO_NAME
  SCHED_API
  EXPORT_FILE_NAME
  ${EXPORT_FILE})

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

target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
target_link_libraries(${PROJECT_NAME} PUBLIC ${CMAKE_DL_LIBS})
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

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-targets)

install(
  EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE SCHED::
  DESTINATION lib/cmake/${PROJECT_NAME})

install(DIRECTORY include/ DESTINATION include)

install(FILES ${EXPORT_FILE} DESTINATION include/${PROJECT_NAME})

enable_testing()
add_subdirectory(test)
