#!/bin/bash

set -e

cd "$(git rev-parse --show-toplevel)" # Retour a la racine du projet

# Création de la structure des dossiers pour l'architecture hexagonale
mkdir -p src/server/domain/{entities,value_objects,services,exceptions} \
         src/server/application/{ports/{in,out/persistence},use_cases,dto} \
         src/server/infrastructure/{adapters/{in/cli,out/persistence},configuration}