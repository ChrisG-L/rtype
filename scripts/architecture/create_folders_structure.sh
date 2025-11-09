#!/bin/bash

set -e

cd "$(git rev-parse --show-toplevel)" # Retour a la racine du projet

# Création de la structure des dossiers pour l'architecture hexagonale
mkdir -p src/server/include/domain/{entities,value_objects,services,exceptions} \
         src/server/include/application/{ports/{in,out/persistence},use_cases,dto} \
         src/server/include/infrastructure/{adapters/{in/cli,out/persistence},configuration}