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
    domain/entities/Room.cpp

    # Application - Use Cases
    application/use_cases/auth/Login.cpp
    application/use_cases/auth/Register.cpp

    # Infrastructure - Adapters In
    infrastructure/adapters/in/network/UDPServer.cpp
    infrastructure/adapters/in/network/VoiceUDPServer.cpp
    infrastructure/adapters/in/network/TCPAuthServer.cpp

    # Infrastructure - Adapters Out (Persistence)
    infrastructure/adapters/out/persistence/MongoDBConfiguration.cpp
    infrastructure/adapters/out/persistence/MongoDBUserRepository.cpp
    infrastructure/adapters/out/persistence/MongoDBUserSettingsRepository.cpp
    infrastructure/adapters/out/persistence/MongoDBChatMessageRepository.cpp

    # Infrastructure - Game
    infrastructure/game/GameWorld.cpp
    infrastructure/game/GameInstanceManager.cpp

    # Infrastructure - Session
    infrastructure/session/SessionManager.cpp

    # Infrastructure - Room
    infrastructure/room/RoomManager.cpp

    # Infrastructure - CLI
    infrastructure/cli/ServerCLI.cpp

    # Infrastructure - TUI
    infrastructure/tui/LogBuffer.cpp
    infrastructure/tui/TUISink.cpp
    infrastructure/tui/TerminalRenderer.cpp
    infrastructure/tui/TerminalUI.cpp
    infrastructure/tui/Utf8Utils.cpp

    # Infrastructure - Logging
    infrastructure/logging/Logger.cpp

    # Infrastructure - Network
    infrastructure/network/NetworkStats.cpp
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
    target_compile_options(rtype_server PRIVATE
        -O0                             # Pas d'optimisation en debug
        -g3                             # Niveau de debug maximum
        ${COMMON_WARNINGS}
        -fno-omit-frame-pointer         # Garde les pointeurs de pile pour traçabilité
    )

    # Options spécifiques Clang en debug
    if(IS_CLANG)
        target_compile_options(rtype_server PRIVATE
            ${CLANG_EXTRA_WARNINGS}
            ${CLANG_OPTIMIZATIONS}
            -fstandalone-debug          # Debug complet (pas de références externes)
        )
    endif()

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
        )

        # Linkage statique des sanitizers (spécifique à chaque compilateur)
        if(IS_GCC)
            target_link_options(rtype_server PRIVATE -static-libasan)
        elseif(IS_CLANG)
            # Clang utilise -static-libsan (sans le 'a')
            target_link_options(rtype_server PRIVATE -static-libsan)
        endif()

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
        -O3                             # Optimisation maximale
        ${COMMON_WARNINGS}
        -DNDEBUG                        # Désactive les assertions
    )

    # Optimisations spécifiques Clang en release
    if(IS_CLANG)
        target_compile_options(rtype_server PRIVATE
            ${CLANG_OPTIMIZATIONS}
            ${CLANG_RELEASE_OPTIMIZATIONS}
        )
        target_link_options(rtype_server PRIVATE
            -flto=thin                  # LTO aussi au linkage
            -fuse-ld=lld                # Linker LLVM (plus rapide)
        )
    elseif(IS_GCC)
        target_compile_options(rtype_server PRIVATE
            -flto                       # LTO standard GCC
            -fstrict-aliasing
        )
        target_link_options(rtype_server PRIVATE
            -flto
        )
    endif()
endif()

# Find MongoDB C++ driver
find_package(mongocxx CONFIG REQUIRED)
find_package(bsoncxx CONFIG REQUIRED)

target_include_directories(rtype_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common/protocol
    ${PROJECT_SOURCE_DIR}/src/common
    )

target_link_libraries(rtype_server PRIVATE
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    $<IF:$<TARGET_EXISTS:mongo::mongocxx_shared>,mongo::mongocxx_shared,mongo::mongocxx_static>
    $<IF:$<TARGET_EXISTS:mongo::bsoncxx_shared>,mongo::bsoncxx_shared,mongo::bsoncxx_static>
)

# Bibliothèques Windows supplémentaires pour Boost.Asio
if(MINGW OR WIN32)
    target_link_libraries(rtype_server PRIVATE
        mswsock  # Pour AcceptEx, GetAcceptExSockaddrs, etc.
    )
endif()