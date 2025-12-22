# claude.py
"""
Module pour lancer Claude Code de manière programmatique avec streaming des logs.
Architecture async native pour éviter les timeouts.
"""

import asyncio
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import websockets


# =============================================================================
# Utilitaires
# =============================================================================

def log_to_stderr(text: str) -> None:
    """Log vers stderr pour voir en temps réel dans le terminal."""
    sys.stderr.write(text)
    sys.stderr.flush()


def strip_ansi(text: str) -> str:
    """Supprime les séquences ANSI de contrôle (couleurs, curseur, etc.)."""
    ansi_escape = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')
    text = ansi_escape.sub('', text)
    text = re.sub(r'\x1B\].*?(?:\x07|\x1B\\)', '', text)
    text = re.sub(r'\x1B[()][AB012]', '', text)
    text = re.sub(r'\x1B[@-_]', '', text)
    text = re.sub(r'[\x00-\x08\x0B\x0C\x0E-\x1F]', '', text)
    return text


def switch_branch(branch_name: str) -> bool:
    """Change de branche git."""
    log_to_stderr(f"Switching to branch: {branch_name}\n")
    result = subprocess.run(
        ["git", "checkout", branch_name],
        capture_output=True,
        text=True
    )
    if result.stdout:
        log_to_stderr(result.stdout)
    return result.returncode == 0


# =============================================================================
# WebSocket Notifier
# =============================================================================

@dataclass
class StepInfo:
    """Informations sur une étape en cours."""
    name: str
    status: str = "running"


class WebSocketNotifier:
    """
    Gère la communication WebSocket pour notifier l'avancement.
    Encapsule toute la logique de messaging.
    """

    STEPS = [
        "setup",           # Initialisation
        "phase1",          # ANALYZER + SECURITY + REVIEWER (parallèle)
        "phase2_risk",     # RISK
        "phase2_parallel", # SYNTHESIS + SONAR (parallèle)
        "phase3",          # META-SYNTHESIS
        "phase4",          # WEB-SYNTHESIZER
        "report"           # Envoi du rapport final
    ]

    # Mapping agent -> phase
    AGENT_TO_PHASE = {
        "analyzer": "phase1",
        "security": "phase1",
        "reviewer": "phase1",
        "risk": "phase2_risk",
        "synthesis": "phase2_parallel",
        "sonar": "phase2_parallel",
        "meta-synthesis": "phase3",
        "web-synthesizer": "phase4",
    }

    def __init__(self, websocket):
        self.ws = websocket
        self.current_step: Optional[str] = None
        self.action_status: str = "success"
        self.action_completed: bool = False
        self.agents_in_phase: dict[str, set] = {}  # phase -> set d'agents lancés
        self.report_path: Optional[str] = None     # Chemin du rapport final
        self.completed_steps: set[str] = set()     # Étapes déjà complétées

    def _get_next_step(self) -> Optional[str]:
        """Retourne la prochaine étape dans l'ordre défini par STEPS."""
        if not self.current_step:
            return self.STEPS[0] if self.STEPS else None
        try:
            idx = self.STEPS.index(self.current_step)
            if idx + 1 < len(self.STEPS):
                return self.STEPS[idx + 1]
        except ValueError:
            pass
        return None

    def _is_valid_transition(self, from_step: Optional[str], to_step: str) -> bool:
        """Vérifie si la transition est valide (pas de retour en arrière)."""
        if not from_step:
            return True
        try:
            from_idx = self.STEPS.index(from_step)
            to_idx = self.STEPS.index(to_step)
            return to_idx >= from_idx
        except ValueError:
            return True

    async def send(self, message: dict) -> None:
        """Envoie un message JSON au WebSocket."""
        try:
            await self.ws.send(json.dumps(message))
        except websockets.exceptions.ConnectionClosed:
            log_to_stderr("[WS] Connection closed, cannot send message\n")

    async def action_started(self) -> None:
        """Notifie le début de l'action avec les étapes."""
        await self.send({
            "type": "action_started",
            "steps": self.STEPS
        })

    async def step_started(self, step: str) -> None:
        """Notifie le début d'une étape."""
        # Éviter de redémarrer une étape déjà complétée ou en cours
        if step in self.completed_steps:
            log_to_stderr(f"[WS] Step '{step}' already completed, skipping\n")
            return
        if self.current_step == step:
            return  # Déjà sur cette étape

        self.current_step = step
        log_to_stderr(f"\n▶️  Step started: {step}\n")
        await self.send({
            "type": "step_started",
            "step": step
        })

    async def step_log(self, line: str, stream: str = "stdout") -> None:
        """Envoie un log pour l'étape en cours."""
        if not self.current_step:
            return
        await self.send({
            "type": "step_log",
            "step": self.current_step,
            "line": line,
            "stream": stream
        })

    async def step_complete(self, status: str, output: str = "", auto_next: bool = False) -> None:
        """
        Marque l'étape en cours comme terminée.

        Args:
            status: "success" ou "failure"
            output: Message de sortie optionnel
            auto_next: Si True, démarre automatiquement la prochaine étape
        """
        if not self.current_step:
            return
        if status == "failure":
            self.action_status = "failure"

        completed_step = self.current_step
        self.completed_steps.add(completed_step)

        log_to_stderr(f"\n✅ Step completed: {completed_step} ({status})\n")
        await self.send({
            "type": "step_complete",
            "step": completed_step,
            "status": status,
            "output": output
        })

        # Démarrer automatiquement la prochaine étape si demandé
        if auto_next and status == "success":
            next_step = self._get_next_step()
            if next_step:
                await self.step_started(next_step)

    async def action_complete(self) -> None:
        """Marque l'action comme terminée."""
        if self.action_completed:
            return
        self.action_completed = True
        await self.send({
            "type": "action_complete",
            "status": self.action_status
        })

    async def transition_to(self, new_step: str, prev_output: str = "") -> None:
        """
        Transition propre d'une étape à une autre.
        Valide que la transition est dans le bon sens (pas de retour en arrière).
        """
        # Vérifier si on est déjà sur cette étape
        if self.current_step == new_step:
            return

        # Vérifier si l'étape cible est déjà complétée
        if new_step in self.completed_steps:
            log_to_stderr(f"[WS] Cannot transition to already completed step '{new_step}'\n")
            return

        # Valider la direction de la transition
        if not self._is_valid_transition(self.current_step, new_step):
            log_to_stderr(f"[WS] Invalid backward transition from '{self.current_step}' to '{new_step}'\n")
            return

        # Compléter l'étape actuelle (sans auto_next car on gère manuellement)
        if self.current_step:
            await self.step_complete("success", prev_output, auto_next=False)

        # Démarrer la nouvelle étape
        await self.step_started(new_step)

    async def on_agent_started(self, agent_type: str) -> None:
        """Gère le lancement d'un agent et la transition de phase."""
        phase = self.AGENT_TO_PHASE.get(agent_type)
        if not phase:
            return

        # Tracker l'agent dans sa phase
        if phase not in self.agents_in_phase:
            self.agents_in_phase[phase] = set()
        self.agents_in_phase[phase].add(agent_type)

        # Transition vers la nouvelle phase si nécessaire
        if self.current_step != phase:
            await self.transition_to(phase, f"Completed {self.current_step}")

    async def report_ready(self, report_path: str) -> None:
        """Envoie le rapport JSON final via WebSocket."""
        self.report_path = report_path

        try:
            report_data = json.loads(Path(report_path).read_text())
            await self.send({
                "type": "report_ready",
                "path": report_path,
                "data": report_data
            })
            log_to_stderr(f"\n📤 Report sent via WebSocket: {report_path}\n")
        except FileNotFoundError:
            log_to_stderr(f"\n❌ Report file not found: {report_path}\n")
            await self.send({
                "type": "report_error",
                "path": report_path,
                "error": "File not found"
            })
        except json.JSONDecodeError as e:
            log_to_stderr(f"\n❌ Invalid JSON in report: {e}\n")
            await self.send({
                "type": "report_error",
                "path": report_path,
                "error": f"Invalid JSON: {e}"
            })


# =============================================================================
# Handlers pour les outils Claude
# =============================================================================

TOOL_FORMATTERS = {
    "Bash": lambda inp: f"$ {inp.get('command', '')[:200]}",
    "Read": lambda inp: f"📄 {inp.get('file_path', '')}",
    "Edit": lambda inp: f"✏️  {inp.get('file_path', '')}",
    "Write": lambda inp: f"📝 {inp.get('file_path', '')}",
    "Grep": lambda inp: f"🔍 Pattern: {inp.get('pattern', '')}",
    "Glob": lambda inp: f"📂 Pattern: {inp.get('pattern', '')}",
    "Task": lambda inp: f"🚀 {inp.get('subagent_type', 'agent')}: {inp.get('description', '')}",
}


def format_tool_log(tool_name: str, tool_input: dict) -> str:
    """Formate le log d'un outil de manière concise."""
    formatter = TOOL_FORMATTERS.get(tool_name)
    if formatter:
        return f"🔧 {tool_name}: {formatter(tool_input)}"
    return f"🔧 {tool_name}: {json.dumps(tool_input, ensure_ascii=False)[:200]}"


# =============================================================================
# Processeur d'événements Claude
# =============================================================================

class ClaudeEventProcessor:
    """
    Traite les événements JSON streamés par Claude.
    Sépare la logique de parsing de la logique de notification.
    """

    def __init__(self, notifier: WebSocketNotifier, verbosity: int = 1):
        self.notifier = notifier
        self.verbosity = verbosity
        self.result_text = ""
        self.total_cost = 0.0
        self.messages = []
        self.pending_report_path: Optional[str] = None  # Chemin du rapport détecté

    async def process_event(self, event: dict) -> None:
        """Traite un événement JSON de Claude."""
        event_type = event.get("type", "")

        if self.verbosity >= 2:
            log_to_stderr(f"\n[DEBUG] {event_type}: {json.dumps(event, ensure_ascii=False)[:500]}\n")

        handler = getattr(self, f"_handle_{event_type}", None)
        if handler:
            await handler(event)

        self.messages.append(event)

    async def _handle_assistant(self, event: dict) -> None:
        """Traite les messages de l'assistant."""
        content = event.get("message", {}).get("content", [])

        for block in content:
            block_type = block.get("type")

            if block_type == "text":
                text = block.get("text", "")
                self.result_text += text
                log_to_stderr(text)
                await self.notifier.step_log(text.strip())

            elif block_type == "tool_use":
                await self._handle_tool_use(block)

            elif block_type == "thinking":
                thinking = block.get("thinking", "")
                preview = thinking[:500] + "..." if len(thinking) > 500 else thinking
                log_to_stderr(f"\n💭 [Thinking]: {preview}\n")
                await self.notifier.step_log(f"[Thinking] {thinking[:200]}")

    async def _handle_tool_use(self, block: dict) -> None:
        """Traite l'utilisation d'un outil."""
        tool_name = block.get("name", "unknown")
        tool_input = block.get("input", {})

        # Détecter les agents Task
        if tool_name == "Task":
            agent_type = tool_input.get("subagent_type", "")
            if agent_type:
                await self.notifier.on_agent_started(agent_type)
                tool_log = f"🚀 Agent: {agent_type} - {tool_input.get('description', '')}"
                log_to_stderr(f"\n{tool_log}\n")
                await self.notifier.step_log(tool_log)
                return

        # Détecter la création du rapport web (Bash ou Write)
        if tool_name in ("Bash", "Write"):
            content = tool_input.get("command", "") or tool_input.get("file_path", "")
            # Pattern: reports/web-report-YYYY-MM-DD-commit.json
            match = re.search(r'reports/web-report-[\w-]+\.json', content)
            if match:
                self.pending_report_path = match.group(0)
                log_to_stderr(f"\n📋 Report will be created: {self.pending_report_path}\n")

        tool_log = format_tool_log(tool_name, tool_input)
        log_to_stderr(f"\n{tool_log}\n")
        await self.notifier.step_log(tool_log)

    async def _handle_content_block_start(self, event: dict) -> None:
        """Traite le début d'un bloc de contenu."""
        block = event.get("content_block", {})
        block_type = block.get("type", "")

        if block_type == "tool_use":
            log_to_stderr(f"\n⏳ Starting tool: {block.get('name', '')}...\n")
        elif block_type == "thinking":
            log_to_stderr("\n💭 [Claude réfléchit...]\n")

    async def _handle_content_block_delta(self, event: dict) -> None:
        """Traite le streaming de contenu."""
        delta = event.get("delta", {})
        delta_type = delta.get("type", "")

        if delta_type == "text_delta":
            text = delta.get("text", "")
            self.result_text += text
            log_to_stderr(text)
        elif delta_type == "thinking_delta":
            log_to_stderr(f"💭 {delta.get('thinking', '')}")

    async def _handle_user(self, event: dict) -> None:
        """Traite les messages utilisateur (résultats d'outils)."""
        content = event.get("message", {}).get("content", [])

        for block in content:
            if block.get("type") != "tool_result":
                continue

            tool_content = block.get("content", "")
            is_error = block.get("is_error", False)

            # Tronquer si trop long
            if isinstance(tool_content, str):
                display = tool_content[:300] + "..." if len(tool_content) > 300 else tool_content
            else:
                display = str(tool_content)[:300]

            if is_error:
                log_to_stderr(f"\n❌ Tool Error: {display}\n")
                await self.notifier.step_log(f"❌ Error: {display}", "stderr")
            else:
                log_to_stderr(f"\n✅ Tool Result: {display}\n")
                await self.notifier.step_log(f"✅ Result: {display[:200]}")

    async def _handle_system(self, event: dict) -> None:
        """Traite les messages système."""
        subtype = event.get("subtype", "")
        log_to_stderr(f"\n🔔 [System - {subtype}]\n")
        await self.notifier.step_log(f"[System] {subtype}")

    async def _handle_result(self, event: dict) -> None:
        """Traite le résultat final."""
        self.total_cost = event.get("cost_usd", 0.0)
        log_to_stderr(f"\n\n💰 Cost: ${self.total_cost:.4f}\n")

        # Transition vers la phase report
        await self.notifier.transition_to("report", "Analysis phases completed")
        await self.notifier.step_log(f"💰 Cost: ${self.total_cost:.4f}")

        # Envoyer le rapport JSON si détecté
        if self.pending_report_path:
            await self.notifier.step_log(f"📤 Sending report: {self.pending_report_path}")
            await self.notifier.report_ready(self.pending_report_path)
        else:
            log_to_stderr("\n⚠️ No report path detected\n")
            await self.notifier.step_log("⚠️ No report path detected", "stderr")

        # Finaliser
        await self.notifier.step_complete("success", f"## Result\n\n{self.result_text[:1000]}")
        await self.notifier.action_complete()

    async def _handle_error(self, event: dict) -> None:
        """Traite les erreurs."""
        error_msg = event.get("error", {}).get("message", "Unknown error")
        log_to_stderr(f"\n❌ Erreur: {error_msg}\n")
        await self.notifier.step_log(f"Error: {error_msg}", "stderr")
        await self.notifier.step_complete("failure", f"## Error\n\n{error_msg}")
        await self.notifier.action_complete()


# =============================================================================
# Fonction principale de streaming
# =============================================================================

async def read_stream(stream, processor: ClaudeEventProcessor) -> None:
    """Lit un stream de manière asynchrone et traite les événements."""
    while True:
        line = await stream.readline()
        if not line:
            break

        line = line.decode('utf-8', errors='replace').strip()
        if not line:
            continue

        try:
            event = json.loads(line)
            await processor.process_event(event)
        except json.JSONDecodeError:
            log_to_stderr(f"{line}\n")


async def read_stderr(stream) -> str:
    """Lit stderr de manière asynchrone."""
    output = []
    while True:
        line = await stream.readline()
        if not line:
            break
        decoded = line.decode('utf-8', errors='replace')
        output.append(decoded)
        log_to_stderr(f"[stderr] {decoded}")
    return "".join(output)


async def launch_claude_stream_json(
    prompt: str,
    request,
    max_timeout: int = 7200,
    verbosity: int = 1,
    ws_url: str = "ws://192.36.128.57:8080/api/ws/jobs"
) -> dict:
    """
    Lance Claude en mode stream-json avec architecture async native.

    Args:
        prompt: Le prompt complet à envoyer à Claude
        request: L'objet request contenant les infos du job
        max_timeout: Timeout maximum en secondes (défaut: 30 minutes)
        verbosity: Niveau de verbosité (0=minimal, 1=normal, 2=debug)
        ws_url: URL du serveur WebSocket

    Returns:
        dict avec 'success', 'result', 'cost', 'messages'
    """
    cmd = [
        "claude",
        "-p", prompt,
        "--dangerously-skip-permissions",
        "--output-format", "stream-json",
        "--verbose"
    ]

    try:
        log_to_stderr("🚀 Starting ou (stream-json)...\n")

        # Connexion WebSocket avec timeout
        async with asyncio.timeout(60):
            websocket = await websockets.connect(ws_url + f"/{request.jobId}")

        async with websocket:
            notifier = WebSocketNotifier(websocket)
            processor = ClaudeEventProcessor(notifier, verbosity)

            # Démarrer l'action
            await notifier.action_started()
            await notifier.step_started("setup")
            await notifier.step_log("Initializing Claude stream-json...")
            # auto_next=True démarre automatiquement phase1 après setup
            await notifier.step_complete("success", "Setup completed", auto_next=True)
            # Les phases suivantes (phase2_risk, etc.) transitionnent automatiquement
            # quand les agents Task sont détectés via on_agent_started()

            # Lancer le processus Claude
            # limit=1MB pour éviter "Separator is found, but chunk is longer than limit"
            # quand Claude génère des lignes JSON très longues (rapports avec markdown)
            process = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                env=dict(os.environ),  # Hérite du PATH complet (fnm, nvm, etc.)
                limit=2**20  # 1MB au lieu de 64KB par défaut
            )

            try:
                # Lire stdout et stderr en parallèle avec timeout
                async with asyncio.timeout(max_timeout):
                    await asyncio.gather(
                        read_stream(process.stdout, processor),
                        read_stderr(process.stderr)
                    )
                    await process.wait()

            except asyncio.TimeoutError:
                log_to_stderr(f"\n⏱️ Timeout atteint ({max_timeout}s)\n")
                await notifier.step_log(f"Timeout after {max_timeout}s", "stderr")
                await notifier.step_complete("failure", f"Timeout after {max_timeout}s")
                await notifier.action_complete()
                process.kill()
                await process.wait()

                return {
                    "success": False,
                    "result": processor.result_text,
                    "cost": processor.total_cost,
                    "messages": processor.messages,
                    "error": f"Timeout after {max_timeout}s"
                }

            # Finaliser si pas encore fait
            if not notifier.action_completed:
                if notifier.current_step:
                    await notifier.step_complete(notifier.action_status, "Process completed")
                await notifier.action_complete()

            return {
                "success": process.returncode == 0,
                "result": processor.result_text,
                "cost": processor.total_cost,
                "messages": processor.messages,
                "returncode": process.returncode
            }

    except websockets.exceptions.WebSocketException as e:
        log_to_stderr(f"\n❌ WebSocket error: {e}\n")
        return {
            "success": False,
            "result": "",
            "cost": 0.0,
            "messages": [],
            "error": f"WebSocket error: {e}"
        }

    except Exception as e:
        log_to_stderr(f"\n❌ Error: {type(e).__name__}: {e}\n")
        return {
            "success": False,
            "result": "",
            "cost": 0.0,
            "messages": [],
            "error": str(e)
        }


# =============================================================================
# Point d'entrée pour lancer Claude
# =============================================================================

def launch_claude_process(request, verbosity: int = 1) -> Optional[dict]:
    """
    Lance Claude avec le prompt d'analyse (analyze_py.md).
    Point d'entrée synchrone qui lance la coroutine async.

    Args:
        request: L'objet request avec les infos du job
        verbosity: Niveau de verbosité

    Returns:
        Le résultat du process Claude ou None si erreur
    """
    prompt_path = Path(".claude/commands/analyze_py.md")

    if not prompt_path.exists():
        log_to_stderr(f"❌ Fichier prompt non trouvé: {prompt_path}\n")
        return None

    prompt = prompt_path.read_text()

    if request.action == 'issue_detector':
        return asyncio.run(launch_claude_stream_json(
            prompt=prompt,
            request=request,
            max_timeout=7200,
            verbosity=verbosity
        ))

    return None
