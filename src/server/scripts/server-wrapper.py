#!/usr/bin/env python3
"""
R-Type Server Wrapper
Gère le serveur dans tmux avec propagation des exit codes et notifications Discord.

Usage:
    python3 server-wrapper.py [--server-path /path/to/rtype_server]

Pour se connecter au TUI (n'importe quel utilisateur):
    tmux -S /tmp/rtype-tmux.sock attach -t rtype

Pour détacher (laisser tourner):
    Ctrl+B puis D
"""

import subprocess
import sys
import time
import json
import argparse
import signal
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional
import urllib.request
import urllib.error

# =============================================================================
# Configuration
# =============================================================================

TMUX_SESSION_NAME = "rtype"
TMUX_SOCKET_PATH = "/tmp/rtype-tmux.sock"  # Socket partagé pour accès multi-utilisateur
DEFAULT_SERVER_PATH = "/opt/rtype/server/rtype_server"
ERROR_LOG_PATH = "/opt/rtype/logs/server-error.log"
LOCK_FILE_PATH = "/tmp/rtype-server.lock"

# Discord Webhook (from environment variable for security)
DISCORD_WEBHOOK_URL = os.environ.get("DISCORD_WEBHOOK_URL", "")

# Couleurs Discord (décimal)
COLOR_GREEN = 3066993
COLOR_RED = 15158332
COLOR_YELLOW = 16776960
COLOR_GRAY = 9807270

# =============================================================================
# Discord Notifications
# =============================================================================

def notify_discord(title: str, message: str, color: int = COLOR_GREEN) -> bool:
    """Envoie une notification Discord via webhook."""
    if not DISCORD_WEBHOOK_URL:
        log("DISCORD_WEBHOOK_URL non configuré, notification ignorée")
        return False

    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    payload = {
        "embeds": [{
            "title": title,
            "description": message,
            "color": color,
            "timestamp": timestamp,
            "footer": {
                "text": "VPS 51.254.137.175"
            }
        }]
    }

    try:
        data = json.dumps(payload).encode('utf-8')
        req = urllib.request.Request(
            DISCORD_WEBHOOK_URL,
            data=data,
            headers={
                "Content-Type": "application/json",
                "User-Agent": "RType-Server/1.0"
            },
            method="POST"
        )
        with urllib.request.urlopen(req, timeout=10) as response:
            success = response.status in (200, 204)
            if success:
                log(f"Notification Discord envoyée: {title}")
            else:
                log(f"Discord a répondu avec status {response.status}")
            return success
    except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError) as e:
        log(f"Erreur notification Discord: {e}")
        return False
    except Exception as e:
        log(f"Erreur inattendue notification Discord: {type(e).__name__}: {e}")
        return False


def notify_start():
    """Notification de démarrage."""
    notify_discord(
        title="🟢 Serveur démarré",
        message=(
            "Le serveur R-Type est maintenant **en ligne**\n\n"
            "**Port Game:** UDP 4124\n"
            "**Port Auth:** TCP 4125\n"
            "**Port Voice:** UDP 4126"
        ),
        color=COLOR_GREEN
    )


def notify_stop():
    """Notification d'arrêt propre."""
    notify_discord(
        title="🔴 Serveur arrêté",
        message="Le serveur R-Type a été **arrêté proprement**",
        color=COLOR_GRAY
    )


def notify_crash(exit_code: int):
    """Notification de crash avec logs d'erreur."""
    last_errors = get_last_errors()

    notify_discord(
        title="💥 CRASH SERVEUR",
        message=(
            f"Le serveur R-Type a **crashé** (code: {exit_code})\n\n"
            f"**Dernières erreurs:**\n```\n{last_errors}\n```\n\n"
            "⚠️ Redémarrage automatique en cours..."
        ),
        color=COLOR_RED
    )


def get_last_errors(lines: int = 20, max_chars: int = 1500) -> str:
    """Récupère les dernières lignes du log d'erreur."""
    try:
        error_log = Path(ERROR_LOG_PATH)
        if not error_log.exists():
            return "Pas de logs d'erreur disponibles"

        with open(error_log, 'r') as f:
            all_lines = f.readlines()
            last_lines = all_lines[-lines:] if len(all_lines) >= lines else all_lines
            content = ''.join(last_lines)
            return content[:max_chars] if len(content) > max_chars else content
    except Exception as e:
        return f"Erreur lecture logs: {e}"


# =============================================================================
# Lock File (Anti multi-instance)
# =============================================================================

def acquire_lock() -> bool:
    """Tente d'acquérir le lock. Retourne False si une instance existe déjà."""
    lock_path = Path(LOCK_FILE_PATH)

    if lock_path.exists():
        # Vérifie si le PID dans le lock est encore actif
        try:
            pid = int(lock_path.read_text().strip())
            # Vérifie si le processus existe
            subprocess.run(["kill", "-0", str(pid)], check=True, capture_output=True)
            # Le processus existe encore
            log(f"Une instance est déjà en cours (PID {pid})")
            return False
        except (ValueError, subprocess.CalledProcessError):
            # PID invalide ou processus mort, on peut prendre le lock
            pass

    # Écrit notre PID
    lock_path.write_text(str(subprocess.os.getpid()))
    return True


def release_lock():
    """Libère le lock et nettoie le socket tmux."""
    try:
        Path(LOCK_FILE_PATH).unlink(missing_ok=True)
        Path(TMUX_SOCKET_PATH).unlink(missing_ok=True)
    except Exception:
        pass


# =============================================================================
# Tmux Management
# =============================================================================

def session_exists() -> bool:
    """Vérifie si la session tmux existe."""
    result = subprocess.run(
        ["tmux", "-S", TMUX_SOCKET_PATH, "has-session", "-t", TMUX_SESSION_NAME],
        capture_output=True
    )
    return result.returncode == 0


def kill_existing_session():
    """Tue une session tmux existante."""
    if session_exists():
        log(f"Session tmux '{TMUX_SESSION_NAME}' existante, suppression...")
        subprocess.run(
            ["tmux", "-S", TMUX_SOCKET_PATH, "kill-session", "-t", TMUX_SESSION_NAME],
            capture_output=True
        )
        time.sleep(0.5)


def start_server_in_tmux(server_path: str) -> bool:
    """
    Lance le serveur dans tmux.
    La commande écrit l'exit code dans un fichier temporaire.
    """
    exit_code_file = f"/tmp/rtype_exit_code_{subprocess.os.getpid()}"

    # Commande qui lance le serveur et capture son exit code
    # Utilise systemd-cat pour envoyer les logs à journald
    tmux_command = (
        f"{server_path} 2>&1 | tee >(systemd-cat -t rtype-server); "
        f"echo ${{PIPESTATUS[0]}} > {exit_code_file}"
    )

    result = subprocess.run([
        "tmux", "-S", TMUX_SOCKET_PATH, "new-session", "-d", "-s", TMUX_SESSION_NAME,
        "bash", "-c", tmux_command
    ], capture_output=True)

    # Rendre le socket accessible à tous les utilisateurs
    try:
        import os
        os.chmod(TMUX_SOCKET_PATH, 0o777)
    except Exception:
        pass

    if result.returncode != 0:
        log(f"Erreur création session tmux: {result.stderr.decode()}")
        return False

    log(f"Serveur lancé dans tmux session '{TMUX_SESSION_NAME}'")
    return True


def wait_for_session_end() -> int:
    """
    Attend que la session tmux se termine.
    Retourne l'exit code du serveur.
    """
    exit_code_file = f"/tmp/rtype_exit_code_{subprocess.os.getpid()}"

    while session_exists():
        time.sleep(1)

    # Récupère l'exit code
    try:
        exit_code = int(Path(exit_code_file).read_text().strip())
        Path(exit_code_file).unlink(missing_ok=True)
        return exit_code
    except (FileNotFoundError, ValueError):
        log("Impossible de récupérer l'exit code, assume crash")
        return 1


# =============================================================================
# Signal Handlers
# =============================================================================

def setup_signal_handlers():
    """Configure les handlers de signaux pour un arrêt propre."""
    def handler(signum, frame):
        log(f"Signal {signum} reçu, arrêt du serveur...")
        if session_exists():
            # Envoie SIGTERM au serveur dans tmux
            subprocess.run([
                "tmux", "-S", TMUX_SOCKET_PATH, "send-keys", "-t", TMUX_SESSION_NAME, "C-c"
            ], capture_output=True)
            time.sleep(2)
            kill_existing_session()

        # Notification d'arrêt propre (systemctl stop)
        notify_stop()

        release_lock()
        sys.exit(0)

    signal.signal(signal.SIGTERM, handler)
    signal.signal(signal.SIGINT, handler)


# =============================================================================
# Logging
# =============================================================================

def log(message: str):
    """Log vers stderr (capturé par journald via systemd)."""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] [wrapper] {message}", file=sys.stderr)


# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description="R-Type Server Wrapper")
    parser.add_argument(
        "--server-path",
        default=DEFAULT_SERVER_PATH,
        help=f"Chemin vers l'exécutable du serveur (défaut: {DEFAULT_SERVER_PATH})"
    )
    args = parser.parse_args()

    # Vérifie que le serveur existe
    server_path = Path(args.server_path)
    if not server_path.exists():
        log(f"Erreur: serveur non trouvé: {server_path}")
        sys.exit(1)

    # Anti multi-instance
    if not acquire_lock():
        log("Erreur: une instance du serveur est déjà en cours")
        sys.exit(1)

    try:
        setup_signal_handlers()

        # Nettoie une éventuelle session orpheline
        kill_existing_session()

        # Lance le serveur
        if not start_server_in_tmux(str(server_path)):
            release_lock()
            sys.exit(1)

        # Notification de démarrage
        notify_start()

        # Attend la fin
        exit_code = wait_for_session_end()
        log(f"Serveur terminé avec code: {exit_code}")

        # Notification selon le résultat
        if exit_code == 0:
            notify_stop()
        else:
            notify_crash(exit_code)

        # Propage l'exit code à systemd
        sys.exit(exit_code)

    finally:
        release_lock()


if __name__ == "__main__":
    main()
