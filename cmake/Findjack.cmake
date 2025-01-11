include(FindPackageHandleStandardArgs)

find_path(JACK_INCLUDE_DIR
	NAMES jack/jack.h
	PATHS
		/usr/local/include
		/usr/include
		/usr/include/libusb-1.0
)

find_library(JACK_LIBRARIES
	NAMES jack
	PATHS
		/usr/local/lib
		/usr/lib
		/usr/lib/x86_64-linux-gnu
)

find_package_handle_standard_args(jack DEFAULT_MSG JACK_LIBRARIES)

add_library(Jack::Jack UNKNOWN IMPORTED)
set_target_properties(Jack::Jack PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${JACK_INCLUDE_DIR}")
set_target_properties(Jack::Jack PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${JACK_LIBRARIES}")

if(COMMAND set_package_properties)
	set_package_properties(jack PROPERTIES DESCRIPTION "jack - low latency audio for linux")
	set_package_properties(jack PROPERTIES URL "https://jackaudio.org")
endif()
