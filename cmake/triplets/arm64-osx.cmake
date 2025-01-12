set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# Customizations
set(VCPKG_BUILD_TYPE release)
#  Qt6 is not yet compatible with OSX-15/Sequioa
set(VCPKG_OSX_DEPLOYMENT_TARGET 14.0)

# Sequoia's libstdc++ does not have std::jthread and stoptoken
# See VCPKG_CHAINLOAD_TOOLCHAIN_FILE in CMakePresets.json

# Would like to link everything static, which vcpkg's dbus does not support
#set(VCPKG_CRT_LINKAGE static)
