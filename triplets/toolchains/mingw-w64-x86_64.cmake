# MinGW-w64 toolchain file for cross-compiling to Windows x64 from Linux

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Indiquer que nous utilisons MinGW (pour désactiver les sanitizers)
set(MINGW TRUE)

# Specify the cross compiler
set(CMAKE_C_COMPILER /usr/bin/x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER /usr/bin/x86_64-w64-mingw32-windres)
set(CMAKE_AR /usr/bin/x86_64-w64-mingw32-ar)
set(CMAKE_RANLIB /usr/bin/x86_64-w64-mingw32-ranlib)

# Where to look for target files
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Adjust the default behavior of FIND_XXX() commands
# Note: We use BOTH instead of ONLY to allow vcpkg packages to be found
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# Static linking flags (for executables only)
set(CMAKE_CXX_FLAGS_INIT "")
set(CMAKE_C_FLAGS_INIT "")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++")
# DLLs cannot use -static-libstdc++ (causes multiple definition errors)
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-Wl,--allow-multiple-definition")

# Thread model
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)
