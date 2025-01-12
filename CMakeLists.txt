cmake_minimum_required(VERSION 3.25)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(${CMAKE_SOURCE_DIR}/cmake/debian.cmake)
debian_add_package(qt6-base-dev libjack-jackd2-dev libpipewire-0.3-dev libusb-1.0-0-dev)

project(MooerManager VERSION 0.1 LANGUAGES CXX)


add_subdirectory(mooer.lib)
add_subdirectory(mooer.gui)


set(CPACK_PACKAGE_CONTACT "Thijs Withaar <thijs.withaar@gmail.com>")
if(WIN32)
	set(CPACK_GENERATOR ZIP)
elseif(APPLE)
	set(CPACK_GENERATOR Bundle)
	set(CPACK_BUNDLE_NAME MooeManager)
	set(CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/mooer.gui/resources/MooerManager.plist)
	set(CPACK_BUNDLE_ICON ${CMAKE_BINARY_DIR}/AppIcons)
elseif(UNIX)
	cmake_host_system_information(RESULT DISTRO QUERY DISTRIB_INFO)
	if(${DISTRO_ID} STREQUAL "fedora")
		set(CPACK_GENERATOR RPM)
		set(DISTRO_VERSION_CODENAME "rawhide")
	endif()
	set(CPACK_SYSTEM_NAME "${DISTRO_ID}-${DISTRO_VERSION_CODENAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

include(CPack)
