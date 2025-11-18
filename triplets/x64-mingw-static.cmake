# x64-mingw-static triplet for cross-compiling Windows binaries from Linux
# This triplet uses MinGW-w64 to produce static Windows executables

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME MinGW)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/toolchains/mingw-w64-x86_64.cmake)

# Use MinGW compilers
set(VCPKG_CXX_FLAGS "-static -static-libgcc -static-libstdc++")
set(VCPKG_C_FLAGS "-static -static-libgcc")
set(VCPKG_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

# Policy: Build Release configuration for dependencies
set(VCPKG_BUILD_TYPE release)
