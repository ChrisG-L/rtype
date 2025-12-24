#!/usr/bin/env bash

set -e # Arreter en cas d'erreur

# Nécessaire pour vcpkg lors de la reconfiguration automatique de CMake
export VCPKG_FORCE_SYSTEM_BINARIES=1

# ==============================================================================
# Couleurs et Formatage
# ==============================================================================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m' # No Color

# Compteurs d'erreurs/warnings
ERROR_COUNT=0
WARNING_COUNT=0

# ==============================================================================
# Fonctions d'affichage
# ==============================================================================
print_header() {
    echo ""
    echo -e "${BLUE}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}${BOLD}  $1${NC}"
    echo -e "${BLUE}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_success() {
    echo -e "${GREEN}${BOLD}✓${NC} ${GREEN}$1${NC}"
}

print_error() {
    echo -e "${RED}${BOLD}✗${NC} ${RED}$1${NC}"
}

print_warning() {
    echo -e "${YELLOW}${BOLD}!${NC} ${YELLOW}$1${NC}"
}

print_info() {
    echo -e "${CYAN}→${NC} $1"
}

print_step() {
    echo -e "${MAGENTA}${BOLD}▶${NC} ${WHITE}$1${NC}"
}

# ==============================================================================
# Fonction de filtrage des erreurs de compilation
# ==============================================================================
filter_build_output() {
    local temp_file=$(mktemp)
    local in_error=0
    local error_buffer=""

    while IFS= read -r line; do
        # Detecter les erreurs (remplacement de \d par [0-9])
        if echo "$line" | grep -qE "^.*:[0-9]+:[0-9]+: error:"; then
            in_error=1
            ERROR_COUNT=$((ERROR_COUNT + 1))
            echo -e "${RED}${BOLD}ERROR${NC} $line" >> "$temp_file"
        # Detecter les warnings (remplacement de \d par [0-9])
        elif echo "$line" | grep -qE "^.*:[0-9]+:[0-9]+: warning:"; then
            WARNING_COUNT=$((WARNING_COUNT + 1))
            echo -e "${YELLOW}WARN${NC}  $line" >> "$temp_file"
        # Detecter les notes (contexte des erreurs) (remplacement de \d par [0-9])
        elif echo "$line" | grep -qE "^.*:[0-9]+:[0-9]+: note:"; then
            echo -e "${DIM}      $line${NC}" >> "$temp_file"
        # Detecter "In file included from"
        elif echo "$line" | grep -qE "^In file included from"; then
            echo -e "${DIM}      $line${NC}" >> "$temp_file"
        # Detecter les lignes de code avec ^ (remplacement de \s par [[:space:]] par précaution)
        elif echo "$line" | grep -qE "^[[:space:]]+\^"; then
            echo -e "${CYAN}      $line${NC}" >> "$temp_file"
        # Detecter les erreurs fatales
        elif echo "$line" | grep -qE "fatal error:|FAILED:"; then
            ERROR_COUNT=$((ERROR_COUNT + 1))
            echo -e "${RED}${BOLD}FATAL${NC} $line" >> "$temp_file"
        # Detecter ninja build progress (remplacement de \d par [0-9])
        elif echo "$line" | grep -qE "^\[[0-9]+/[0-9]+\]"; then
            # Afficher en temps reel le progres
            echo -ne "\r${DIM}$line${NC}                    "
        # Detecter undefined reference (linker)
        elif echo "$line" | grep -qE "undefined reference to"; then
            ERROR_COUNT=$((ERROR_COUNT + 1))
            echo -e "${RED}${BOLD}LINK${NC}  $line" >> "$temp_file"
        # Ignorer les lignes vides et le bruit
        elif [[ -n "$line" ]] && ! echo "$line" | grep -qE "^(ninja:|cmake|-- |In member function)"; then
            # Garder les autres lignes pertinentes
            if echo "$line" | grep -qE "(error|Error|ERROR)"; then
                echo -e "${RED}$line${NC}" >> "$temp_file"
            elif echo "$line" | grep -qE "(warning|Warning|WARNING)"; then
                echo -e "${YELLOW}$line${NC}" >> "$temp_file"
            fi
        fi
    done

    echo ""  # Nouvelle ligne apres le progres
    cat "$temp_file"
    rm -f "$temp_file"
}

# ==============================================================================
# Fonction pour afficher le resume
# ==============================================================================
print_summary() {
    echo ""
    echo -e "${BLUE}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}${BOLD}  RESUME DE LA COMPILATION${NC}"
    echo -e "${BLUE}${BOLD}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    if [[ $ERROR_COUNT -eq 0 ]] && [[ $WARNING_COUNT -eq 0 ]]; then
        echo -e "  ${GREEN}${BOLD}✓ Compilation reussie sans erreur ni warning${NC}"
    else
        if [[ $ERROR_COUNT -gt 0 ]]; then
            echo -e "  ${RED}${BOLD}✗ Erreurs:   $ERROR_COUNT${NC}"
        fi
        if [[ $WARNING_COUNT -gt 0 ]]; then
            echo -e "  ${YELLOW}${BOLD}! Warnings:  $WARNING_COUNT${NC}"
        fi
    fi
    echo ""
}

# ==============================================================================
# Parse arguments
# ==============================================================================
PLATFORM="linux"
LAUNCH_MODE="server"
VERBOSE=0

for arg in "$@"; do
    case $arg in
        --platform=*)
            PLATFORM="${arg#*=}"
            shift
            ;;
        --client)
            LAUNCH_MODE="client"
            shift
            ;;
        --server)
            LAUNCH_MODE="server"
            shift
            ;;
        --both)
            LAUNCH_MODE="both"
            shift
            ;;
        --no-launch)
            LAUNCH_MODE="none"
            shift
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --platform=PLATFORM   linux, windows, macos (default: linux)"
            echo "  --server              Lancer le serveur (default)"
            echo "  --client              Lancer le client"
            echo "  --both                Lancer serveur + client"
            echo "  --no-launch           Compiler sans lancer"
            echo "  --verbose, -v         Afficher la sortie complete"
            echo "  --help, -h            Afficher cette aide"
            exit 0
            ;;
        *)
            ;;
    esac
done

# Validate platform
if [[ "$PLATFORM" != "linux" && "$PLATFORM" != "windows" && "$PLATFORM" != "macos" ]]; then
    print_error "Platform invalide '$PLATFORM'. Utilisez: linux, windows, ou macos"
    exit 1
fi

# Determiner le dossier de build selon la plateforme
case $PLATFORM in
    linux)
        BUILD_DIR="buildLinux"
        SERVER_PATH="./artifacts/server/linux/rtype_server"
        CLIENT_PATH="./artifacts/client/linux/rtype_client"
        ;;
    windows)
        BUILD_DIR="buildWin"
        SERVER_PATH="./artifacts/server/windows/rtype_server.exe"
        CLIENT_PATH="./artifacts/client/windows/rtype_client.exe"
        ;;
    macos)
        BUILD_DIR="buildMac"
        SERVER_PATH="./artifacts/server/macos/rtype_server"
        CLIENT_PATH="./artifacts/client/macos/rtype_client"
        ;;
    *)
        print_error "Plateforme non supportee: $PLATFORM"
        exit 1
        ;;
esac

cd "$(git rev-parse --show-toplevel)"

# Verifier que le dossier de build existe
if [[ ! -d "$BUILD_DIR" ]]; then
    print_error "Le dossier $BUILD_DIR n'existe pas."
    print_info "Veuillez d'abord executer: ./scripts/build.sh --platform=$PLATFORM"
    exit 1
fi

# ==============================================================================
# Compilation
# ==============================================================================
print_header "COMPILATION R-TYPE"
print_info "Plateforme: ${BOLD}$PLATFORM${NC}"
print_info "Build dir:  ${BOLD}$BUILD_DIR${NC}"
print_info "Mode:       ${BOLD}$LAUNCH_MODE${NC}"
echo ""

print_step "Compilation en cours..."

# Compilation avec filtrage
if [[ $VERBOSE -eq 1 ]]; then
    # Mode verbose: afficher tout
    cmake --build "$BUILD_DIR" --config Debug 2>&1
    BUILD_STATUS=$?
else
    # Mode normal: filtrer la sortie
    cmake --build "$BUILD_DIR" --config Debug 2>&1 | filter_build_output
    BUILD_STATUS=${PIPESTATUS[0]}
fi

# Afficher le resume
print_summary

# Verifier le statut de la compilation
if [[ $BUILD_STATUS -ne 0 ]]; then
    print_error "La compilation a echoue"
    exit 1
fi

print_success "Compilation terminee"

# ==============================================================================
# Lancement
# ==============================================================================
SANITIZER_ENV="LSAN_OPTIONS=suppressions=$(pwd)/lsan.supp"

kill_previous() {
    if pgrep -f "rtype_server" > /dev/null 2>&1; then
        print_info "Arret du serveur precedent..."
        pkill -f "rtype_server" 2>/dev/null || true
        sleep 0.5
    fi
}

if [[ "$PLATFORM" == "linux" || "$PLATFORM" == "macos" ]]; then
    case $LAUNCH_MODE in
        server)
            kill_previous
            print_header "LANCEMENT SERVEUR"
            env $SANITIZER_ENV "$SERVER_PATH"
            ;;
        client)
            print_header "LANCEMENT CLIENT"
            env $SANITIZER_ENV "$CLIENT_PATH"
            ;;
        both)
            kill_previous
            print_header "LANCEMENT SERVEUR + CLIENT"
            print_info "Demarrage du serveur en arriere-plan..."
            env $SANITIZER_ENV "$SERVER_PATH" &
            sleep 1
            print_info "Demarrage du client..."
            env $SANITIZER_ENV "$CLIENT_PATH"
            ;;
        none)
            print_info "Aucun lancement (--no-launch)"
            print_info "Serveur: $SERVER_PATH"
            print_info "Client:  $CLIENT_PATH"
            ;;
    esac
else
    print_warning "Lancement automatique non disponible pour $PLATFORM"
    print_info "Serveur: $SERVER_PATH"
    print_info "Client:  $CLIENT_PATH"
fi
