# TCLAP_FOUND
# TCLAP_INCLUDE_DIRS

find_path(TCLAP_INCLUDE_DIR tclap/CmdLine.h
  /usr/local/include
  /usr/include
)

set(TCLAP_INCLUDE_DIRS ${TCLAP_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TCLAP DEFAULT_MSG TCLAP_INCLUDE_DIR)
mark_as_advanced(TCLAP_FOUND TCLAP_INCLUDE_DIR)