cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(
  deciphon_sched
  VERSION 0.0.6
  LANGUAGES C
)

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${CMAKE_CURRENT_LIST_DIR}/external/cmake-boilerplate/cbp.cmake")
cbp_set_rpath()
cbp_include_sanitizers()
cbp_include_code_coverage()
add_code_coverage_all_targets()

find_package(SQLite3 3.35.0 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(xxHash REQUIRED)

find_program(XXD_FOUND "xxd")
if(XXD_FOUND)
  message(STATUS "Looking for xxd - found")
else()
  message(FATAL_ERROR "Looking for xxd - not found")
endif()

add_custom_command(
  OUTPUT schema.c
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
  ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql
  schema
  schema.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
  ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql
  VERBATIM
)

add_library(
  dcp_sched
  schema.c
  src/db.c
  src/job.c
  src/prod.c
  src/safe.c
  src/sched.c
  src/seq.c
  src/seq_queue.c
  src/to.c
  src/tok.c
  src/xfile.c
  src/xsql.c
)
add_library(DCP_SCHED::dcp_sched ALIAS dcp_sched)
target_code_coverage(dcp_sched)

cbp_hide_functions(dcp_sched)
cbp_set_warning_flags()
cbp_ci_build_option()
cbp_assert_null_is_zero_bits()
cbp_generate_export_header(dcp_sched SCHED EXPORT_FILE)

target_include_directories(
  dcp_sched
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(dcp_sched PUBLIC SQLite::SQLite3)
target_link_libraries(dcp_sched PUBLIC xxHash::xxHash)
set_target_properties(dcp_sched PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(dcp_sched PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_features(dcp_sched PRIVATE c_std_11)
target_compile_definitions(dcp_sched PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(dcp_sched PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_definitions(dcp_sched PRIVATE SQLITE_OMIT_LOAD_EXTENSION)
# https://stackoverflow.com/a/42597059
# To make mkstemp available.
target_compile_definitions(dcp_sched PRIVATE _XOPEN_SOURCE=700)
target_compile_options(dcp_sched PRIVATE ${WARNING_FLAGS})
target_compile_options(dcp_sched PRIVATE "-fverbose-asm")
target_link_libraries(dcp_sched INTERFACE $<$<BOOL:${UNIX}>:m>)

set(VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/dcp_sched/version.h)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dcp_sched/version.h.in ${VERSION_FILE}
  @ONLY
)

cbp_install(dcp_sched MERGED)
install(FILES ${EXPORT_FILE} ${CONFIG_FILE} ${VERSION_FILE} DESTINATION include/dcp_sched)

enable_testing()
add_subdirectory(test)
target_code_coverage(sched AUTO ALL)
