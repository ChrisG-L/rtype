
#!/usr/bin/env python3
"""
Builder server with asynchronous job execution.

POST /run -> starts a job (command: "build" or "compile"), returns a UUID immediately.
GET  /status/<uuid>?tail=<n_lines> -> returns job status and optionally last N lines of the log.

Jobs are executed with `/bin/bash <script>` without timeout; stdout/stderr are written
to `artifacts/builder_jobs/<uuid>.log` under the workspace.
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
PORT = int(os.environ.get("BUILDER_PORT", "8080"))

JOB_DIR = os.path.join(WORKSPACE, "artifacts", "builder_jobs")
os.makedirs(JOB_DIR, exist_ok=True)

SCRIPTS = {
	"build": os.path.join(WORKSPACE, "scripts", "build.sh"),
	"compile": os.path.join(WORKSPACE, "scripts", "compile.sh"),
}

# jobs: uuid -> dict(status, command, pid, started_at, finished_at, returncode, log_path)
jobs = {}
jobs_lock = threading.Lock()


def monitor_job(job_id, proc, log_path):
	with jobs_lock:
		jobs[job_id]["status"] = "running"
		jobs[job_id]["pid"] = proc.pid
		jobs[job_id]["started_at"] = time.time()

	returncode = proc.wait()
	with jobs_lock:
		jobs[job_id]["status"] = "finished"
		jobs[job_id]["returncode"] = returncode
		jobs[job_id]["finished_at"] = time.time()


class Handler(BaseHTTPRequestHandler):
	def _set_json(self, code=200):
		self.send_response(code)
		self.send_header("Content-Type", "application/json")
		self.end_headers()

	def do_GET(self):
		parsed = urlparse(self.path)
		path = parsed.path
		if path in ("/", "/health"):
			self._set_json(200)
			payload = {"status": "ok", "allowed_commands": list(SCRIPTS.keys())}
			self.wfile.write(json.dumps(payload).encode())
			return

		# status endpoint: /status/<uuid>
		if path.startswith("/status/"):
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

		self.send_response(404)
		self.end_headers()

	def do_POST(self):
		if self.path != "/run":
			self.send_response(404)
			self.end_headers()
			return

		length = int(self.headers.get("Content-Length", 0))
		body = self.rfile.read(length).decode("utf-8") if length else ""
		try:
			data = json.loads(body) if body else {}
		except json.JSONDecodeError:
			self._set_json(400)
			self.wfile.write(json.dumps({"error": "invalid json"}).encode())
			return

		cmd = data.get("command")
		if cmd not in SCRIPTS:
			self._set_json(400)
			self.wfile.write(json.dumps({"error": "unknown command"}).encode())
			return

		script = SCRIPTS[cmd]
		if not os.path.isfile(script):
			self._set_json(500)
			self.wfile.write(json.dumps({"error": "script not found", "script": script}).encode())
			return

		# create job id and log path
		job_id = str(uuid.uuid4())
		log_path = os.path.join(JOB_DIR, f"{job_id}.log")

		with jobs_lock:
			jobs[job_id] = {
				"status": "queued",
				"command": cmd,
				"pid": None,
				"started_at": None,
				"finished_at": None,
				"returncode": None,
				"log_path": log_path,
			}

		# start process without timeout, redirect output to file
		logfile = open(log_path, "wb")
		try:
			# run with bash to ensure script runs even if not executable
			proc = subprocess.Popen(["/bin/bash", script], cwd=WORKSPACE, stdout=logfile, stderr=subprocess.STDOUT)
		except Exception as e:
			logfile.close()
			with jobs_lock:
				jobs[job_id]["status"] = "failed"
				jobs[job_id]["failed_error"] = str(e)
			self._set_json(500)
			self.wfile.write(json.dumps({"error": "failed to start", "detail": str(e)}).encode())
			return

		# monitor in background thread; thread will close logfile when done
		def _monitor_and_close():
			try:
				monitor_job(job_id, proc, log_path)
			finally:
				try:
					logfile.close()
				except Exception:
					pass

		t = threading.Thread(target=_monitor_and_close, daemon=True)
		t.start()

		self._set_json(200)
		self.wfile.write(json.dumps({"job_id": job_id}).encode())


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
