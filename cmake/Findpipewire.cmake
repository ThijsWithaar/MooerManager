include(FindPackageHandleStandardArgs)

find_path(PIPEWIRE_INCLUDE_DIR
	NAMES pipewire/pipewire.h
	PATHS
		/usr/include/pipewire-0.3
		/usr/include
)

find_library(PIPEWIRE_LIBRARIES
	NAMES pipewire-0.3 pipewire
	PATHS
		/usr/local/lib
		/usr/lib
		/usr/lib/x86_64-linux-gnu
)

find_path(SPA_INCLUDE_DIR
	NAMES spa/buffer/buffer.h
	PATHS
		/usr/include/spa-0.2
		/usr/include
)

find_package_handle_standard_args(pipewire DEFAULT_MSG PIPEWIRE_LIBRARIES)

add_library(Pipewire::Pipewire UNKNOWN IMPORTED)
set_target_properties(Pipewire::Pipewire PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${PIPEWIRE_INCLUDE_DIR})
set_target_properties(Pipewire::Pipewire PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${PIPEWIRE_LIBRARIES}")

add_library(Pipewire::Spa INTERFACE IMPORTED)
set_target_properties(Pipewire::Spa PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${SPA_INCLUDE_DIR})
#set_target_properties(Pipewire::Spa PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C" IMPORTED_LOCATION "${SPA_LIBRARIES}")


if(COMMAND set_package_properties)
	set_package_properties(pipewire PROPERTIES DESCRIPTION "pipewire - audio & video under linux")
	set_package_properties(pipewire PROPERTIES URL "https://pipewire.org/")
endif()
