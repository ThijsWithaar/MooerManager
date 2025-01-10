cmake_minimum_required(VERSION 3.25)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(${CMAKE_SOURCE_DIR}/cmake/debian.cmake)
debian_add_package(qt6-base-dev libjack-jackd2-dev libpipewire-0.3-dev libusb-1.0-0-dev)

project(MooerManager VERSION 0.1 LANGUAGES CXX)

# These should match the VCPKG triplets
if(DEFINED VCPKG_BUILD_TYPE AND APPLE)
	# Sequoia's libstdc++ does not have std::jthread and stoptoken
	set(LLVM_ROOT "/usr/local/opt/llvm\@18")
	add_compile_options(-nostdinc++ -stdlib=libc++ -I${LLVM_ROOT}/include/c++/v1/ -L${LLVM_ROOT}/lib -Wl,-rpath,${LLVM_ROOT}/lib)
	add_link_options(-stdlib=libc++ -nodefaultlibs -lc++abi)
	message("** Using libc++ from ${LLVM_ROOT} **")
endif()

add_subdirectory(mooer.lib)
add_subdirectory(mooer.gui)


set(CPACK_PACKAGE_CONTACT "Thijs Withaar <thijs.withaar@gmail.com>")
if(WIN32)
	set(CPACK_GENERATOR ZIP)
elseif(APPLE)
	set(CPACK_GENERATOR Bundle)
	set(CPACK_BUNDLE_NAME MooeManager)
	set(CPACK_BUNDLE_PLIST ${CMAKE_SOURCE_DIR}/MooerManager.plist)
	set(CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/appIcon)
elseif(UNIX)
	cmake_host_system_information(RESULT DISTRO QUERY DISTRIB_INFO)
	if(${DISTRO_ID} STREQUAL "fedora")
		set(CPACK_GENERATOR RPM)
		set(DISTRO_VERSION_CODENAME "rawhide")
	endif()
	set(CPACK_SYSTEM_NAME "${DISTRO_ID}-${DISTRO_VERSION_CODENAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

include(CPack)
