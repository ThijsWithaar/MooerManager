include(FindPackageHandleStandardArgs)

find_path(LIBUSB_INCLUDE_DIR
	NAMES libusb.h
	PATHS
		/usr/local/include
		/usr/include
		/usr/include/libusb-1.0
)

find_library(LIBUSB_LIBRARIES
	NAMES usb-1.0 usb
	PATHS
		/usr/local/lib
		/usr/lib
		/usr/lib/x86_64-linux-gnu
		/usr/lib64
)

find_package_handle_standard_args(libusb DEFAULT_MSG LIBUSB_LIBRARIES)

add_library(LIBUSB::LIBUSB UNKNOWN IMPORTED)
set_target_properties(LIBUSB::LIBUSB PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}")
set_target_properties(LIBUSB::LIBUSB PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${LIBUSB_LIBRARIES}")

if(COMMAND set_package_properties)
	set_package_properties(libusb PROPERTIES DESCRIPTION "libusb - user mode USB driver")
	set_package_properties(libusb PROPERTIES URL "https://libusb.info/")
endif()
