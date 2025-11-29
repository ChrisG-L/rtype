set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR})

add_library(rtype_sfml SHARED
    lib/sfml/src/AssetManager.cpp
    lib/sfml/src/SFMLRenderer.cpp
    lib/sfml/src/SFMLTexture.cpp
    lib/sfml/src/SFMLWindow.cpp

    lib/sfml/src/plugins/SFMLPlugin.cpp
)

set_target_properties(rtype_sfml PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(rtype_sfml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/sfml/include
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(rtype_sfml PRIVATE
    SFML::Graphics
    SFML::Audio
    SFML::Network
    spdlog::spdlog
    # rtype_proto
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
    src/scenes/SceneManager.cpp
    src/scenes/LoginScene.cpp
    src/scenes/GameScene.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(rtype_client PRIVATE
    # rtype_proto
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>
    $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>
    spdlog::spdlog
    rtype_sfml
)
