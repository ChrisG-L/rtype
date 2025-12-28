set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR})

# Common library (shared between client and plugins)
add_library(rtype_client_common STATIC
    src/core/Logger.cpp
    src/graphics/Graphics.cpp
)

set_target_properties(rtype_client_common PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(rtype_client_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtype_client_common PUBLIC
    spdlog::spdlog
)

# SDL2 Plugin Library
add_library(rtype_sdl2 SHARED
    lib/sdl2/src/SDL2AssetManager.cpp
    lib/sdl2/src/SDL2Renderer.cpp
    lib/sdl2/src/SDL2Window.cpp
    lib/sdl2/src/SDL2Plugin.cpp
)

set_target_properties(rtype_sdl2 PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    # Assurer que la DLL/so va dans le même répertoire que l'exécutable
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR}
)

# Windows: enlever le préfixe "lib" pour avoir rtype_sdl2.dll (pas librtype_sdl2.dll)
if(WIN32 OR MINGW)
    set_target_properties(rtype_sdl2 PROPERTIES PREFIX "")
endif()

# MinGW: liaison statique du runtime C++ avec symboles cachés
# Évite le conflit ABI avec l'exécutable qui a aussi libstdc++ statique
# --allow-multiple-definition résout les conflits inline vs static lib
if(MINGW)
    target_link_options(rtype_sdl2 PRIVATE
        -static-libgcc
        -static-libstdc++
        -Wl,--allow-multiple-definition
        -Wl,--exclude-libs,libstdc++.a
        -Wl,--exclude-libs,libgcc.a
        -Wl,--exclude-libs,libgcc_eh.a
    )
    message(STATUS "rtype_sdl2: liaison statique C++ runtime avec symboles cachés (MinGW)")
endif()

# Find SDL2 packages - try vcpkg (CONFIG) first, fallback to pkg-config (system/Nix)
find_package(SDL2 CONFIG QUIET)
find_package(SDL2_image CONFIG QUIET)
find_package(SDL2_ttf CONFIG QUIET)
find_package(SDL2_mixer CONFIG QUIET)

# Check if vcpkg found all packages
if(TARGET SDL2::SDL2 OR TARGET SDL2::SDL2-static)
    set(SDL2_FOUND_VIA_VCPKG TRUE)
    message(STATUS "SDL2 found via vcpkg")
else()
    set(SDL2_FOUND_VIA_VCPKG FALSE)
    message(STATUS "SDL2 not found via vcpkg, trying pkg-config...")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
    # SDL2_ttf and SDL2_mixer via find_library for NixOS compatibility
    find_library(SDL2_TTF_LIBRARY NAMES SDL2_ttf REQUIRED)
    find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h PATH_SUFFIXES SDL2 REQUIRED)
    find_library(SDL2_MIXER_LIBRARY NAMES SDL2_mixer REQUIRED)
    find_path(SDL2_MIXER_INCLUDE_DIR SDL_mixer.h PATH_SUFFIXES SDL2 REQUIRED)

    # Fix for NixOS: get parent directories for includes
    get_filename_component(SDL2_MIXER_PARENT_INCLUDE_DIR "${SDL2_MIXER_INCLUDE_DIR}" DIRECTORY)
    set(SDL2_PARENT_INCLUDE_DIRS "")
    foreach(dir ${SDL2_INCLUDE_DIRS})
        get_filename_component(parent_dir "${dir}" DIRECTORY)
        list(APPEND SDL2_PARENT_INCLUDE_DIRS "${parent_dir}")
    endforeach()
    list(REMOVE_DUPLICATES SDL2_PARENT_INCLUDE_DIRS)
endif()

target_include_directories(rtype_sdl2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/sdl2/include
    $<$<NOT:$<BOOL:${SDL2_FOUND_VIA_VCPKG}>>:${SDL2_PARENT_INCLUDE_DIRS}>
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(SDL2_FOUND_VIA_VCPKG)
    target_link_libraries(rtype_sdl2 PRIVATE
        rtype_client_common
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
        $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    )
else()
    target_link_libraries(rtype_sdl2 PRIVATE
        rtype_client_common
        PkgConfig::SDL2
        PkgConfig::SDL2_IMAGE
        ${SDL2_TTF_LIBRARY}
    )
endif()

# SFML Plugin Library
add_library(rtype_sfml SHARED
    lib/sfml/src/AssetManager.cpp
    lib/sfml/src/SFMLRenderer.cpp
    lib/sfml/src/SFMLWindow.cpp
    lib/sfml/src/SFMLTexture.cpp
    lib/sfml/src/plugins/SFMLPlugin.cpp
)

set_target_properties(rtype_sfml PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR}
)

# Windows: enlever le préfixe "lib" pour avoir rtype_sfml.dll (pas librtype_sfml.dll)
if(WIN32 OR MINGW)
    set_target_properties(rtype_sfml PROPERTIES PREFIX "")
endif()

# MinGW: liaison statique du runtime C++
if(MINGW)
    target_link_options(rtype_sfml PRIVATE
        -static-libgcc
        -static-libstdc++
        -Wl,--allow-multiple-definition
        -Wl,--exclude-libs,libstdc++.a
        -Wl,--exclude-libs,libgcc.a
        -Wl,--exclude-libs,libgcc_eh.a
    )
    message(STATUS "rtype_sfml: liaison statique C++ runtime avec symboles cachés (MinGW)")
endif()

# Find SFML - try vcpkg (CONFIG) first, fallback to pkg-config
find_package(SFML 3 COMPONENTS Graphics Window System CONFIG QUIET)

if(TARGET SFML::Graphics)
    set(SFML_FOUND_VIA_VCPKG TRUE)
    message(STATUS "SFML found via vcpkg")
else()
    set(SFML_FOUND_VIA_VCPKG FALSE)
    message(STATUS "SFML not found via vcpkg, trying pkg-config...")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SFML REQUIRED IMPORTED_TARGET sfml-graphics sfml-window sfml-system)
endif()

target_include_directories(rtype_sfml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/sfml/include
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(SFML_FOUND_VIA_VCPKG)
    target_link_libraries(rtype_sfml PRIVATE
        rtype_client_common
        SFML::Graphics
        SFML::Window
        SFML::System
    )
else()
    target_link_libraries(rtype_sfml PRIVATE
        rtype_client_common
        PkgConfig::SFML
    )
endif()

add_executable(rtype_client
    main.cpp

    src/boot/Boot.cpp
    src/core/Engine.cpp
    src/core/GameLoop.cpp
    src/core/DynamicLib.cpp
    src/network/UDPClient.cpp
    src/network/TCPClient.cpp
    src/scenes/SceneManager.cpp
    src/scenes/GameScene.cpp
    src/scenes/LoginScene.cpp
    src/scenes/MainMenuScene.cpp
    src/scenes/ConnectionScene.cpp

    src/ui/Button.cpp
    src/ui/TextInput.cpp
    src/ui/StarfieldBackground.cpp

    src/gameplay/GameObject.cpp
    src/gameplay/Missile.cpp
    src/gameplay/EntityManager.cpp

    src/accessibility/AccessibilityConfig.cpp
    src/audio/AudioManager.cpp
)

# RPATH configuration: chercher les .so dans le même répertoire que l'exécutable
set_target_properties(rtype_client PROPERTIES
    BUILD_RPATH_USE_ORIGIN ON
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH "$ORIGIN"
)

# Détection du compilateur
set(IS_CLANG FALSE)
set(IS_GCC FALSE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(IS_CLANG TRUE)
    message(STATUS "Compilateur détecté: Clang")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(IS_GCC TRUE)
    message(STATUS "Compilateur détecté: GCC")
endif()

# Options communes à tous les compilateurs
set(COMMON_WARNINGS -Wall -Wextra -Wpedantic)

# Options spécifiques Clang
set(CLANG_EXTRA_WARNINGS
    -Wno-unused-command-line-argument  # Évite warnings sur flags non utilisés
    -Wconversion                        # Conversions implicites
    -Wshadow                            # Variables masquées
    -Wnon-virtual-dtor                  # Destructeurs non virtuels
)

set(CLANG_OPTIMIZATIONS
    -fcolor-diagnostics                 # Messages colorés
)

set(CLANG_RELEASE_OPTIMIZATIONS
    -flto=thin                          # Link Time Optimization (rapide)
    -fstrict-aliasing                   # Optimisations d'aliasing
    -fomit-frame-pointer                # Optimisation des registres
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(rtype_client PRIVATE
        -O0                             # Pas d'optimisation en debug
        -g3                             # Niveau de debug maximum
        ${COMMON_WARNINGS}
        -fno-omit-frame-pointer         # Garde les pointeurs de pile pour traçabilité
    )

    # Options spécifiques Clang en debug
    if(IS_CLANG)
        target_compile_options(rtype_client PRIVATE
            ${CLANG_EXTRA_WARNINGS}
            ${CLANG_OPTIMIZATIONS}
            -fstandalone-debug          # Debug complet (pas de références externes)
        )
    endif()

    # Sanitizers : uniquement pour builds natifs Linux/macOS (pas MinGW cross-compilation)
    if(NOT CMAKE_CROSSCOMPILING AND NOT MINGW)
        target_compile_options(rtype_client PRIVATE
            -fsanitize=address          # Détecte erreurs mémoire
            -fsanitize=undefined        # Détecte comportement indéfini
            -fsanitize=leak             # Détecte fuites mémoire
        )
        target_link_options(rtype_client PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fsanitize=leak
        )

        # Linkage statique des sanitizers (spécifique à chaque compilateur)
        if(IS_GCC)
            target_link_options(rtype_client PRIVATE -static-libasan)
        elseif(IS_CLANG)
            # Clang utilise -static-libsan (sans le 'a')
            target_link_options(rtype_client PRIVATE -static-libsan)
        endif()

        message(STATUS "Sanitizers activés (build natif)")
    else()
        message(STATUS "Sanitizers désactivés (cross-compilation ou MinGW)")
        # MinGW + static linking + C++ = problèmes de définitions multiples
        if(MINGW)
            target_link_options(rtype_client PRIVATE
                -Wl,--allow-multiple-definition
            )
            message(STATUS "Flag --allow-multiple-definition activé pour MinGW")
        endif()
    endif()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(rtype_client PRIVATE
        -O3                             # Optimisation maximale
        ${COMMON_WARNINGS}
        -DNDEBUG                        # Désactive les assertions
    )

    # Optimisations spécifiques Clang en release
    if(IS_CLANG)
        target_compile_options(rtype_client PRIVATE
            ${CLANG_OPTIMIZATIONS}
            ${CLANG_RELEASE_OPTIMIZATIONS}
        )
        target_link_options(rtype_client PRIVATE
            -flto=thin                  # LTO aussi au linkage
            -fuse-ld=lld                # Linker LLVM (plus rapide)
        )
    elseif(IS_GCC)
        target_compile_options(rtype_client PRIVATE
            -flto                       # LTO standard GCC
            -fstrict-aliasing
        )
        target_link_options(rtype_client PRIVATE
            -flto
        )
    endif()
endif()

target_include_directories(rtype_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common/protocol
    $<$<NOT:$<BOOL:${SDL2_FOUND_VIA_VCPKG}>>:${SDL2_PARENT_INCLUDE_DIRS}>
    $<$<NOT:$<BOOL:${SDL2_FOUND_VIA_VCPKG}>>:${SDL2_MIXER_PARENT_INCLUDE_DIR}>
    )

if(SDL2_FOUND_VIA_VCPKG)
    target_link_libraries(rtype_client PRIVATE
        rtype_client_common
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        rtype_sdl2
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    )
else()
    target_link_libraries(rtype_client PRIVATE
        rtype_client_common
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        rtype_sdl2
        ${SDL2_MIXER_LIBRARY}
        PkgConfig::SDL2
    )
endif()
