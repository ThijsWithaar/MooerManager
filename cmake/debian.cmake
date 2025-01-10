# Allows installing of packages during configuration
# By default it is disabled, using the DEBIAN_INSTALL_DEPENDENCIES option
#
# Example:
# 	debian_add_package(libboost-dev)
#	find_package(Boost QUIET REQUIRED COMPONENTS program_options


function(is_debian ret)
	if(EXISTS /etc/debian_version)
		set(${ret} TRUE PARENT_SCOPE)
	else()
		set(${ret} FALSE PARENT_SCOPE)
	endif()
endfunction(is_debian)

set_property(GLOBAL PROPERTY debian_packages)

function(debian_add_package)
	# Keep a list of all build-dependencies for the project
	get_property(tmp GLOBAL PROPERTY debian_packages)
	list(APPEND tmp ${ARGN})
	list(REMOVE_DUPLICATES tmp)
	list(SORT tmp)
	set_property(GLOBAL PROPERTY debian_packages "${tmp}")

	is_debian(isdeb)
	if(isdeb)
		foreach(name ${ARGN})
			execute_process(COMMAND "dpkg" "-s" "${name}" OUTPUT_FILE /dev/null ERROR_FILE /dev/null RESULT_VARIABLE ret)
			if(ret EQUAL "0")
				#message("debian_add_package: ${name} is installed (retcode ${ret})")
			else()
				message("debian_add_package: Installing ${name}:")
				execute_process(COMMAND "sudo" "apt" "install" "-y" "-qq" "${name}")
			endif()
		endforeach()
	endif()
endfunction(debian_add_package)
