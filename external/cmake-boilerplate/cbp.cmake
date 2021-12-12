set(CBP_DIR "${CMAKE_CURRENT_LIST_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CBP_DIR}/cmake")

function(cbp_install tgt hdr_place)
  install(TARGETS ${tgt} EXPORT ${tgt}-targets)

  if(hdr_place STREQUAL "SEPARATE")
    install(DIRECTORY include/ DESTINATION include)
  elseif(hdr_place STREQUAL "MERGED")
    install(DIRECTORY src/${tgt}/ DESTINATION include/${tgt})
  else()
    message(FATAL_ERROR "Wrong header placement.")
  endif()

  string(TOUPPER ${tgt} TGT)
  set(dst lib/cmake/${tgt})
  install(
    EXPORT ${tgt}-targets
    FILE ${tgt}-targets.cmake
    NAMESPACE ${TGT}::
    DESTINATION ${dst}
  )

  include(CMakePackageConfigHelpers)

  set(cfg ${CMAKE_CURRENT_BINARY_DIR}/${tgt}-config.cmake)
  configure_package_config_file(
    ${tgt}-config.cmake.in ${cfg}
    INSTALL_DESTINATION ${dst}
  )

  set(ver ${CMAKE_CURRENT_BINARY_DIR}/${tgt}-config-version.cmake)
  write_basic_package_version_file(${ver} COMPATIBILITY SameMajorVersion)

  install(FILES ${cfg} ${ver} DESTINATION ${dst})
endfunction()

macro(cbp_include_sanitizers)
  include("${CBP_DIR}/cmake-scripts/sanitizers.cmake")
endmacro()

macro(cbp_include_code_coverage)
  include("${CBP_DIR}/cmake-scripts/code-coverage.cmake")
endmacro()

macro(cbp_set_warning_flags)
  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    # /wd5105:
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5105?view=msvc-160
    set(WARNING_FLAGS /W3 /wd5105)
  else()
    set(
      WARNING_FLAGS
      -Wall
      -Wextra
      -Wstrict-prototypes
      -Wshadow
      -Wconversion
      -Wmissing-prototypes
      -Wno-unused-parameter
      -Wsign-conversion
      -Wno-unused-function
      -Wmissing-field-initializers
    )

    if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
      list(
        APPEND WARNING_FLAGS -Wno-gnu-designator -Wno-empty-translation-unit
        -Wno-gnu-statement-expression -Wno-nullability-extension
        -Wconditional-uninitialized -Wgnu-empty-initializer
      )
    endif()
  endif()
endmacro()

macro(cbp_ci_build_option)
  option(CI_BUILD "CI, extra flags will be set" OFF)
  if(CI_BUILD)
    message(STATUS "CI build enabled")
    if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
      add_compile_options(/WX)
    else()
      add_compile_options(-Werror)
    endif()
  endif()
endmacro()

macro(cbp_set_rpath)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  # Set RPATH only if it's not a system directory
  list(
    FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
    "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir
  )
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  endif()
endmacro()

function(cbp_hide_functions tgt)
  # merge request fix:
  # https://gitlab.kitware.com/cmake/cmake/-/merge_requests/1799
  if(CMAKE_VERSION VERSION_GREATER 3.12.0)
    # Hide functions by default.
    set_property(TARGET ${tgt} PROPERTY C_VISIBILITY_PRESET hidden)
    set_property(TARGET ${tgt} PROPERTY VISIBILITY_INLINES_HIDDEN ON)
  endif()
endfunction()

macro(cbp_generate_export_header tgt base export_file)
  set(${export_file} ${CMAKE_CURRENT_BINARY_DIR}/${tgt}/export.h)
  string(TOUPPER ${tgt} TGT)
  include(GenerateExportHeader)
  generate_export_header(
    ${tgt}
    BASE_NAME
    ${base}
    INCLUDE_GUARD_NAME
    ${TGT}_EXPORT_H
    EXPORT_MACRO_NAME
    ${base}_API
    EXPORT_FILE_NAME
    ${${export_file}}
  )
endmacro()

macro(cbp_assert_null_is_zero_bits)
  include(CheckCSourceRuns)
  check_c_source_runs(
    "
  #include <stddef.h>
  #include <stdint.h>

  /* Returns zero if NULL is zero-bits represented. */
  int main(void)
  {
    void *ptr = NULL;
    intptr_t v0 = (intptr_t)ptr;
    intptr_t v1 = 0;
    return !(v0 == v1);
  }
  "
    NULL_IS_ZERO_BITS
  )

  if(NULL_IS_ZERO_BITS)
    message(STATUS "NULL is zero-bits represented.")
  else()
    message(FATAL_ERROR "NULL is not zero-bits represented.")
  endif()
endmacro()
