add_executable(rtype_server
    main.cpp

    # Domain - Exceptions
    domain/exceptions/DomainException.cpp
    domain/exceptions/HealthException.cpp
    domain/exceptions/PositionException.cpp
    domain/exceptions/player/PlayerIdException.cpp
    domain/exceptions/user/UserIdException.cpp
    domain/exceptions/user/UsernameException.cpp
    domain/exceptions/user/EmailException.cpp
    domain/exceptions/user/PasswordException.cpp
    domain/exceptions/persistence/MongoDBException.cpp

    # Domain - Value Objects
    domain/value_objects/Health.cpp
    domain/value_objects/Position.cpp
    domain/value_objects/player/PlayerId.cpp
    domain/value_objects/user/UserId.cpp
    domain/value_objects/user/Username.cpp
    domain/value_objects/user/Email.cpp
    domain/value_objects/user/Password.cpp
    domain/value_objects/user/utils/PasswordUtils.cpp

    # Domain - Entities
    domain/entities/Player.cpp
    domain/entities/User.cpp

    # Application - Use Cases
    application/use_cases/MovePlayerUseCase.cpp
    application/use_cases/auth/Login.cpp
    application/use_cases/auth/Register.cpp

    # Infrastructure - Adapters
    infrastructure/adapters/in/cli/CLIGameController.cpp
    infrastructure/adapters/in/network/UDPServer.cpp
    infrastructure/adapters/in/network/TCPServer.cpp
    infrastructure/adapters/in/network/protocol/CommandParser.cpp
    infrastructure/adapters/in/network/execute/Execute.cpp
    infrastructure/adapters/in/network/execute/auth/ExecuteAuth.cpp
    infrastructure/adapters/out/persistence/MongoDBConfiguration.cpp
    infrastructure/adapters/out/persistence/MongoDBUserRepository.cpp

    # Infrastructure - Logging
    infrastructure/logging/Logger.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(rtype_server PRIVATE
        -O0                         # Pas d'optimisation en debug
        -g3                         # Niveau de debug maximum
        -Wall                       # Activer tous les warnings
        -Wextra                     # Activer les warnings supplémentaires
        -Wpedantic                  # Rendre le standard strict
        -fno-omit-frame-pointer     # Garde les pointeurs de pile pour une meilleure traçabilité
    )

    # Sanitizers : uniquement pour builds natifs Linux/macOS (pas MinGW cross-compilation)
    if(NOT CMAKE_CROSSCOMPILING AND NOT MINGW)
        target_compile_options(rtype_server PRIVATE
            -fsanitize=address          # Détecte erreurs mémoire
            -fsanitize=undefined        # Détecte comportement indéfini
            -fsanitize=leak             # Détecte fuites mémoire
        )
        target_link_options(rtype_server PRIVATE
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
            target_link_options(rtype_server PRIVATE
                -Wl,--allow-multiple-definition
            )
            message(STATUS "Flag --allow-multiple-definition activé pour MinGW")
        endif()
    endif()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(rtype_server PRIVATE
        -O3                         # Optimisation maximale en release
        -Wall                       # Activer tous les warnings
        -Wextra                     # Activer les warnings supplémentaires
        -Wpedantic                  # Rendre le standard strict
    )
endif()

target_include_directories(rtype_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(rtype_server PRIVATE
    # rtype_proto
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>
    $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>
    spdlog::spdlog
)

# Bibliothèques Windows supplémentaires pour Boost.Asio
if(MINGW OR WIN32)
    target_link_libraries(rtype_server PRIVATE
        mswsock  # Pour AcceptEx, GetAcceptExSockaddrs, etc.
    )
endif()