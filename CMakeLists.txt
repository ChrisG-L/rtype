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

# Inclure les sous-répertoires
add_subdirectory("src/server")

# Ajouter des tests pour le serveur
add_subdirectory("tests/server")