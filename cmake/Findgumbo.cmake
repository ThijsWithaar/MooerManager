include(FindPackageHandleStandardArgs)

find_path(GUMBO_INCLUDE_DIR
	NAMES gumbo.h
	PATHS
		/usr/local/include
		/usr/include
)

find_library(GUMBO_LIBRARIES
	NAMES gumbo
	PATHS
		/usr/local/lib
		/usr/lib
		/usr/lib/x86_64-linux-gnu
)

find_package_handle_standard_args(gumbo DEFAULT_MSG GUMBO_LIBRARIES)

add_library(gumbo::gumbo UNKNOWN IMPORTED)
set_target_properties(gumbo::gumbo PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GUMBO_INCLUDE_DIR}")
set_target_properties(gumbo::gumbo PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${GUMBO_LIBRARIES}")

if(COMMAND set_package_properties)
	set_package_properties(gumbo PROPERTIES DESCRIPTION "An HTML5 parsing library in pure C99")
	set_package_properties(gumbo PROPERTIES URL "https://github.com/google/gumbo-parser")
endif()
