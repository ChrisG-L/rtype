#!/bin/bash

# Retourner au répertoire racine du dépôt
cd $(git rev-parse --show-toplevel)

# Installer vcpkg
./scripts/vcpkg/vcpkg.sh install