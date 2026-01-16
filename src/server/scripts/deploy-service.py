#!/usr/bin/env python3
"""
R-Type Deploy Service
Service de déploiement automatique - écoute uniquement sur localhost.

Usage:
    python3 deploy-service.py [--port 8080]

Endpoints:
    POST /deploy          - Déploie un nouvel artifact
    GET  /status          - Statut du serveur
    POST /restart         - Redémarre le serveur sans déploiement
"""

import subprocess
import sys
import json
import shutil
import tempfile
import argparse
from datetime import datetime
from pathlib import Path
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import urllib.request
import urllib.error

# =============================================================================
# Configuration
# =============================================================================

LISTEN_HOST = "127.0.0.1"  # Localhost uniquement - sécurité par design
DEFAULT_PORT = 8080

SERVER_BINARY_PATH = "/opt/rtype/server/rtype_server"
SERVER_SERVICE_NAME = "rtype-server"
BACKUP_DIR = "/opt/rtype/backups"
MAX_BACKUPS = 5

# =============================================================================
# Logging
# =============================================================================

def log(message: str, level: str = "INFO"):
    """Log vers stdout (capturé par journald via systemd)."""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] [{level}] {message}", flush=True)

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
    Processus de déploiement complet:
    1. Télécharge/copie le nouveau binaire
    2. Arrête le serveur
    3. Remplace le binaire
    4. Redémarre le serveur
    5. Vérifie le statut
    """
    result = {
        "success": False,
        "steps": [],
        "error": None
    }

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
        return result

    # Étape 3: Démarrer le serveur
    success, msg = start_server()
    result["steps"].append({"step": "start", "success": success, "message": msg})
    if not success:
        result["error"] = f"Start failed: {msg}"
        return result

    # Étape 4: Vérifier le statut
    import time
    time.sleep(2)  # Attendre que le serveur démarre
    status = get_service_status()
    result["steps"].append({"step": "verify", "success": status["active"], "status": status})

    if not status["active"]:
        result["error"] = "Server failed to start after deploy"
        return result

    result["success"] = True
    log("=== DÉPLOIEMENT RÉUSSI ===")
    return result

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
    log("Endpoints disponibles:")
    log("  GET  /status  - Statut du serveur")
    log("  GET  /health  - Health check")
    log("  POST /deploy  - Déployer un artifact {\"source\": \"path_or_url\"}")
    log("  POST /restart - Redémarrer le serveur")
    log("  POST /stop    - Arrêter le serveur")
    log("  POST /start   - Démarrer le serveur")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        log("Arrêt demandé")
        httpd.shutdown()

if __name__ == "__main__":
    main()
