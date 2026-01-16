#!/usr/bin/env python3
"""
R-Type Deploy Service
Service de déploiement automatique - écoute uniquement sur localhost.
Inclut un lock fichier pour éviter les déploiements concurrents.

Usage:
    python3 deploy-service.py [--port 8081]

Endpoints:
    GET  /status          - Statut du serveur rtype-server
    GET  /deploy/status   - Statut du déploiement en cours (lock)
    GET  /health          - Health check
    POST /deploy          - Déploie un nouvel artifact (avec lock exclusif)
    POST /restart         - Redémarre le serveur sans déploiement
    POST /stop            - Arrête le serveur
    POST /start           - Démarre le serveur
"""

import subprocess
import sys
import json
import shutil
import tempfile
import argparse
import os
import fcntl
import time
from datetime import datetime, timezone
from pathlib import Path
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import urllib.request
import urllib.error
from typing import Optional

# =============================================================================
# Configuration
# =============================================================================

LISTEN_HOST = "127.0.0.1"  # Localhost uniquement - sécurité par design
DEFAULT_PORT = 8081

SERVER_BINARY_PATH = "/opt/rtype/server/rtype_server"
SERVER_SERVICE_NAME = "rtype-server"
BACKUP_DIR = "/opt/rtype/backups"
MAX_BACKUPS = 5

# Lock fichier pour éviter les déploiements concurrents
DEPLOY_LOCK_FILE = "/tmp/rtype-deploy.lock"

# État global du déploiement en cours
_deploy_lock_fd: Optional[int] = None
_deploy_in_progress: bool = False
_deploy_started_at: Optional[str] = None
_deploy_source: Optional[str] = None

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
                "text": "Deploy Service - VPS 51.254.137.175"
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
                "User-Agent": "RType-Deploy/1.0"
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


def notify_deploy_success(source: str):
    """Notification de déploiement réussi."""
    notify_discord(
        title="🚀 Déploiement Réussi",
        message=(
            f"Un nouveau binaire a été **déployé avec succès**\n\n"
            f"**Source:** `{source[:50]}{'...' if len(source) > 50 else ''}`\n"
            f"**Service:** {SERVER_SERVICE_NAME}"
        ),
        color=COLOR_GREEN
    )


def notify_deploy_failure(source: str, error: str):
    """Notification de déploiement échoué."""
    notify_discord(
        title="❌ Échec du Déploiement",
        message=(
            f"Le déploiement a **échoué**\n\n"
            f"**Source:** `{source[:50]}{'...' if len(source) > 50 else ''}`\n"
            f"**Erreur:** {error[:200]}"
        ),
        color=COLOR_RED
    )

# =============================================================================
# Logging
# =============================================================================

def log(message: str, level: str = "INFO"):
    """Log vers stdout (capturé par journald via systemd)."""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] [{level}] {message}", flush=True)

# =============================================================================
# Deploy Lock Management
# =============================================================================

def acquire_deploy_lock(source: str) -> tuple[bool, str]:
    """
    Tente d'acquérir le lock de déploiement.
    Retourne (True, "") si réussi, (False, "raison") sinon.
    """
    global _deploy_lock_fd, _deploy_in_progress, _deploy_started_at, _deploy_source

    try:
        # Ouvrir ou créer le fichier de lock
        _deploy_lock_fd = os.open(DEPLOY_LOCK_FILE, os.O_RDWR | os.O_CREAT, 0o644)

        # Tenter d'acquérir un lock exclusif non-bloquant
        fcntl.flock(_deploy_lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)

        # Lock acquis avec succès
        _deploy_in_progress = True
        _deploy_started_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        _deploy_source = source

        # Écrire les infos dans le fichier de lock
        os.ftruncate(_deploy_lock_fd, 0)
        os.lseek(_deploy_lock_fd, 0, os.SEEK_SET)
        lock_info = json.dumps({
            "pid": os.getpid(),
            "started_at": _deploy_started_at,
            "source": source[:100]
        })
        os.write(_deploy_lock_fd, lock_info.encode())

        log(f"Lock de déploiement acquis pour: {source[:50]}")
        return True, ""

    except BlockingIOError:
        # Lock déjà pris par un autre processus
        if _deploy_lock_fd is not None:
            os.close(_deploy_lock_fd)
            _deploy_lock_fd = None

        # Lire les infos du lock existant
        try:
            with open(DEPLOY_LOCK_FILE, 'r') as f:
                existing_lock = json.load(f)
                reason = (
                    f"Déploiement déjà en cours depuis {existing_lock.get('started_at', '?')} "
                    f"(PID: {existing_lock.get('pid', '?')}, source: {existing_lock.get('source', '?')[:30]})"
                )
        except Exception:
            reason = "Déploiement déjà en cours par un autre processus"

        log(f"Lock refusé: {reason}", "WARN")
        return False, reason

    except Exception as e:
        if _deploy_lock_fd is not None:
            os.close(_deploy_lock_fd)
            _deploy_lock_fd = None
        log(f"Erreur acquisition lock: {e}", "ERROR")
        return False, f"Erreur lock: {e}"


def release_deploy_lock():
    """Libère le lock de déploiement."""
    global _deploy_lock_fd, _deploy_in_progress, _deploy_started_at, _deploy_source

    if _deploy_lock_fd is not None:
        try:
            fcntl.flock(_deploy_lock_fd, fcntl.LOCK_UN)
            os.close(_deploy_lock_fd)
            log("Lock de déploiement libéré")
        except Exception as e:
            log(f"Erreur libération lock: {e}", "WARN")
        finally:
            _deploy_lock_fd = None
            _deploy_in_progress = False
            _deploy_started_at = None
            _deploy_source = None


def get_deploy_status() -> dict:
    """Retourne le statut du déploiement en cours."""
    global _deploy_in_progress, _deploy_started_at, _deploy_source

    # Vérifier si un autre processus a le lock
    if not _deploy_in_progress:
        try:
            with open(DEPLOY_LOCK_FILE, 'r') as f:
                lock_info = json.load(f)
                # Vérifier si le processus est toujours actif
                pid = lock_info.get('pid')
                if pid:
                    try:
                        os.kill(pid, 0)  # Vérifie si le processus existe
                        return {
                            "in_progress": True,
                            "started_at": lock_info.get('started_at'),
                            "source": lock_info.get('source'),
                            "pid": pid,
                            "owner": "other_process"
                        }
                    except ProcessLookupError:
                        pass  # Processus mort, lock stale
        except (FileNotFoundError, json.JSONDecodeError):
            pass

    return {
        "in_progress": _deploy_in_progress,
        "started_at": _deploy_started_at,
        "source": _deploy_source,
        "pid": os.getpid() if _deploy_in_progress else None,
        "owner": "this_process" if _deploy_in_progress else None
    }


# =============================================================================
# Deployment Logic
# =============================================================================

def get_service_status() -> dict:
    """Récupère le statut du service rtype-server."""
    try:
        result = subprocess.run(
            ["systemctl", "is-active", SERVER_SERVICE_NAME],
            capture_output=True, text=True
        )
        is_active = result.stdout.strip() == "active"

        # Récupérer plus de détails
        result_status = subprocess.run(
            ["systemctl", "show", SERVER_SERVICE_NAME,
             "--property=ActiveState,SubState,MainPID"],
            capture_output=True, text=True
        )

        props = {}
        for line in result_status.stdout.strip().split("\n"):
            if "=" in line:
                key, value = line.split("=", 1)
                props[key] = value

        return {
            "service": SERVER_SERVICE_NAME,
            "active": is_active,
            "state": props.get("ActiveState", "unknown"),
            "substate": props.get("SubState", "unknown"),
            "pid": props.get("MainPID", "0"),
            "binary_exists": Path(SERVER_BINARY_PATH).exists(),
            "binary_path": SERVER_BINARY_PATH
        }
    except Exception as e:
        return {
            "service": SERVER_SERVICE_NAME,
            "active": False,
            "error": str(e)
        }

def stop_server() -> tuple[bool, str]:
    """Arrête le serveur."""
    log(f"Arrêt du service {SERVER_SERVICE_NAME}...")
    try:
        result = subprocess.run(
            ["sudo", "systemctl", "stop", SERVER_SERVICE_NAME],
            capture_output=True, text=True, timeout=30
        )
        if result.returncode == 0:
            log("Service arrêté")
            return True, "Service stopped"
        else:
            return False, f"Failed to stop: {result.stderr}"
    except subprocess.TimeoutExpired:
        return False, "Timeout stopping service"
    except Exception as e:
        return False, str(e)

def start_server() -> tuple[bool, str]:
    """Démarre le serveur."""
    log(f"Démarrage du service {SERVER_SERVICE_NAME}...")
    try:
        result = subprocess.run(
            ["sudo", "systemctl", "start", SERVER_SERVICE_NAME],
            capture_output=True, text=True, timeout=30
        )
        if result.returncode == 0:
            log("Service démarré")
            return True, "Service started"
        else:
            return False, f"Failed to start: {result.stderr}"
    except subprocess.TimeoutExpired:
        return False, "Timeout starting service"
    except Exception as e:
        return False, str(e)

def restart_server() -> tuple[bool, str]:
    """Redémarre le serveur."""
    log(f"Redémarrage du service {SERVER_SERVICE_NAME}...")
    try:
        result = subprocess.run(
            ["sudo", "systemctl", "restart", SERVER_SERVICE_NAME],
            capture_output=True, text=True, timeout=60
        )
        if result.returncode == 0:
            log("Service redémarré")
            return True, "Service restarted"
        else:
            return False, f"Failed to restart: {result.stderr}"
    except subprocess.TimeoutExpired:
        return False, "Timeout restarting service"
    except Exception as e:
        return False, str(e)

def backup_current_binary() -> tuple[bool, str]:
    """Sauvegarde le binaire actuel."""
    binary_path = Path(SERVER_BINARY_PATH)
    if not binary_path.exists():
        return True, "No existing binary to backup"

    backup_dir = Path(BACKUP_DIR)
    backup_dir.mkdir(parents=True, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_path = backup_dir / f"rtype_server_{timestamp}"

    try:
        shutil.copy2(binary_path, backup_path)
        log(f"Backup créé: {backup_path}")

        # Nettoyage des vieux backups
        backups = sorted(backup_dir.glob("rtype_server_*"), reverse=True)
        for old_backup in backups[MAX_BACKUPS:]:
            old_backup.unlink()
            log(f"Vieux backup supprimé: {old_backup}")

        return True, str(backup_path)
    except Exception as e:
        return False, f"Backup failed: {e}"

def deploy_binary(source_path: str) -> tuple[bool, str]:
    """
    Déploie un nouveau binaire.
    source_path peut être:
    - Un chemin local (/path/to/binary)
    - Une URL HTTP(S) (http://jenkins/artifact/...)
    """
    log(f"Déploiement depuis: {source_path}")

    temp_file = None

    try:
        # Si c'est une URL, télécharger d'abord
        if source_path.startswith("http://") or source_path.startswith("https://"):
            log("Téléchargement de l'artifact...")
            temp_file = tempfile.NamedTemporaryFile(delete=False)

            try:
                urllib.request.urlretrieve(source_path, temp_file.name)
                source_path = temp_file.name
                log("Téléchargement terminé")
            except urllib.error.URLError as e:
                return False, f"Download failed: {e}"

        # Vérifier que le fichier source existe
        source = Path(source_path)
        if not source.exists():
            return False, f"Source file not found: {source_path}"

        # Vérifier que c'est un exécutable ELF (basique)
        with open(source, "rb") as f:
            magic = f.read(4)
            if magic != b"\x7fELF":
                return False, "Source is not a valid ELF binary"

        # Backup du binaire actuel
        success, msg = backup_current_binary()
        if not success:
            log(f"Warning: {msg}", "WARN")

        # Copier le nouveau binaire
        dest = Path(SERVER_BINARY_PATH)
        dest.parent.mkdir(parents=True, exist_ok=True)

        shutil.copy2(source, dest)
        dest.chmod(0o755)

        log(f"Nouveau binaire installé: {dest}")
        return True, "Binary deployed successfully"

    except Exception as e:
        return False, f"Deploy failed: {e}"
    finally:
        if temp_file:
            Path(temp_file.name).unlink(missing_ok=True)

def full_deploy(source_path: str) -> dict:
    """
    Processus de déploiement complet avec lock exclusif:
    0. Acquérir le lock (évite les déploiements concurrents)
    1. Arrête le serveur
    2. Déploie le binaire (backup + copie)
    3. Redémarre le serveur
    4. Vérifie le statut
    5. Libère le lock
    """
    result = {
        "success": False,
        "steps": [],
        "error": None,
        "locked": False
    }

    # Étape 0: Acquérir le lock
    lock_acquired, lock_error = acquire_deploy_lock(source_path)
    if not lock_acquired:
        result["error"] = lock_error
        result["steps"].append({"step": "lock", "success": False, "message": lock_error})
        log(f"Déploiement refusé: {lock_error}", "WARN")
        return result

    result["locked"] = True
    result["steps"].append({"step": "lock", "success": True, "message": "Lock acquis"})

    try:
        log("=== DÉBUT DU DÉPLOIEMENT ===")

        # Étape 1: Arrêter le serveur
        success, msg = stop_server()
        result["steps"].append({"step": "stop", "success": success, "message": msg})
        if not success:
            result["error"] = f"Stop failed: {msg}"
            # Continuer quand même, le serveur n'est peut-être pas lancé

        # Étape 2: Déployer le binaire
        success, msg = deploy_binary(source_path)
        result["steps"].append({"step": "deploy", "success": success, "message": msg})
        if not success:
            result["error"] = f"Deploy failed: {msg}"
            # Tenter de redémarrer avec l'ancien binaire
            start_server()
            notify_deploy_failure(source_path, msg)
            return result

        # Étape 3: Démarrer le serveur
        success, msg = start_server()
        result["steps"].append({"step": "start", "success": success, "message": msg})
        if not success:
            result["error"] = f"Start failed: {msg}"
            notify_deploy_failure(source_path, msg)
            return result

        # Étape 4: Vérifier le statut
        time.sleep(2)  # Attendre que le serveur démarre
        status = get_service_status()
        result["steps"].append({"step": "verify", "success": status["active"], "status": status})

        if not status["active"]:
            result["error"] = "Server failed to start after deploy"
            notify_deploy_failure(source_path, result["error"])
            return result

        result["success"] = True
        log("=== DÉPLOIEMENT RÉUSSI ===")
        notify_deploy_success(source_path)
        return result

    finally:
        # Toujours libérer le lock
        release_deploy_lock()
        result["locked"] = False

# =============================================================================
# HTTP Handler
# =============================================================================

class DeployHandler(BaseHTTPRequestHandler):
    """Handler HTTP pour les requêtes de déploiement."""

    def log_message(self, format, *args):
        """Override pour utiliser notre logger."""
        log(f"{self.address_string()} - {format % args}")

    def send_json(self, data: dict, status: int = 200):
        """Envoie une réponse JSON."""
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(data, indent=2).encode())

    def do_GET(self):
        """Handle GET requests."""
        parsed = urlparse(self.path)

        if parsed.path == "/status":
            status = get_service_status()
            self.send_json(status)
        elif parsed.path == "/deploy/status":
            # Statut du déploiement en cours
            deploy_status = get_deploy_status()
            self.send_json(deploy_status)
        elif parsed.path == "/health":
            self.send_json({"status": "ok", "service": "deploy-service"})
        else:
            self.send_json({"error": "Not found"}, 404)

    def do_POST(self):
        """Handle POST requests."""
        parsed = urlparse(self.path)

        # Lire le body si présent
        content_length = int(self.headers.get("Content-Length", 0))
        body = {}
        if content_length > 0:
            raw_body = self.rfile.read(content_length)
            try:
                body = json.loads(raw_body.decode())
            except json.JSONDecodeError:
                self.send_json({"error": "Invalid JSON"}, 400)
                return

        if parsed.path == "/deploy":
            # Récupérer la source du binaire
            source = body.get("source") or body.get("artifact_url") or body.get("path")

            if not source:
                self.send_json({
                    "error": "Missing 'source' parameter",
                    "usage": {
                        "source": "/path/to/binary OR http://url/to/artifact"
                    }
                }, 400)
                return

            result = full_deploy(source)
            status_code = 200 if result["success"] else 500
            self.send_json(result, status_code)

        elif parsed.path == "/restart":
            success, msg = restart_server()
            self.send_json({
                "success": success,
                "message": msg,
                "status": get_service_status()
            }, 200 if success else 500)

        elif parsed.path == "/stop":
            success, msg = stop_server()
            self.send_json({
                "success": success,
                "message": msg
            }, 200 if success else 500)

        elif parsed.path == "/start":
            success, msg = start_server()
            self.send_json({
                "success": success,
                "message": msg,
                "status": get_service_status()
            }, 200 if success else 500)

        else:
            self.send_json({"error": "Not found"}, 404)

# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description="R-Type Deploy Service")
    parser.add_argument(
        "--port", "-p",
        type=int,
        default=DEFAULT_PORT,
        help=f"Port d'écoute (défaut: {DEFAULT_PORT})"
    )
    args = parser.parse_args()

    server_address = (LISTEN_HOST, args.port)
    httpd = HTTPServer(server_address, DeployHandler)

    log(f"Deploy service démarré sur http://{LISTEN_HOST}:{args.port}")
    log(f"Lock fichier: {DEPLOY_LOCK_FILE}")
    log("Endpoints disponibles:")
    log("  GET  /status        - Statut du serveur rtype-server")
    log("  GET  /deploy/status - Statut du déploiement en cours (lock)")
    log("  GET  /health        - Health check")
    log("  POST /deploy        - Déployer un artifact (avec lock exclusif)")
    log("  POST /restart       - Redémarrer le serveur")
    log("  POST /stop          - Arrêter le serveur")
    log("  POST /start         - Démarrer le serveur")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        log("Arrêt demandé")
        httpd.shutdown()

if __name__ == "__main__":
    main()
