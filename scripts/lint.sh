#!/usr/bin/env bash
#===============================================================================
# R-Type Linter - clang-tidy
#===============================================================================
# Script pour analyser le code avec clang-tidy
#
# Usage:
#   ./scripts/lint.sh              # Analyse tout le projet
#   ./scripts/lint.sh --fix        # Analyse et corrige automatiquement
#   ./scripts/lint.sh --server     # Analyse uniquement le serveur
#   ./scripts/lint.sh --client     # Analyse uniquement le client
#   ./scripts/lint.sh --help       # Affiche l'aide
#===============================================================================

set -e

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Répertoires
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/buildLinux"

# Options par défaut
FIX_MODE=false
LINT_SERVER=true
LINT_CLIENT=true
VERBOSE=false

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

show_help() {
    cat << EOF
${BOLD}R-Type Linter (clang-tidy)${NC}

${BOLD}USAGE:${NC}
    ./scripts/lint.sh [OPTIONS]

${BOLD}OPTIONS:${NC}
    -h, --help      Affiche cette aide
    -f, --fix       Mode correction automatique
    -s, --server    Analyse uniquement le serveur
    -c, --client    Analyse uniquement le client
    -v, --verbose   Mode verbose

${BOLD}EXEMPLES:${NC}
    ./scripts/lint.sh                  # Analyse tout
    ./scripts/lint.sh --fix            # Analyse et corrige
    ./scripts/lint.sh --server --fix   # Corrige uniquement le serveur

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
            -f|--fix)
                FIX_MODE=true
                shift
                ;;
            -s|--server)
                LINT_SERVER=true
                LINT_CLIENT=false
                shift
                ;;
            -c|--client)
                LINT_SERVER=false
                LINT_CLIENT=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            *)
                print_error "Option inconnue: $1"
                echo "Utilisez --help pour voir les options disponibles."
                exit 1
                ;;
        esac
    done
}

#===============================================================================
# Fonctions principales
#===============================================================================

check_dependencies() {
    print_info "Vérification des dépendances..."

    if ! command -v clang-tidy &> /dev/null; then
        print_error "clang-tidy n'est pas installé"
        print_info "Installation: sudo apt install clang-tidy"
        exit 1
    fi

    local version=$(clang-tidy --version | head -1)
    print_success "clang-tidy trouvé: $version"
}

ensure_compile_commands() {
    print_info "Vérification de compile_commands.json..."

    if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
        print_warning "compile_commands.json non trouvé, configuration CMake..."

        mkdir -p "$BUILD_DIR"
        cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -G Ninja > /dev/null 2>&1

        if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
            print_error "Impossible de générer compile_commands.json"
            exit 1
        fi
    fi

    print_success "compile_commands.json disponible"
}

run_clang_tidy() {
    local component=$1
    local source_dir=$2

    print_header "Analyse $component"

    local files=()
    while IFS= read -r -d '' file; do
        files+=("$file")
    done < <(find "$source_dir" -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 2>/dev/null)

    if [[ ${#files[@]} -eq 0 ]]; then
        print_warning "Aucun fichier trouvé dans $source_dir"
        return 0
    fi

    print_info "Analyse de ${#files[@]} fichiers..."

    local clang_args=("-p" "$BUILD_DIR")

    if [[ "$FIX_MODE" == true ]]; then
        clang_args+=("--fix")
        print_info "Mode correction automatique activé"
    fi

    local error_count=0
    local warning_count=0

    for file in "${files[@]}"; do
        local rel_path="${file#$PROJECT_ROOT/}"

        if [[ "$VERBOSE" == true ]]; then
            print_info "Analyse: $rel_path"
        fi

        local output
        if output=$(clang-tidy "${clang_args[@]}" "$file" 2>&1); then
            if [[ -n "$output" ]] && [[ "$output" != *"No relevant diagnostics"* ]]; then
                echo "$output"
                warning_count=$((warning_count + 1))
            fi
        else
            echo "$output"
            error_count=$((error_count + 1))
        fi
    done

    if [[ $error_count -eq 0 ]] && [[ $warning_count -eq 0 ]]; then
        print_success "$component: Aucun problème détecté"
    else
        print_warning "$component: $warning_count warning(s), $error_count erreur(s)"
    fi

    return $error_count
}

#===============================================================================
# Main
#===============================================================================

main() {
    parse_args "$@"

    print_header "R-Type Linter (clang-tidy)"
    print_info "Projet: ${PROJECT_ROOT}"

    check_dependencies
    ensure_compile_commands

    local total_errors=0

    if [[ "$LINT_SERVER" == true ]]; then
        if ! run_clang_tidy "Serveur" "${PROJECT_ROOT}/src/server"; then
            total_errors=$((total_errors + 1))
        fi
    fi

    if [[ "$LINT_CLIENT" == true ]]; then
        if ! run_clang_tidy "Client" "${PROJECT_ROOT}/src/client"; then
            total_errors=$((total_errors + 1))
        fi
    fi

    # Résumé final
    print_header "Résumé"

    if [[ $total_errors -eq 0 ]]; then
        echo -e "${GREEN}${BOLD}✓ Analyse terminée sans erreurs critiques${NC}"
        exit 0
    else
        echo -e "${YELLOW}${BOLD}⚠ Analyse terminée avec des avertissements${NC}"
        exit 0  # Ne pas échouer le build pour des warnings
    fi
}

main "$@"
