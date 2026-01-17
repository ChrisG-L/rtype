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
STATE_FILE_PATH = "/opt/rtype/logs/server-state.json"  # Pour tracking uptime et crashes

# Discord Webhook (from environment variable for security)
DISCORD_WEBHOOK_URL = os.environ.get("DISCORD_WEBHOOK_URL", "")

# GitHub repo pour les liens
GITHUB_REPO = "https://github.com/Pluenet-Killian/rtype"

# Couleurs Discord (décimal)
COLOR_GREEN = 3066993
COLOR_RED = 15158332
COLOR_YELLOW = 16776960
COLOR_GRAY = 9807270

# =============================================================================
# State Management (uptime & crash tracking)
# =============================================================================

def load_state() -> dict:
    """Charge l'état persistant du serveur."""
    try:
        state_file = Path(STATE_FILE_PATH)
        if state_file.exists():
            with open(state_file, 'r') as f:
                return json.load(f)
    except Exception as e:
        log(f"Erreur lecture state: {e}")
    return {"crashes": [], "last_start": None, "total_uptime_seconds": 0}


def save_state(state: dict):
    """Sauvegarde l'état persistant du serveur."""
    try:
        state_file = Path(STATE_FILE_PATH)
        state_file.parent.mkdir(parents=True, exist_ok=True)
        with open(state_file, 'w') as f:
            json.dump(state, f, indent=2)
    except Exception as e:
        log(f"Erreur sauvegarde state: {e}")


def record_start():
    """Enregistre le démarrage du serveur."""
    state = load_state()
    state["last_start"] = datetime.now(timezone.utc).isoformat()
    save_state(state)


def record_stop() -> Optional[str]:
    """Enregistre l'arrêt et retourne l'uptime formaté."""
    state = load_state()
    uptime_str = None
    if state.get("last_start"):
        try:
            start_time = datetime.fromisoformat(state["last_start"])
            uptime = datetime.now(timezone.utc) - start_time
            state["total_uptime_seconds"] = state.get("total_uptime_seconds", 0) + uptime.total_seconds()
            uptime_str = format_duration(uptime.total_seconds())
        except Exception:
            pass
    state["last_start"] = None
    save_state(state)
    return uptime_str


def record_crash(exit_code: int) -> tuple[Optional[str], Optional[str]]:
    """Enregistre un crash et retourne (uptime, last_crash_ago)."""
    state = load_state()
    uptime_str = None
    last_crash_ago = None

    # Calculer uptime avant crash
    if state.get("last_start"):
        try:
            start_time = datetime.fromisoformat(state["last_start"])
            uptime = datetime.now(timezone.utc) - start_time
            uptime_str = format_duration(uptime.total_seconds())
        except Exception:
            pass

    # Trouver le dernier crash
    if state.get("crashes"):
        try:
            last_crash = datetime.fromisoformat(state["crashes"][-1]["timestamp"])
            ago = datetime.now(timezone.utc) - last_crash
            last_crash_ago = format_duration(ago.total_seconds())
        except Exception:
            pass

    # Enregistrer ce crash
    state["crashes"].append({
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "exit_code": exit_code,
        "uptime": uptime_str
    })
    # Garder seulement les 10 derniers crashes
    state["crashes"] = state["crashes"][-10:]
    state["last_start"] = None
    save_state(state)

    return uptime_str, last_crash_ago


def get_last_crash_info() -> Optional[str]:
    """Retourne le temps depuis le dernier crash."""
    state = load_state()
    if state.get("crashes"):
        try:
            last_crash = datetime.fromisoformat(state["crashes"][-1]["timestamp"])
            ago = datetime.now(timezone.utc) - last_crash
            return format_duration(ago.total_seconds())
        except Exception:
            pass
    return None


def format_duration(seconds: float) -> str:
    """Formate une durée en texte lisible."""
    seconds = int(seconds)
    if seconds < 60:
        return f"{seconds}s"
    elif seconds < 3600:
        minutes = seconds // 60
        secs = seconds % 60
        return f"{minutes}m {secs}s"
    elif seconds < 86400:
        hours = seconds // 3600
        minutes = (seconds % 3600) // 60
        return f"{hours}h {minutes}m"
    else:
        days = seconds // 86400
        hours = (seconds % 86400) // 3600
        return f"{days}j {hours}h"


# =============================================================================
# Discord Notifications
# =============================================================================

def notify_discord(title: str, message: str, color: int = COLOR_GREEN, fields: list = None, thumbnail: str = None) -> bool:
    """Envoie une notification Discord via webhook avec support des fields."""
    if not DISCORD_WEBHOOK_URL:
        log("DISCORD_WEBHOOK_URL non configuré, notification ignorée")
        return False

    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    embed = {
        "title": title,
        "description": message,
        "color": color,
        "timestamp": timestamp,
        "footer": {
            "text": "VPS 51.254.137.175"
        }
    }

    # Ajouter les fields si présents
    if fields:
        embed["fields"] = fields

    # Ajouter la thumbnail si présente
    if thumbnail:
        embed["thumbnail"] = {"url": thumbnail}

    payload = {"embeds": [embed]}

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
    """Notification de démarrage avec infos détaillées."""
    record_start()
    last_crash = get_last_crash_info()

    fields = [
        {"name": "Game", "value": "UDP `4124`", "inline": True},
        {"name": "Auth", "value": "TCP `4125`", "inline": True},
        {"name": "Voice", "value": "UDP `4126`", "inline": True},
    ]

    # Ajouter info sur le dernier crash si disponible
    if last_crash:
        fields.append({"name": "Dernier crash", "value": f"il y a {last_crash}", "inline": True})

    notify_discord(
        title="🟢 Serveur en ligne",
        message=f"Le serveur R-Type est maintenant **opérationnel**",
        color=COLOR_GREEN,
        fields=fields
    )


def notify_stop():
    """Notification d'arrêt propre avec uptime."""
    uptime = record_stop()

    fields = []
    if uptime:
        fields.append({"name": "Uptime", "value": uptime, "inline": True})

    notify_discord(
        title="🔴 Serveur arrêté",
        message="Le serveur R-Type a été **arrêté proprement**",
        color=COLOR_GRAY,
        fields=fields if fields else None
    )


def notify_crash(exit_code: int):
    """Notification de crash avec logs d'erreur et statistiques."""
    uptime, last_crash_ago = record_crash(exit_code)
    last_errors = get_last_errors(lines=15, max_chars=800)

    fields = [
        {"name": "Exit Code", "value": f"`{exit_code}`", "inline": True},
    ]

    if uptime:
        fields.append({"name": "Uptime avant crash", "value": uptime, "inline": True})

    if last_crash_ago:
        fields.append({"name": "Crash précédent", "value": f"il y a {last_crash_ago}", "inline": True})

    # Bloc d'erreurs séparé pour meilleure lisibilité
    error_block = f"```\n{last_errors}\n```" if last_errors != "Pas de logs d'erreur disponibles" else ""

    notify_discord(
        title="💥 CRASH SERVEUR",
        message=(
            f"Le serveur R-Type a **crashé**\n\n"
            f"{error_block}\n"
            "⚠️ **Redémarrage automatique en cours...**"
        ),
        color=COLOR_RED,
        fields=fields
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
        try:
            pid = int(lock_path.read_text().strip())
            subprocess.run(["kill", "-0", str(pid)], check=True, capture_output=True)
            log(f"Une instance est déjà en cours (PID {pid})")
            return False
        except (ValueError, subprocess.CalledProcessError):
            pass

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
    """Lance le serveur dans tmux."""
    exit_code_file = f"/tmp/rtype_exit_code_{subprocess.os.getpid()}"

    tmux_command = (
        f"{server_path} 2>&1 | tee >(systemd-cat -t rtype-server); "
        f"echo ${{PIPESTATUS[0]}} > {exit_code_file}"
    )

    result = subprocess.run([
        "tmux", "-S", TMUX_SOCKET_PATH, "new-session", "-d", "-s", TMUX_SESSION_NAME,
        "bash", "-c", tmux_command
    ], capture_output=True)

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
    """Attend que la session tmux se termine."""
    exit_code_file = f"/tmp/rtype_exit_code_{subprocess.os.getpid()}"

    while session_exists():
        time.sleep(1)

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
            subprocess.run([
                "tmux", "-S", TMUX_SOCKET_PATH, "send-keys", "-t", TMUX_SESSION_NAME, "C-c"
            ], capture_output=True)
            time.sleep(2)
            kill_existing_session()

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

    server_path = Path(args.server_path)
    if not server_path.exists():
        log(f"Erreur: serveur non trouvé: {server_path}")
        sys.exit(1)

    if not acquire_lock():
        log("Erreur: une instance du serveur est déjà en cours")
        sys.exit(1)

    try:
        setup_signal_handlers()
        kill_existing_session()

        if not start_server_in_tmux(str(server_path)):
            release_lock()
            sys.exit(1)

        notify_start()

        exit_code = wait_for_session_end()
        log(f"Serveur terminé avec code: {exit_code}")

        if exit_code == 0:
            notify_stop()
        else:
            notify_crash(exit_code)

        sys.exit(exit_code)

    finally:
        release_lock()


if __name__ == "__main__":
    main()
