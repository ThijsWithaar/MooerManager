set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

# Customizations
set(VCPKG_BUILD_TYPE release)
#set(VCPKG_CRT_LINKAGE static) # static-crt, try to make the executables smaller

# Link-Time-Optimization: Try to make Qt-binaries smaller
#set(VCPKG_C_FLAGS_RELEASE "/GL /Gw /GS-")
#set(VCPKG_CXX_FLAGS_RELEASE "/GL /Gw /GS-")
#set(VCPKG_LINKER_FLAGS_RELEASE "/OPT:ICF=3 /LTCG")
