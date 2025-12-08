set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR})

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
)

# Find SDL2 from system (NixOS provides it via flake.nix)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)

# Fix for NixOS: pkg-config returns include/SDL2 but we need include for <SDL2/SDL.h>
set(SDL2_PARENT_INCLUDE_DIRS "")
foreach(dir ${SDL2_INCLUDE_DIRS})
    get_filename_component(parent_dir "${dir}" DIRECTORY)
    list(APPEND SDL2_PARENT_INCLUDE_DIRS "${parent_dir}")
endforeach()
list(REMOVE_DUPLICATES SDL2_PARENT_INCLUDE_DIRS)

set(SDL2_IMAGE_PARENT_INCLUDE_DIRS "")
foreach(dir ${SDL2_IMAGE_INCLUDE_DIRS})
    get_filename_component(parent_dir "${dir}" DIRECTORY)
    list(APPEND SDL2_IMAGE_PARENT_INCLUDE_DIRS "${parent_dir}")
endforeach()
list(REMOVE_DUPLICATES SDL2_IMAGE_PARENT_INCLUDE_DIRS)

target_include_directories(rtype_sdl2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/sdl2/include
    ${SDL2_PARENT_INCLUDE_DIRS}
    ${SDL2_IMAGE_PARENT_INCLUDE_DIRS}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(rtype_sdl2 PRIVATE
    PkgConfig::SDL2
    PkgConfig::SDL2_IMAGE
    spdlog::spdlog
)


add_executable(rtype_client
    main.cpp

    src/boot/Boot.cpp
    src/core/Engine.cpp
    src/core/GameLoop.cpp
    src/core/Logger.cpp
    src/core/DynamicLib.cpp
    src/graphics/Graphics.cpp
    src/network/TCPClient.cpp
    src/network/UDPClient.cpp
    src/scenes/SceneManager.cpp
    src/scenes/LoginScene.cpp
    src/scenes/GameScene.cpp

    src/gameplay/GameObject.cpp
    src/gameplay/Missile.cpp
    src/gameplay/EntityManager.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(rtype_client PRIVATE
        -O0                         # Pas d'optimisation en debug
        -g3                         # Niveau de debug maximum
        -Wall                       # Activer tous les warnings
        -Wextra                     # Activer les warnings supplémentaires
        -Wpedantic                  # Rendre le standard strict
    )
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
            -static-libasan
        )
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
        -O3                         # Optimisation maximale en release
        -Wall                       # Activer tous les warnings
        -Wextra                     # Activer les warnings supplémentaires
        -Wpedantic                  # Rendre le standard strict
    )
endif()

target_include_directories(rtype_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common/protocol
    )

target_link_libraries(rtype_client PRIVATE
    # rtype_proto
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>
    $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>
    spdlog::spdlog
    rtype_sdl2
)
