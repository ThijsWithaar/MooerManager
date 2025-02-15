
debian_add_package(libasound2-dev libfmt-dev libpipewire-0.3-dev libzip-dev zipcmp zipmerge ziptool libpugixml-dev)

if(DEFINED CMAKE_TOOLCHAIN_FILE)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(libusb REQUIRED IMPORTED_TARGET libusb-1.0)
else()
	find_package(libusb REQUIRED)
endif()

if(UNIX AND NOT APPLE)
	find_package(ALSA REQUIRED)
	find_package(pipewire REQUIRED)
	find_package(jack)
endif()

find_package(fmt REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(PugiXML REQUIRED)

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS *.h)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cc)

set(SOURCES_WIN TeVirtualMidi.cc WindowsMultiMedia.cc)
set(SOURCES_LINUX Alsa.cc Jack.cc Pipewire.cc)
if(NOT WIN32)
	foreach(SRC ${SOURCES_WIN})
		list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/midi/${SRC})
	endforeach()
endif()
if(NOT UNIX OR APPLE)
	foreach(SRC ${SOURCES_LINUX})
		list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/midi/${SRC})
	endforeach()
endif()


add_library(MooerLib ${HEADERS} ${SOURCES})

target_compile_features(MooerLib PUBLIC cxx_std_20)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# std::jthread is still expirimental
	target_compile_options(MooerLib PUBLIC "-fexperimental-library" "-Wno-invalid-offsetof")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(MooerLib PUBLIC "-Wno-invalid-offsetof")
endif()

target_include_directories(MooerLib PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

set_target_properties(MooerLib PROPERTIES PUBLIC_HEADER "${HEADERS}")

target_link_libraries(MooerLib PRIVATE
	fmt::fmt
	pugixml::pugixml
	${LIBUSB_LIBRARIES}
	libzip::zip
)
if(UNIX AND NOT APPLE)
	target_link_libraries(MooerLib
		PUBLIC 
			ALSA::ALSA
			LIBUSB::LIBUSB
			Pipewire::Spa
		PRIVATE
			Jack::Jack
			Pipewire::Pipewire
	)
endif()
if(DEFINED CMAKE_TOOLCHAIN_FILE)
	target_link_libraries(MooerLib PUBLIC PkgConfig::libusb)
endif()

if(WIN32)
	# https://stackoverflow.com/a/71134764/673137
	set(DEF ${CMAKE_CURRENT_SOURCE_DIR}/midi/teVirtualMIDI64.def)
	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/teVirtualMIDI64.lib
		COMMAND lib.exe /machine:X64 /def:${DEF} /out:${CMAKE_BINARY_DIR}/teVirtualMIDI64.lib
		DEPENDS ${DEF}
		VERBATIM
	)

	add_custom_target(teVirtualMIDI ALL DEPENDS ${CMAKE_BINARY_DIR}/teVirtualMIDI64.lib VERBATIM)

	add_dependencies(MooerLib teVirtualMIDI)
	target_link_libraries(MooerLib PRIVATE ${CMAKE_BINARY_DIR}/teVirtualMIDI64.lib)
endif()

# ToDo: See ../../eds_sim/lib/CMakeLists.txt for CMake script generation

install(TARGETS MooerLib 
	ARCHIVE DESTINATION lib 
	PUBLIC_HEADER DESTINATION include/Mooer
	COMPONENT dev
)
if(UNIX AND NOT APPLE)
	install(FILES 60-mooer.rules DESTINATION /etc/udev/rules.d COMPONENT lib)
endif()
if (CPACK_DEB_COMPONENT_INSTALL)
	set(CPACK_DEBIAN_DEV_PACKAGE_SHLIBDEPS ON PARENT_SCOPE)
endif()
