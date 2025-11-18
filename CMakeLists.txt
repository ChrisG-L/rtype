cmake_minimum_required(VERSION 3.30)

project("R-Type" VERSION 0.0.1 LANGUAGES CXX)

# Mettre le standard C++ à utiliser à C++23
set(CMAKE_CXX_STANDARD 23)
# Exiger l'utilisation de ce standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Ne pas activer les extensions spécifiques au compilateur
set(CMAKE_CXX_EXTENSIONS OFF)

# Inclure vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# Définir l'emplacement des binaires
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/artifacts/server/linux)

# Autoriser les tests
enable_testing()

# Trouver les paquets nécessaires
find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(GTest CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Network)

add_library(rtype_proto STATIC)

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/user.proto
)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

message(STATUS "Protobuf sources générées : ${PROTO_SRCS}")
message(STATUS "Protobuf headers générés : ${PROTO_HDRS}")

target_sources(rtype_proto PRIVATE
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_include_directories(rtype_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(rtype_proto PRIVATE
    ${PROTOBUF_LIBRARIES}
)

# Inclure les sous-répertoires
add_subdirectory("src/server")
add_subdirectory("src/client")

# Ajouter des tests pour le serveur
add_subdirectory("tests/server")
