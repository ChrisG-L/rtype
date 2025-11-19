
#!/usr/bin/env python3
"""
Builder server with workspace-isolated job execution.

Routes:
  POST   /workspace/create                -> Create isolated workspace for a build
  POST   /workspace/{id}/run              -> Start a job in workspace (build/compile)
  GET    /workspace/{id}/artifacts        -> List artifacts in workspace
  DELETE /workspace/{id}                  -> Delete workspace and cleanup
  GET    /status/{uuid}?tail=<n_lines>    -> Get job status and optional log tail
  GET    /health                          -> Health check

Jobs are executed with `/bin/bash <script>` in isolated workspaces.
Logs are written to `{workspace}/artifacts/{uuid}.log`.
"""

import json
import os
import subprocess
import threading
import uuid
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn
from urllib.parse import urlparse, unquote, parse_qs

WORKSPACE = os.environ.get("WORKSPACE", "/workspace")
PORT = int(os.environ.get("BUILDER_PORT", "8082"))

WORKSPACE_404 = "workspace not found"
WORKSPACE_PATHPREFIX = "/workspace/"

# Commandes disponibles pour les workspaces
AVAILABLE_COMMANDS = ["build", "compile"]

# jobs: uuid -> dict(status, command, pid, started_at, finished_at, returncode, log_path)
jobs = {}
jobs_lock = threading.Lock()

# workspaces: workspace_id -> dict(path, created_at)
workspaces = {}
workspaces_lock = threading.Lock()


def monitor_job(job_id, proc):
    with jobs_lock:
        jobs[job_id]["status"] = "running"
        jobs[job_id]["pid"] = proc.pid
        jobs[job_id]["started_at"] = time.time()

    returncode = proc.wait()
    with jobs_lock:
        jobs[job_id]["status"] = "finished"
        jobs[job_id]["returncode"] = returncode
        jobs[job_id]["finished_at"] = time.time()


def create_workspace(build_number):
    """Crée un workspace isolé pour un build."""
    workspace_id = f"build_{build_number}"
    workspace_path = os.path.join(WORKSPACE, "builds", workspace_id)

    # Créer les répertoires
    os.makedirs(workspace_path, exist_ok=True)
    os.makedirs(os.path.join(workspace_path, "artifacts"), exist_ok=True)

    with workspaces_lock:
        if workspace_id not in workspaces:
            workspaces[workspace_id] = {
                "path": workspace_path,
                "created_at": time.time()
            }

    return workspace_id, workspace_path


def delete_workspace(workspace_id):
    """Supprime un workspace et ses fichiers."""
    import shutil
    with workspaces_lock:
        if workspace_id in workspaces:
            workspace_path = workspaces[workspace_id]["path"]
            del workspaces[workspace_id]

            if os.path.exists(workspace_path):
                try:
                    shutil.rmtree(workspace_path)
                    return True
                except Exception as e:
                    print(f"Error deleting workspace {workspace_id}: {e}")
                    return False
    return False


class Handler(BaseHTTPRequestHandler):
    def _set_json(self, code=200):
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()

    def sendArtifacts(self, path):
        parts = path.split("/")
        if len(parts) != 4:
            print("Invalid artifacts path:")
            print(path)
            print(parts)
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "invalid path"}).encode())
            return

        workspace_id = parts[2]

        with workspaces_lock:
            if workspace_id not in workspaces:
                self._set_json(404)
                self.wfile.write(json.dumps({"error": WORKSPACE_404}).encode())
                return

            workspace_path = workspaces[workspace_id]["path"]

        artifacts_path = os.path.join(workspace_path, "artifacts")

        if not os.path.exists(artifacts_path):
            self._set_json(404)
            self.wfile.write(json.dumps({"error": "no artifacts directory"}).encode())
            return

        # Lister récursivement tous les fichiers dans artifacts/
        artifacts_list = []
        for root, dirs, files in os.walk(artifacts_path):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, artifacts_path)
                file_size = os.path.getsize(full_path)
                artifacts_list.append({
                    "path": rel_path,
                    "size": file_size,
                    "full_path": full_path
                })

        self._set_json(200)
        self.wfile.write(json.dumps({
            "workspace_id": workspace_id,
            "artifacts": artifacts_list,
            "artifacts_path": f"/workspace/{workspace_id}/artifacts/"
        }).encode())

    def sendHealth(self):
        self._set_json(200)
        payload = {"status": "ok", "allowed_commands": AVAILABLE_COMMANDS}
        self.wfile.write(json.dumps(payload).encode())

    def sendStatus(self, parsed, path):
        job_id = unquote(path[len("/status/"):])
        if not job_id:
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "missing job id"}).encode())
            return

        with jobs_lock:
            job = jobs.get(job_id)
        if not job:
            self._set_json(404)
            self.wfile.write(json.dumps({"error": "job not found"}).encode())
            return

        # parse optional tail param
        qs = parse_qs(parsed.query)
        tail = None
        if "tail" in qs:
            try:
                tail = int(qs.get("tail", [0])[0])
            except Exception:
                tail = None

        resp = {k: v for k, v in job.items() if k != "log_path"}
        if tail and os.path.isfile(job.get("log_path", "")):
            try:
                with open(job["log_path"], "rb") as f:
                    data = f.read()
                # return last N lines
                text = data.decode(errors="replace")
                lines = text.splitlines()
                resp["log_tail"] = "\n".join(lines[-tail:])
            except Exception as e:
                resp["log_error"] = str(e)

        self._set_json(200)
        self.wfile.write(json.dumps(resp).encode())
        return

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # Route: GET /workspace/{workspace_id}/artifacts
        # Retourne la liste des fichiers artifacts disponibles
        if path.startswith(WORKSPACE_PATHPREFIX) and path.endswith("/artifacts"):
            self.sendArtifacts(path)
            return

        if path in ("/", "/health"):
            self.sendHealth()
            return

        # status endpoint: /status/<uuid>
        if path.startswith("/status/"):
            self.sendStatus(parsed, path)
            return

        self.send_response(404)
        self.end_headers()

    def createWorkspace(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length else ""
        try:
            data = json.loads(body) if body else {}
        except json.JSONDecodeError:
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "invalid json"}).encode())
            return

        build_number = data.get("build_number")
        if not build_number:
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "build_number required"}).encode())
            return

        workspace_id, workspace_path = create_workspace(build_number)
        self._set_json(200)
        self.wfile.write(json.dumps({
            "workspace_id": workspace_id,
            "workspace_path": workspace_path
        }).encode())

    def runCmdInWorkspace(self, path):
        parts = path.split("/")
        if len(parts) != 4:
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "invalid path"}).encode())
            return
        workspace_id = parts[2]

        with workspaces_lock:
            if workspace_id not in workspaces:
                self._set_json(404)
                self.wfile.write(json.dumps({"error": WORKSPACE_404}).encode())
                return
            workspace_path = workspaces[workspace_id]["path"]

        # Lire le body pour obtenir la commande
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length else ""

        try:
            data = json.loads(body) if body else {}
        except json.JSONDecodeError as e:
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "invalid json"}).encode())
            return

        cmd = data.get("command")

        # Parse la commande et les arguments (ex: "build --platform=windows")
        cmd_parts = cmd.split() if cmd else []
        if not cmd_parts:
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "empty command"}).encode())
            return

        base_cmd = cmd_parts[0]
        cmd_args = cmd_parts[1:] if len(cmd_parts) > 1 else []

        # Mapping des commandes vers les scripts (dans le workspace uploadé)
        script_mapping = {
            "build": "scripts/build.sh",
            "compile": "scripts/compile.sh",
        }

        if base_cmd not in script_mapping:
            self._set_json(400)
            self.wfile.write(json.dumps({"error": "unknown command", "allowed": list(script_mapping.keys())}).encode())
            return

        # Le script doit être dans le workspace uploadé
        script_relative = script_mapping[base_cmd]
        script = os.path.join(workspace_path, script_relative)

        if not os.path.isfile(script):
            error_response = {
                "error": "script not found in workspace",
                "script": script_relative,
                "script_full_path": script,
                "workspace_path": workspace_path,
                "hint": "Make sure the scripts are uploaded via rsync"
            }
            self._set_json(500)
            self.wfile.write(json.dumps(error_response).encode())
            return

        # Créer job avec workspace spécifique
        job_id = str(uuid.uuid4())

        # S'assurer que le dossier artifacts existe (rsync --delete peut l'avoir supprimé)
        artifacts_dir = os.path.join(workspace_path, "artifacts")
        os.makedirs(artifacts_dir, exist_ok=True)

        log_path = os.path.join(artifacts_dir, f"{job_id}.log")

        with jobs_lock:
            jobs[job_id] = {
                "workspace_id": workspace_id,
                "status": "queued",
                "command": cmd,
                "pid": None,
                "started_at": None,
                "finished_at": None,
                "returncode": None,
                "log_path": log_path,
            }

        # Lancer le process dans le workspace
        logfile = open(log_path, "wb")
        try:
            # Construire les arguments du script
            script_args = ["/bin/bash", script]

            # Ajouter les arguments passés avec la commande (ex: --platform=windows)
            script_args.extend(cmd_args)

            # Ajouter --no-launch pour compile car le serveur ne doit pas se lancer dans CI/CD
            if base_cmd == "compile":
                script_args.append("--no-launch")

            proc = subprocess.Popen(
                script_args,
                cwd=workspace_path,  # Important: working dir = workspace
                stdout=logfile,
                stderr=subprocess.STDOUT
            )
        except Exception as e:
            logfile.close()
            with jobs_lock:
                jobs[job_id]["status"] = "failed"
                jobs[job_id]["failed_error"] = str(e)
            error_msg = f"Failed to start process: {str(e)}"
            self._set_json(500)
            self.wfile.write(json.dumps({
                "error": "failed to start",
                "detail": str(e),
                "script": script_relative,
                "workspace_path": workspace_path
            }).encode())
            return

        def _monitor_and_close():
            try:
                monitor_job(job_id, proc)
            finally:
                try:
                    logfile.close()
                except Exception:
                    pass

        t = threading.Thread(target=_monitor_and_close, daemon=True)
        t.start()

        self._set_json(200)
        self.wfile.write(json.dumps({"job_id": job_id}).encode())
        return

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # Route: POST /workspace/create
        if path == "/workspace/create":
            self.createWorkspace()
            return

        # Route: POST /workspace/{workspace_id}/run
        if path.startswith(WORKSPACE_PATHPREFIX) and path.endswith("/run"):
            self.runCmdInWorkspace(path)
            return

        self.send_response(404)
        self.end_headers()

    def do_DELETE(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # Route: DELETE /workspace/{workspace_id}
        if path.startswith(WORKSPACE_PATHPREFIX):
            parts = path.split("/")
            if len(parts) >= 3:
                workspace_id = parts[2]

                if delete_workspace(workspace_id):
                    self._set_json(200)
                    self.wfile.write(json.dumps({"message": "workspace deleted"}).encode())
                else:
                    self._set_json(404)
                    self.wfile.write(json.dumps({"error": WORKSPACE_404}).encode())
                return

        self.send_response(404)
        self.end_headers()


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True


def main():
    server = ThreadedHTTPServer(("0.0.0.0", PORT), Handler)
    print(f"Builder server listening on 0.0.0.0:{PORT}, workspace={WORKSPACE}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("Shutting down")
        server.shutdown()


if __name__ == "__main__":
    main()
