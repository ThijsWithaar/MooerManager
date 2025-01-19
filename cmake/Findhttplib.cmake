include(FindPackageHandleStandardArgs)

find_path(HTTPLIB_INCLUDE_DIR
	NAMES httplib.h
	PATHS /usr/include
		  /usr/local/include
)

find_library(HTTPLIB_LIBRARY
	NAMES cpp-httplib
	PATHS /usr/lib
		  /usr/local/lib
		  /usr/lib/x86_64-linux-gnu
)

find_package_handle_standard_args(httplib DEFAULT_MSG HTTPLIB_INCLUDE_DIR HTTPLIB_LIBRARY)

add_library(httplib::httplib UNKNOWN IMPORTED)
set_target_properties(httplib::httplib PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${HTTPLIB_INCLUDE_DIR}"
	IMPORTED_LOCATION ${HTTPLIB_LIBRARY}
)

mark_as_advanced(httplib_FOUND HTTPLIB_INCLUDE_DIR)

if(COMMAND set_package_properties)
    set_package_properties(cgicc PROPERTIES DESCRIPTION "a C++ class library for http")
    set_package_properties(cgicc PROPERTIES URL "https://github.com/yhirose/cpp-httplib")
endif()
