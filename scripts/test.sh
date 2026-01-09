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

#===============================================================================
# Filtrage de la sortie GTest pour une meilleure lisibilite
#===============================================================================
filter_gtest_output() {
    local passed=0
    local failed=0

    while IFS= read -r line; do
        # Tests qui passent
        if echo "$line" | grep -qE "^\[       OK \]"; then
            passed=$((passed + 1))
            echo -e "${GREEN}$line${NC}"
        # Tests qui echouent
        elif echo "$line" | grep -qE "^\[  FAILED  \]"; then
            failed=$((failed + 1))
            echo -e "${RED}${BOLD}$line${NC}"
        # En cours d'execution
        elif echo "$line" | grep -qE "^\[ RUN      \]"; then
            echo -e "${CYAN}$line${NC}"
        # Separateurs
        elif echo "$line" | grep -qE "^\[==========\]"; then
            echo -e "${BLUE}${BOLD}$line${NC}"
        elif echo "$line" | grep -qE "^\[----------\]"; then
            echo -e "${DIM}$line${NC}"
        # Resume passes
        elif echo "$line" | grep -qE "^\[  PASSED  \]"; then
            echo -e "${GREEN}${BOLD}$line${NC}"
        # Erreurs d'assertion
        elif echo "$line" | grep -qE "Failure$|FAILED"; then
            echo -e "${RED}${BOLD}$line${NC}"
        # Expected/Actual values
        elif echo "$line" | grep -qE "^\s*(Expected|Actual|Value of|Which is):"; then
            echo -e "${YELLOW}$line${NC}"
        # Lignes de code source avec erreur
        elif echo "$line" | grep -qE "^.*:\d+:"; then
            echo -e "${RED}$line${NC}"
        # Global test environment
        elif echo "$line" | grep -qE "Global test environment"; then
            echo -e "${DIM}$line${NC}"
        # Autres lignes
        else
            echo "$line"
        fi
    done

    # Retourner le statut
    if [[ $failed -gt 0 ]]; then
        return 1
    fi
    return 0
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

    mkdir -p "$BUILD_DIR"

    local cmake_args=(
        -S "$PROJECT_ROOT"
        -B "$BUILD_DIR"
        -DCMAKE_BUILD_TYPE=Debug
        -G "$CMAKE_GENERATOR"
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )

    if [[ -f "${PROJECT_ROOT}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake" ]]; then
        cmake_args+=(-DCMAKE_TOOLCHAIN_FILE="${PROJECT_ROOT}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")
    fi

    if [[ "$VERBOSE" == true ]]; then
        cmake "${cmake_args[@]}"
    else
        cmake "${cmake_args[@]}" > /dev/null 2>&1
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

    local gtest_args=()

    if [[ -n "$GTEST_FILTER" ]]; then
        gtest_args+=("--gtest_filter=$GTEST_FILTER")
        print_info "Filtre appliqué: $GTEST_FILTER"
    fi

    if [[ "$VERBOSE" == true ]]; then
        gtest_args+=("--gtest_print_time=1")
    fi

    local total_passed=0
    local total_failed=0
    local total_tests=0

    # Tests Serveur
    if [[ "$RUN_SERVER_TESTS" == true ]]; then
        print_header "TESTS SERVEUR"
        echo ""

        if [[ -f "${TESTS_OUTPUT_DIR}/server_tests" ]]; then
            if [[ "$VERBOSE" == true ]]; then
                if "${TESTS_OUTPUT_DIR}/server_tests" "${gtest_args[@]}" --gtest_color=yes; then
                    print_success "Tests serveur reussis"
                else
                    print_error "Certains tests serveur ont echoue"
                    total_failed=$((total_failed + 1))
                fi
            else
                if "${TESTS_OUTPUT_DIR}/server_tests" "${gtest_args[@]}" --gtest_color=no 2>&1 | filter_gtest_output; then
                    print_success "Tests serveur reussis"
                else
                    print_error "Certains tests serveur ont echoue"
                    total_failed=$((total_failed + 1))
                fi
            fi
        else
            print_warning "server_tests non trouve - compilation requise"
        fi
    fi

    # Tests Client
    if [[ "$RUN_CLIENT_TESTS" == true ]]; then
        print_header "TESTS CLIENT"
        echo ""

        if [[ -f "${TESTS_OUTPUT_DIR}/client_tests" ]]; then
            if [[ "$VERBOSE" == true ]]; then
                if "${TESTS_OUTPUT_DIR}/client_tests" "${gtest_args[@]}" --gtest_color=yes; then
                    print_success "Tests client reussis"
                else
                    print_error "Certains tests client ont echoue"
                    total_failed=$((total_failed + 1))
                fi
            else
                if "${TESTS_OUTPUT_DIR}/client_tests" "${gtest_args[@]}" --gtest_color=no 2>&1 | filter_gtest_output; then
                    print_success "Tests client reussis"
                else
                    print_error "Certains tests client ont echoue"
                    total_failed=$((total_failed + 1))
                fi
            fi
        else
            print_warning "client_tests non trouve - compilation requise"
        fi
    fi

    # Résumé final
    print_header "Résumé"

    if [[ $total_failed -eq 0 ]]; then
        echo -e "${GREEN}${BOLD}✓ Tous les tests ont réussi !${NC}"
        return 0
    else
        echo -e "${RED}${BOLD}✗ Des tests ont échoué${NC}"
        return 1
    fi
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
