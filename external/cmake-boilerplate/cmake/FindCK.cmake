# FindCK
# ------
#
# Find CK include dirs and libraries
#
# This module reads hints about search locations from variables::
#
#   CK_INCLUDEDIR       - Preferred include directory e.g. <prefix>/include
#   CK_LIBRARYDIR       - Preferred library directory e.g. <prefix>/lib
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``CK::ck``, if
# CK has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables::
#
#   CK_INCLUDE_DIR - Where to find the header files.
#   CK_LIBRARY     - Library when using ck.
#   CK_FOUND       - True if ck library is found.


set(ck_incl_dirs "/usr/include" "/usr/local/include")
if(CK_INCLUDEDIR)
    list(APPEND ck_incl_dirs ${CK_INCLUDEDIR})
endif()

find_path(
    CK_INCLUDE_DIR
    NAMES ck_ring.h
    HINTS ${ck_incl_dirs}
)

set(ck_lib_dirs "/usr/lib" "/usr/local/lib")
if(CK_LIBRARYDIR)
    list(APPEND ck_lib_dirs ${CK_LIBRARYDIR})
endif()

find_library(
    CK_LIBRARY
    NAMES ck cklib libck
    HINTS ${ck_lib_dirs}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CK DEFAULT_MSG
                                  CK_LIBRARY CK_INCLUDE_DIR)

mark_as_advanced(CK_INCLUDE_DIR CK_LIBRARY CK_FOUND)

if(CK_FOUND)
    if(NOT TARGET CK::ck)
        add_library(CK::ck UNKNOWN IMPORTED)
        set_target_properties(CK::ck PROPERTIES
            IMPORTED_LOCATION "${CK_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CK_INCLUDE_DIR}")
    endif()
endif()
