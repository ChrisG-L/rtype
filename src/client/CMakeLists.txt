set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/client/${PLATFORM_DIR})

add_executable(rtype_client
    main.cpp

    # Core
    core/Game.cpp
    core/RTypeGame.cpp

    # Graphics
    graphics/Starfield.cpp

    # Network
    network/UDPClient.cpp
    network/NetworkSystem.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(rtype_client PRIVATE
        -O0                         # Pas d'optimisation en debug
        -g3                         # Niveau de debug maximum
        -Wall                       # Activer tous les warnings
        -Wextra                     # Activer les warnings supplémentaires
        -Wpedantic                  # Rendre le standard strict

        -fsanitize=address          # Détecte erreurs mémoire
        -fsanitize=undefined        # Détecte comportement indéfini
        -fsanitize=leak             # Détecte fuites mémoire
        -fno-omit-frame-pointer     # Garde les pointeurs de pile pour une meilleure traçabilité
    )
    target_link_options(rtype_client PRIVATE
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=leak
        -static-libasan
    )
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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rtype_client PRIVATE
    Boost::system
    SFML::Graphics
    SFML::Audio
    SFML::Network
    SFML::Window
    SFML::System
)
