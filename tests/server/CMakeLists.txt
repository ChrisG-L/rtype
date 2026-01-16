#===============================================================================
# R-Type Server Tests - CMakeLists.txt
#===============================================================================
# Configuration des tests unitaires pour le serveur R-Type
# Utilise Google Test (GTest) pour les tests
#===============================================================================

project("server_tests" VERSION 0.0.1 LANGUAGES CXX)

# Sources de tests
set(TEST_SOURCES
    main.cpp

    # Tests Value Objects
    domain/value_objects/HealthTest.cpp
    domain/value_objects/PositionTest.cpp
    domain/value_objects/EmailTest.cpp
    domain/value_objects/UsernameTest.cpp

    # Tests Entities
    domain/entities/PlayerTest.cpp

    # Tests Network - Integration UDP
    network/UDPIntegrationTest.cpp

    # Tests Network - Voice Protocol
    network/VoiceProtocolTest.cpp

    # Tests Infrastructure - Session (CSPRNG crypto tests)
    infrastructure/session/SessionManagerCryptoTest.cpp

    # Tests Infrastructure - Session (GodMode hidden feature)
    infrastructure/session/SessionManagerGodModeTest.cpp

    # Tests Application - Services (Leaderboard & Achievements)
    application/services/AchievementCheckerTest.cpp
    application/services/LeaderboardDataTest.cpp
    application/services/WeaponLevelsTest.cpp
)

# Sources du serveur nécessaires pour les tests
# (les fichiers .cpp du code source à tester)
set(SERVER_SOURCES
    # Domain - Exceptions
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/DomainException.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/HealthException.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/PositionException.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/player/PlayerIdException.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/user/UserIdException.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/user/UsernameException.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/user/EmailException.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/exceptions/user/PasswordException.cpp

    # Domain - Value Objects
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/Health.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/Position.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/player/PlayerId.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/user/UserId.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/user/Username.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/user/Email.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/user/Password.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/value_objects/user/utils/PasswordUtils.cpp

    # Domain - Entities
    ${CMAKE_SOURCE_DIR}/src/server/domain/entities/Player.cpp
    ${CMAKE_SOURCE_DIR}/src/server/domain/entities/User.cpp

    # Infrastructure - Session (for CSPRNG tests)
    ${CMAKE_SOURCE_DIR}/src/server/infrastructure/session/SessionManager.cpp

    # Infrastructure - Logging (required by SessionManager)
    ${CMAKE_SOURCE_DIR}/src/server/infrastructure/logging/Logger.cpp

    # Infrastructure - TUI (required by Logger)
    ${CMAKE_SOURCE_DIR}/src/server/infrastructure/tui/TUISink.cpp
    ${CMAKE_SOURCE_DIR}/src/server/infrastructure/tui/LogBuffer.cpp

    # Application - Services (Leaderboard & Achievements)
    ${CMAKE_SOURCE_DIR}/src/server/application/services/AchievementChecker.cpp
)

# Créer l'exécutable de tests
add_executable(server_tests
    ${TEST_SOURCES}
    ${SERVER_SOURCES}
)

# Configuration de compilation pour les tests
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(server_tests PRIVATE
        -O0
        -g3
        -Wall
        -Wextra
    )
endif()

# Inclure les répertoires d'en-têtes
target_include_directories(server_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/server/include
    ${CMAKE_SOURCE_DIR}/src/common/protocol  # Protocol.hpp
    ${CMAKE_SOURCE_DIR}/src/common           # collision/AABB.hpp
    ${CMAKE_BINARY_DIR}  # Pour les headers Protobuf générés
    ${Protobuf_INCLUDE_DIRS}
)

# Lier les bibliothèques nécessaires
target_link_libraries(server_tests PRIVATE
    Boost::system
    GTest::gtest
    GTest::gtest_main
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    fmt::fmt
)

# Bibliothèques Windows pour Boost.Asio (tests UDP)
if(MINGW OR WIN32)
    target_link_libraries(server_tests PRIVATE
        ws2_32    # Windows Sockets 2
        mswsock   # Microsoft Sockets extensions
    )
    # MinGW + static linking = problèmes de définitions multiples
    target_link_options(server_tests PRIVATE
        -Wl,--allow-multiple-definition
    )
endif()

# Enregistrer les tests avec CTest (sauf en cross-compilation)
if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(server_tests)
endif()

# Définir la sortie des tests dans artifacts/tests
set_target_properties(server_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/tests
)

# Message de configuration
message(STATUS "Server tests configured with ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Test output directory: ${CMAKE_SOURCE_DIR}/artifacts/tests")
