cmake_minimum_required(VERSION 3.26)

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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/artificats/server)

# Trouver les paquets nécessaires
find_package(Boost REQUIRED COMPONENTS system filesystem)

# Inclure les sous-répertoires
add_subdirectory("src/server")