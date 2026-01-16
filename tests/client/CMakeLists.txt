#===============================================================================
# R-Type Client Tests - CMakeLists.txt
#===============================================================================
# Configuration des tests unitaires pour le client R-Type
# Utilise Google Test (GTest) pour les tests
#===============================================================================

project("client_tests" VERSION 0.0.1 LANGUAGES CXX)

# Sources de tests
set(TEST_SOURCES
    main.cpp

    # Tests Utils
    utils/VecsTest.cpp
    utils/SignalTest.cpp

    # Tests Config
    config/ServerConfigManagerTest.cpp

    # Tests UI
    ui/ServerConfigPanelTest.cpp

    # Tests Accessibility
    accessibility/AccessibilityConfigTest.cpp

    # Tests Audio
    audio/VoiceAudioTest.cpp
    audio/PortAudioTest.cpp
    audio/VoiceIntegrationTest.cpp
    audio/AudioDevicePersistenceTest.cpp
)

# Sources du client nécessaires pour les tests
set(CLIENT_SOURCES_FOR_TESTS
    ${CMAKE_SOURCE_DIR}/src/client/src/accessibility/AccessibilityConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/client/src/config/ServerConfigManager.cpp
    ${CMAKE_SOURCE_DIR}/src/client/src/core/Logger.cpp
    ${CMAKE_SOURCE_DIR}/src/client/src/ui/ServerConfigPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/client/src/ui/Button.cpp
    ${CMAKE_SOURCE_DIR}/src/client/src/ui/TextInput.cpp
)

# Créer l'exécutable de tests
add_executable(client_tests
    ${TEST_SOURCES}
    ${CLIENT_SOURCES_FOR_TESTS}
)

# Configuration de compilation pour les tests
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(client_tests PRIVATE
        -O0
        -g3
        -Wall
        -Wextra
    )
endif()

# Inclure les répertoires d'en-têtes du client
target_include_directories(client_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/src/client/include
)

# Trouver spdlog pour le Logger
find_package(spdlog CONFIG REQUIRED)

# Trouver Opus et PortAudio pour les tests audio
find_package(Opus CONFIG REQUIRED)

# PortAudio - essayer d'abord vcpkg (portaudio_static), puis pkg-config
find_package(portaudio CONFIG QUIET)
if(TARGET portaudio_static)
    set(PORTAUDIO_TARGET portaudio_static)
    message(STATUS "[Tests] PortAudio found via vcpkg (portaudio_static)")
elseif(TARGET portaudio)
    set(PORTAUDIO_TARGET portaudio)
    message(STATUS "[Tests] PortAudio found via vcpkg (portaudio)")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
    target_include_directories(client_tests PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
    set(PORTAUDIO_TARGET ${PORTAUDIO_LIBRARIES})
    message(STATUS "[Tests] PortAudio found via pkg-config")
endif()

# Lier les bibliothèques nécessaires
# Note: portaudio_static includes INTERFACE_LINK_LIBRARIES with winmm, dsound, etc.
target_link_libraries(client_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    spdlog::spdlog
    Opus::opus
    ${PORTAUDIO_TARGET}
)

# MinGW: options de liaison pour éviter les conflits de définitions multiples
if(MINGW)
    target_link_options(client_tests PRIVATE
        -static-libgcc
        -static-libstdc++
        -Wl,--allow-multiple-definition
    )
    message(STATUS "[Tests] MinGW link options enabled for client_tests")
endif()

# Enregistrer les tests avec CTest (sauf en cross-compilation)
if(NOT CMAKE_CROSSCOMPILING)
    include(GoogleTest)
    gtest_discover_tests(client_tests)
endif()

# Définir la sortie des tests dans artifacts/tests
set_target_properties(client_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/tests
)

# Message de configuration
message(STATUS "Client tests configured with ${CMAKE_BUILD_TYPE} mode")
message(STATUS "Test output directory: ${CMAKE_SOURCE_DIR}/artifacts/tests")
