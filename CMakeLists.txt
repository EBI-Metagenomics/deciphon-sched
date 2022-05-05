cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(
  sched
  VERSION 0.4.0
  LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/compiler-options.cmake)
include(cmake/sanitizers.cmake)
include(cmake/CPM.cmake)

add_custom_command(
  OUTPUT schema.c
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
          ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql schema schema.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/embed-file
          ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.sql
  VERBATIM)

cpmaddpackage(NAME c_toolbelt VERSION 0.2.0 GITHUB_REPOSITORY horta/c-toolbelt)
find_package(Threads REQUIRED)

add_library(
  sched STATIC
  schema.c
  src/db.c
  src/error.c
  src/hmm.c
  src/job.c
  src/prod.c
  src/scan.c
  src/sched.c
  src/sched_health.c
  src/seq.c
  src/seq_queue.c
  src/sqlite3/sqlite3.c
  src/stmt.c
  src/to.c
  src/tok.c
  src/xfile.c
  src/xsql.c)
add_library(SCHED::sched ALIAS sched)
set_property(TARGET sched PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(headers INTERFACE)
add_library(SCHED::headers ALIAS headers)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(sched PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_include_directories(
  sched
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(
  headers INTERFACE $<INSTALL_INTERFACE:include>
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(sched PUBLIC Threads::Threads)
target_link_libraries(sched PUBLIC C_TOOLBELT::c_toolbelt)
target_link_libraries(sched PUBLIC ${CMAKE_DL_LIBS})
set_target_properties(sched PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(sched PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_features(sched PRIVATE c_std_11)

target_compile_definitions(sched PRIVATE XXH_STATIC_LINKING_ONLY)
target_compile_definitions(sched PRIVATE XXH_IMPLEMENTATION)
target_compile_definitions(sched PRIVATE SQLITE_OMIT_COMPILEOPTION_DIAGS)
target_compile_definitions(sched PRIVATE SQLITE_OMIT_DEPRECATED)
target_compile_definitions(sched PRIVATE SQLITE_OMIT_LOAD_EXTENSION)
target_compile_definitions(sched PRIVATE SQLITE_OMIT_EXPLAIN)

install(TARGETS sched EXPORT sched-targets)

install(
  EXPORT sched-targets
  FILE sched-targets.cmake
  NAMESPACE SCHED::
  DESTINATION lib/cmake/sched)

include(CMakePackageConfigHelpers)

set(CMAKE_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/sched-config.cmake)
configure_package_config_file(sched-config.cmake.in ${CMAKE_CONFIG_FILE}
                              INSTALL_DESTINATION lib/cmake/sched)

install(DIRECTORY include/sched DESTINATION include/sched)

install(FILES ${CMAKE_CONFIG_FILE} DESTINATION lib/cmake/sched)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md
              ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md
        DESTINATION share/docs/sched)

enable_testing()
add_subdirectory(test)

set(CPACK_PACKAGE_NAME sched)
set(CPACK_PACKAGE_VENDOR "Danilo Horta")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Deciphon scheduler")
set(CPACK_PACKAGE_CONTACT "Danilo Horta")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/package)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_RELOCATABLE YES)
set(CPACK_MONOLITHIC_INSTALL YES)
include(CPack)
