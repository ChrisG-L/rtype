#!/bin/bash

# Revenir à la racine du projet
cd $(git rev-parse --show-toplevel)

# Aller dans le dossier third_party
mkdir -p third_party
cd third_party

# Script d'installation de vcpkg
git clone https://github.com/microsoft/vcpkg.git

# Aller dans le dossier vcpkg
cd vcpkg

# Exécuter le script d'installation
./bootstrap-vcpkg.sh

echo "vcpkg installé avec succès."

# Faire un submodule de vcpkg
cd ../..
git submodule add https://github.com/microsoft/vcpkg.git third_party/vcpkg

# Retourner à la racine du projet
cd $(git rev-parse --show-toplevel)

# Initialiser et mettre à jour les submodules
echo "vcpkg installé et ajouté en tant que submodule."