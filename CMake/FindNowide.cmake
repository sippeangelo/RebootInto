# Nowide_FOUND
# Nowide_INCLUDE_DIRS

find_path(Nowide_INCLUDE_DIR boost/nowide/convert.hpp
  /usr/local/include
  /usr/include
)

set(Nowide_INCLUDE_DIR ${Nowide_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Nowide DEFAULT_MSG Nowide_INCLUDE_DIR)
mark_as_advanced(Nowide_FOUND Nowide_INCLUDE_DIR)