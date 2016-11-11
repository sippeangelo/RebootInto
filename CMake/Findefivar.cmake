# efivar_FOUND
# efivar_INCLUDE_DIRS

find_path(efivar_INCLUDE_DIR efivar.h
    /usr/include/efivar
    /usr/local/include/efivar
)
set(efivar_INCLUDE_DIRS ${efivar_INCLUDE_DIR})

find_library(efivar_LIBRARIES libefivar.so
    /usr/lib
    /usr/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(efivar DEFAULT_MSG efivar_INCLUDE_DIR efivar_LIBRARIES)
mark_as_advanced(efivar_FOUND efivar_INCLUDE_DIR)