set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)

# Customizations
set(VCPKG_BUILD_TYPE release)
#  Qt6 is not yet compatible with OSX-15/Sequioa
set(VCPKG_OSX_DEPLOYMENT_TARGET 14.0)
#  Sequoia's libstdc++ does not have std::jthread and stoptoken
set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS}")
set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -stdlib=libc++")

#  Enable Link-Time-Optimization
#set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS}" -flto)
#set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -flto")
#set(VCPKG_LINKER_FLAGS -flto)
