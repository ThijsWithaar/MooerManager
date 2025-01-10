set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64)

# Customizations
set(VCPKG_BUILD_TYPE release)
#  Qt6 is not yet compatible with OSX-15/Sequioa
set(VCPKG_OSX_DEPLOYMENT_TARGET 14.0)
#  Sequoia's libstdc++ does not have std::jthread and stoptoken, so use LLVM's libc++
set(LLVM_ROOT "/usr/local/opt/llvm\@18")
set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS}")
set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} -nostdinc++ -stdlib=libc++ -I${LLVM_ROOT}/include/c++/v1/ -L${LLVM_ROOT}/lib -Wl,-rpath,${LLVM_ROOT}/lib")
set(VCPKG_LINKER_FLAGS "${VCPKG_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
