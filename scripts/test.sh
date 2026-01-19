#!/usr/bin/env bash
#===============================================================================
# R-Type Test Runner
#===============================================================================
# Script pour compiler et exécuter les tests unitaires du projet R-Type
#
# Usage:
#   ./scripts/test.sh              # Compile et exécute tous les tests
#   ./scripts/test.sh --server     # Tests serveur uniquement
#   ./scripts/test.sh --client     # Tests client uniquement
#   ./scripts/test.sh --build-only # Compile sans exécuter
#   ./scripts/test.sh --verbose    # Mode verbose avec détails
#   ./scripts/test.sh --filter "HealthTest.*"  # Filtre GTest
#   ./scripts/test.sh --help       # Affiche l'aide
#
# Exemples:
#   ./scripts/test.sh --server --verbose
#   ./scripts/test.sh --filter "Position*"
#   ./scripts/test.sh --client --filter "Signal*"
#===============================================================================

set -e  # Arrêter en cas d'erreur

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color
BOLD='\033[1m'
DIM='\033[2m'

# Répertoires
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# Utiliser le même dossier de build que build.sh (buildLinux par défaut)
PLATFORM="linux"
BUILD_DIR="${PROJECT_ROOT}/buildLinux"
TESTS_OUTPUT_DIR="${PROJECT_ROOT}/artifacts/tests"

# Options par défaut
RUN_SERVER_TESTS=true
RUN_CLIENT_TESTS=true
BUILD_ONLY=false
VERBOSE=false
GTEST_FILTER=""
PARALLEL_JOBS=$(nproc 2>/dev/null || echo 4)

#===============================================================================
# Fonctions utilitaires
#===============================================================================

print_header() {
    echo ""
    echo -e "${BLUE}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}${BOLD}  $1${NC}"
    echo -e "${BLUE}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_info() {
    echo -e "${CYAN}→${NC} $1"
}

print_step() {
    echo -e "${MAGENTA}${BOLD}▶${NC} ${WHITE}$1${NC}"
}

show_help() {
    cat << EOF
${BOLD}R-Type Test Runner${NC}

${BOLD}USAGE:${NC}
    ./scripts/test.sh [OPTIONS]

${BOLD}OPTIONS:${NC}
    -h, --help          Affiche cette aide
    -s, --server        Exécute uniquement les tests serveur
    -c, --client        Exécute uniquement les tests client
    -b, --build-only    Compile les tests sans les exécuter
    -v, --verbose       Mode verbose (affiche tous les détails)
    -f, --filter PATTERN  Filtre les tests avec un pattern GTest
    -j, --jobs N        Nombre de jobs parallèles pour la compilation
    -l, --list          Liste tous les tests disponibles
    -p, --platform P    Plateforme cible: linux (défaut), windows, macos
    --clean             Nettoie le répertoire de build avant

${BOLD}EXEMPLES:${NC}
    ./scripts/test.sh                           # Tous les tests
    ./scripts/test.sh --server                  # Tests serveur uniquement
    ./scripts/test.sh --client --verbose        # Tests client en mode verbose
    ./scripts/test.sh --filter "HealthTest.*"   # Tests Health uniquement
    ./scripts/test.sh --filter "*Position*"     # Tests contenant Position
    ./scripts/test.sh --list                    # Liste les tests
    ./scripts/test.sh --clean --server          # Clean + tests serveur

${BOLD}PATTERNS GTEST:${NC}
    HealthTest.*        Tous les tests de la classe HealthTest
    *Position*          Tous les tests contenant "Position"
    HealthTest.Create*  Tests HealthTest commençant par Create
    -HealthTest.*       Exclure les tests HealthTest

${BOLD}SORTIE:${NC}
    Les binaires de tests sont générés dans: ${TESTS_OUTPUT_DIR}

EOF
}

#===============================================================================
# Parsing des arguments
#===============================================================================

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -s|--server)
                RUN_SERVER_TESTS=true
                RUN_CLIENT_TESTS=false
                shift
                ;;
            -c|--client)
                RUN_SERVER_TESTS=false
                RUN_CLIENT_TESTS=true
                shift
                ;;
            -b|--build-only)
                BUILD_ONLY=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -f|--filter)
                GTEST_FILTER="$2"
                shift 2
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            -l|--list)
                LIST_TESTS=true
                shift
                ;;
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            -p|--platform)
                PLATFORM="$2"
                shift 2
                ;;
            --platform=*)
                PLATFORM="${1#*=}"
                shift
                ;;
            *)
                print_error "Option inconnue: $1"
                echo "Utilisez --help pour voir les options disponibles."
                exit 1
                ;;
        esac
    done

    # Déterminer le dossier de build selon la plateforme
    case $PLATFORM in
        linux)
            BUILD_DIR="${PROJECT_ROOT}/buildLinux"
            ;;
        windows)
            BUILD_DIR="${PROJECT_ROOT}/buildWin"
            ;;
        macos)
            BUILD_DIR="${PROJECT_ROOT}/buildMac"
            ;;
        *)
            print_error "Plateforme invalide: $PLATFORM (linux, windows, macos)"
            exit 1
            ;;
    esac
}

#===============================================================================
# Fonctions principales
#===============================================================================

check_dependencies() {
    print_info "Vérification des dépendances..."

    if ! command -v cmake &> /dev/null; then
        print_error "CMake n'est pas installé"
        exit 1
    fi

    if ! command -v ninja &> /dev/null; then
        print_warning "Ninja n'est pas installé, utilisation de make"
        CMAKE_GENERATOR="Unix Makefiles"
    else
        CMAKE_GENERATOR="Ninja"
    fi

    print_success "Dépendances vérifiées"
}

clean_build() {
    if [[ "$CLEAN_BUILD" == true ]]; then
        print_info "Nettoyage du répertoire de build..."
        rm -rf "$BUILD_DIR"
        print_success "Build nettoyé"
    fi
}

configure_project() {
    print_header "Configuration CMake"

    # Si le build existe déjà avec CMakeCache, réutiliser la configuration existante
    if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
        print_success "Configuration existante trouvée dans ${BUILD_DIR}, réutilisation..."
        return 0
    fi

    print_info "Aucune configuration existante, création..."
    print_warning "Pour de meilleurs résultats, exécutez d'abord: ./scripts/build.sh --platform=${PLATFORM}"

    mkdir -p "$BUILD_DIR"

    # Configuration alignée avec build.sh
    local cmake_args=(
        -S "$PROJECT_ROOT"
        -B "$BUILD_DIR"
        -DCMAKE_BUILD_TYPE=Debug
        -G "$CMAKE_GENERATOR"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        -DCMAKE_CXX_SCAN_FOR_MODULES=OFF
    )

    # Ajouter le triplet vcpkg selon la plateforme
    case $PLATFORM in
        linux)
            cmake_args+=(-DVCPKG_TARGET_TRIPLET=x64-linux)
            cmake_args+=(-DCMAKE_CXX_COMPILER=clang++)
            cmake_args+=(-DCMAKE_C_COMPILER=clang)
            ;;
        windows)
            cmake_args+=(-DVCPKG_TARGET_TRIPLET=x64-mingw-static)
            cmake_args+=(-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++)
            cmake_args+=(-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc)
            ;;
        macos)
            cmake_args+=(-DVCPKG_TARGET_TRIPLET=x64-osx)
            cmake_args+=(-DCMAKE_CXX_COMPILER=clang++)
            cmake_args+=(-DCMAKE_C_COMPILER=clang)
            ;;
    esac

    if [[ -f "${PROJECT_ROOT}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake" ]]; then
        cmake_args+=(-DCMAKE_TOOLCHAIN_FILE="${PROJECT_ROOT}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")
    fi

    # Exécuter CMake (ne pas masquer les erreurs)
    if [[ "$VERBOSE" == true ]]; then
        if ! cmake "${cmake_args[@]}"; then
            print_error "Échec de la configuration CMake"
            exit 1
        fi
    else
        # Capturer la sortie et le code de retour
        local cmake_output
        cmake_output=$(cmake "${cmake_args[@]}" 2>&1) || {
            print_error "Échec de la configuration CMake"
            echo "$cmake_output" | grep -E "(error|Error|ERROR|CMake Error|Could not find)" || echo "$cmake_output"
            exit 1
        }
        # Afficher les lignes importantes même en mode non-verbose
        echo "$cmake_output" | grep -E "(-- Found|-- Configuring done|-- Build files)" || true
    fi

    print_success "Configuration terminée"
}

build_tests() {
    print_header "Compilation des Tests"

    local targets=()

    if [[ "$RUN_SERVER_TESTS" == true ]]; then
        targets+=("server_tests")
    fi

    if [[ "$RUN_CLIENT_TESTS" == true ]]; then
        targets+=("client_tests")
    fi

    for target in "${targets[@]}"; do
        print_info "Compilation de $target..."

        if [[ "$VERBOSE" == true ]]; then
            cmake --build "$BUILD_DIR" --target "$target" -j "$PARALLEL_JOBS"
        else
            cmake --build "$BUILD_DIR" --target "$target" -j "$PARALLEL_JOBS" 2>&1 | grep -E "(error|warning|Building|Linking)" || true
        fi

        if [[ -f "${TESTS_OUTPUT_DIR}/${target}" ]]; then
            print_success "$target compilé avec succès"
        else
            print_error "Échec de compilation de $target"
            exit 1
        fi
    done
}

list_tests() {
    print_header "Tests Disponibles"

    if [[ "$RUN_SERVER_TESTS" == true ]] && [[ -f "${TESTS_OUTPUT_DIR}/server_tests" ]]; then
        echo -e "\n${BOLD}Tests Serveur:${NC}"
        "${TESTS_OUTPUT_DIR}/server_tests" --gtest_list_tests 2>/dev/null || true
    fi

    if [[ "$RUN_CLIENT_TESTS" == true ]] && [[ -f "${TESTS_OUTPUT_DIR}/client_tests" ]]; then
        echo -e "\n${BOLD}Tests Client:${NC}"
        "${TESTS_OUTPUT_DIR}/client_tests" --gtest_list_tests 2>/dev/null || true
    fi
}

run_tests() {
    print_header "Exécution des Tests"

    local gtest_args=("--gtest_color=yes")
    local exit_code=0

    if [[ -n "$GTEST_FILTER" ]]; then
        gtest_args+=("--gtest_filter=$GTEST_FILTER")
        print_info "Filtre appliqué: $GTEST_FILTER"
    fi

    # Tests Serveur
    if [[ "$RUN_SERVER_TESTS" == true ]]; then
        print_header "TESTS SERVEUR"
        echo ""

        if [[ -f "${TESTS_OUTPUT_DIR}/server_tests" ]]; then
            "${TESTS_OUTPUT_DIR}/server_tests" "${gtest_args[@]}" || exit_code=$?
        else
            print_warning "server_tests non trouve - compilation requise"
        fi
    fi

    # Tests Client
    if [[ "$RUN_CLIENT_TESTS" == true ]]; then
        print_header "TESTS CLIENT"
        echo ""

        if [[ -f "${TESTS_OUTPUT_DIR}/client_tests" ]]; then
            "${TESTS_OUTPUT_DIR}/client_tests" "${gtest_args[@]}" || exit_code=$?
        else
            print_warning "client_tests non trouve - compilation requise"
        fi
    fi

    # Résumé final
    print_header "Résumé"

    if [[ $exit_code -eq 0 ]]; then
        echo -e "${GREEN}${BOLD}✓ Tous les tests ont réussi !${NC}"
    else
        echo -e "${RED}${BOLD}✗ Des tests ont échoué (code: $exit_code)${NC}"
    fi

    return $exit_code
}

#===============================================================================
# Main
#===============================================================================

main() {
    parse_args "$@"

    print_header "R-Type Test Runner"
    print_info "Projet: ${PROJECT_ROOT}"
    print_info "Platform: ${PLATFORM}"
    print_info "Build: ${BUILD_DIR}"
    print_info "Output: ${TESTS_OUTPUT_DIR}"

    check_dependencies
    clean_build

    # Créer le répertoire de sortie des tests
    mkdir -p "$TESTS_OUTPUT_DIR"

    configure_project
    build_tests

    if [[ "$LIST_TESTS" == true ]]; then
        list_tests
        exit 0
    fi

    if [[ "$BUILD_ONLY" == true ]]; then
        print_success "Compilation terminée (--build-only)"
        exit 0
    fi

    run_tests
}

main "$@"
