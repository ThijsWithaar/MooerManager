# To be used with -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=

execute_process(COMMAND brew --prefix llvm@18 OUTPUT_VARIABLE LLVM_ROOT)
string(STRIP ${LLVM_ROOT} LLVM_ROOT)

set(CMAKE_C_COMPILER ${LLVM_ROOT}/bin/clang)
set(CMAKE_CXX_COMPILER ${LLVM_ROOT}/bin/clang++)
set(CMAKE_CXX_FLAGS "-nostdinc++ -stdlib=libc++ -isystem${LLVM_ROOT}/include/c++/v1")
set(CMAKE_EXE_LINKER_FLAGS "-L${LLVM_ROOT}/lib/c++ -lc++ -L${LLVM_ROOT}/lib -lunwind")
