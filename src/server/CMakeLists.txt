add_executable(rtype_server
    main.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(rtype_server PRIVATE
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
    target_link_options(rtype_server PRIVATE
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=leak
        -static-libasan
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(
        -O3                         # Optimisation maximale en release
        -Wall                       # Activer tous les warnings
        -Wextra                     # Activer les warnings supplémentaires
        -Wpedantic                  # Rendre le standard strict
    )
endif()

target_include_directories(rtype_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rtype_server PRIVATE
    # Boost::system
    # Boost::thread
    # Boost::serialization
    # Boost::filesystem
    # $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>
    # $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>
)