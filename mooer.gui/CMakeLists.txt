set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 CONFIG COMPONENTS Core Widgets Network)

file(GLOB HEADERS CONFIGURE_DEPENDS *.h)
file(GLOB SOURCES CONFIGURE_DEPENDS *.cc)
file(GLOB QTUI CONFIGURE_DEPENDS *.ui)

qt_add_executable(MooerManager ${HEADERS} ${SOURCES} ${QTUI})

target_link_libraries(MooerManager PUBLIC
	MooerLib
	Qt::Widgets
	Qt::Network
)

install(TARGETS MooerManager DESTINATION bin COMPONENT gui)

file(GLOB fnMagicWin "C:/Program Files/ImageMagick8/magick.exe")
get_filename_component(ptMagicWin "${fnMagicWin}" DIRECTORY)
find_program(MAGICK NAMES convert-im6 magick convert PATHS "/usr/bin" "/usr/local/bin" "${ptMagicWin}")

function(add_resize_command SVG SIZE PNG_OUT)
	get_filename_component(PNG_PATH ${PNG_OUT} DIRECTORY)
	add_custom_command(
		OUTPUT ${PNG_OUT}
		DEPENDS ${SVG}
		COMMAND cmake -E make_directory ${PNG_PATH}
		COMMAND "${MAGICK}" "-background" "none" "${SVG}" "-density" "4000" "-resize" "${SIZE}x${SIZE}" "${PNG_OUT}"
	)
endfunction()

file(GLOB SVGS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/*.svg)
foreach(SVG ${SVGS})
	get_filename_component(SVG_WE ${SVG} NAME_WE)
	set(PNG_OUT ${CMAKE_CURRENT_BINARY_DIR}/${SVG_WE}.png)
	set(ICO_OUT ${CMAKE_CURRENT_BINARY_DIR}/${SVG_WE}.ico)
	set(SVG_OUT ${CMAKE_CURRENT_BINARY_DIR}/${SVG_WE}.svg)

	add_resize_command(${SVG} 32 ${PNG_OUT})

	add_custom_command(OUTPUT ${ICO_OUT}
		COMMAND "${MAGICK}" -background transparent ${SVG} -define icon:auto-resize=16,32,48,64 ${ICO_OUT}
		COMMENT "Generating icon ${ICO_OUT}"
		DEPENDS ${PNG_OUT}
	)

	configure_file(${SVG} ${SVG_OUT} COPYONLY)

	set(FILES_OUT ${PNG_OUT} ${ICO_OUT} ${SVG_OUT})
	add_custom_target(create_icon_${SVG_WE} DEPENDS ${FILES_OUT})
	add_dependencies(MooerManager create_icon_${SVG_WE})
endforeach()

function(create_resized_png SVG PTDST WIDTH HEIGHT SUFFIX)
	add_custom_command(OUTPUT ${PTDST}/${SUFFIX}
		COMMAND ${MAGICK} -background none ${SVG} -resize ${WIDTH}x${HEIGHT} ${PNG_OUT}
		COMMENT Generating ${PTDST}/${SUFFIX}
		DEPENDS ${SVG}
	)
endfunction()

function(create_apple_icons SVGIN PTDST)
	# See https://stackoverflow.com/a/71149490/673137
	create_resized_png(${SVGIN} ${PTDST} 20 20 "-20x20@1x")
	create_resized_png(${SVGIN} ${PTDST} 20 20 "-20x20@2x")
	create_resized_png(${SVGIN} ${PTDST} 60 60 "-20x20@3x")
	create_resized_png(${SVGIN} ${PTDST} 29 29 "-29x29@1x")
	create_resized_png(${SVGIN} ${PTDST} 58 58 "-29x29@2x")
	create_resized_png(${SVGIN} ${PTDST} 87 87 "-29x29@3x")
	create_resized_png(${SVGIN} ${PTDST} 40 40 "-40x40@1x")
	create_resized_png(${SVGIN} ${PTDST} 80 80 "-40x40@2x")
	create_resized_png(${SVGIN} ${PTDST} 120 120"-40x40@3x")
	create_resized_png(${SVGIN} ${PTDST} 120 120 "-60x60@2x")
	create_resized_png(${SVGIN} ${PTDST} 180 180 "-60x60@3x")

	get_filename_component(SVG_WE ${SVGIN} NAME_WE)
	add_custom_target(create_icon_${SVG_WE} DEPENDS "${PTDST}-20x20@1x" COMMENT "Creating Icon: ${SVG_WE}")
	add_dependencies(MooerManager create_icon_${SVG_WE})
endfunction()

if(WIN32)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/MooerManager.rc ${CMAKE_CURRENT_BINARY_DIR}/MooerManager.rc COPYONLY)
	target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/MooerManager.rc)
	# Copy the .dlls from the vcpkg build as well
	install(
		DIRECTORY
			$<$<CONFIG:MinSizeRel>:${CMAKE_BINARY_DIR}/MinSizeRel>/
			$<$<CONFIG:RelWithDebInfo>:${CMAKE_BINARY_DIR}/RelWithDebInfo>/
		DESTINATION bin
		COMPONENT gui
		FILES_MATCHING PATTERN "*.dll" PATTERN "*.conf"
	)
elseif(APPLE)
	set_target_properties(MooerManager PROPERTIES MACOSX_BUNDLE TRUE)
	# create_apple_icons(SVG ${CMAKE_CURRENT_BINARY_DIR}/AppIcons)
elseif(UNIX)
	# Desktop shortcut and icon
	# https://stackoverflow.com/questions/14235798/specify-an-icon-and-menu-entry-for-cmake-cpack-application
	set(DESKTOP_DIR "share/applications")
	set(PIXMAPS_DIR "share/icons")
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/MooerManager.desktop DESTINATION ${DESKTOP_DIR} COMPONENT gui)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MooerManager.png DESTINATION ${PIXMAPS_DIR} COMPONENT gui)

	# Installer: Add to start-menu
	if (CPACK_DEB_COMPONENT_INSTALL)
		set(CPACK_DEBIAN_GUI_PACKAGE_SHLIBDEPS ON PARENT_SCOPE)
		file(GLOB CONTROL_EXTRA CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/deb/*)
		set(CPACK_DEBIAN_GUI_PACKAGE_CONTROL_EXTRA ${CONTROL_EXTRA} PARENT_SCOPE)
		set(CPACK_DEBIAN_GUI_PACKAGE_CONTROL_STRICT_PERMISSION TRUE PARENT_SCOPE)
	endif()
endif()
