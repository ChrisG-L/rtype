# main.py
"""
API FastAPI pour déclencher les analyses Claude.
"""

import asyncio
from pathlib import Path

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import Literal, Optional
from datetime import datetime
from enum import Enum

from claude import switch_branch, launch_claude_stream_json, log_to_stderr


# =============================================================================
# Modèles
# =============================================================================

class JobStatus(str, Enum):
    """Statuts possibles d'un job."""
    PENDING = "pending"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"


class ClaudeRequest(BaseModel):
    """Requête pour lancer une analyse Claude."""
    jobId: str
    branchName: str
    commitSha: str
    action: Literal['build_i4gen', 'build_compact', 'issue_detector']


class JobInfo(BaseModel):
    """Informations sur un job."""
    jobId: str
    status: JobStatus
    started_at: datetime
    completed_at: Optional[datetime] = None
    result: Optional[dict] = None
    error: Optional[str] = None


# =============================================================================
# État des jobs (en mémoire)
# =============================================================================

jobs: dict[str, JobInfo] = {}
running_tasks: set[asyncio.Task] = set()  # Garde les références pour éviter le GC


# =============================================================================
# Application FastAPI
# =============================================================================

app = FastAPI(
    title="Claude Analysis API",
    description="API pour déclencher des analyses de code avec Claude",
    version="1.0.0",
    
)


# =============================================================================
# Tâches en arrière-plan
# =============================================================================

async def run_claude_analysis(request: ClaudeRequest) -> None:
    """
    Exécute l'analyse Claude en arrière-plan (async).
    Met à jour le statut du job au fur et à mesure.
    """
    job_id = request.jobId

    # Marquer comme en cours
    if job_id in jobs:
        jobs[job_id].status = JobStatus.RUNNING

    try:
        log_to_stderr(f"[Job {job_id}] Starting Claude analysis...\n")

        # Charger le prompt
        prompt_path = Path(".claude/commands/analyze_py.md")
        if not prompt_path.exists():
            raise FileNotFoundError(f"Prompt file not found: {prompt_path}")

        prompt = prompt_path.read_text()

        # Lancer l'analyse async (ne bloque plus le serveur)
        result = await launch_claude_stream_json(
            prompt=prompt,
            request=request,
            max_timeout=7200,
            verbosity=1
        )

        # Mettre à jour avec le résultat
        if job_id in jobs:
            jobs[job_id].completed_at = datetime.now()
            if result and result.get("success"):
                jobs[job_id].status = JobStatus.COMPLETED
                jobs[job_id].result = result
            else:
                jobs[job_id].status = JobStatus.FAILED
                jobs[job_id].error = result.get("error") if result else "Unknown error"

        log_to_stderr(f"[Job {job_id}] Analysis completed.\n")

    except Exception as e:
        log_to_stderr(f"[Job {job_id}] Error: {e}\n")
        if job_id in jobs:
            jobs[job_id].status = JobStatus.FAILED
            jobs[job_id].error = str(e)
            jobs[job_id].completed_at = datetime.now()


# =============================================================================
# Routes
# =============================================================================

@app.get("/")
def read_root():
    """Health check."""
    return {"status": "ok", "service": "Claude Analysis API"}


@app.post("/trigger")
async def launch_claude(request: ClaudeRequest):
    """
    Déclenche une analyse Claude en arrière-plan.

    Returns:
        Informations sur le job créé
    """
    # Vérifier si le job existe déjà
    if request.jobId in jobs:
        existing = jobs[request.jobId]
        if existing.status in [JobStatus.PENDING, JobStatus.RUNNING]:
            raise HTTPException(
                status_code=409,
                detail=f"Job {request.jobId} is already {existing.status.value}"
            )

    # Changer de branche
    if not switch_branch(request.branchName):
        raise HTTPException(
            status_code=400,
            detail=f"Failed to switch to branch: {request.branchName}"
        )

    # Logger les infos
    log_to_stderr(f"Lancement de Claude pour la branche: {request.branchName}\n")
    log_to_stderr(f"Commit: {request.commitSha}\n")
    log_to_stderr(f"Job ID: {request.jobId}\n")
    log_to_stderr(f"Action: {request.action}\n")
    log_to_stderr("=" * 60 + "\n\n")

    # Créer l'entrée du job
    jobs[request.jobId] = JobInfo(
        jobId=request.jobId,
        status=JobStatus.PENDING,
        started_at=datetime.now()
    )

    # Lancer en arrière-plan avec asyncio (non-bloquant)
    if request.action == 'issue_detector':
        task = asyncio.create_task(run_claude_analysis(request))
        running_tasks.add(task)
        task.add_done_callback(running_tasks.discard)  # Nettoie quand terminé
    else:
        raise HTTPException(
            status_code=400,
            detail=f"Unsupported action: {request.action}"
        )

    return {
        "jobId": request.jobId,
        "status": "accepted",
        "message": "Job accepted and queued for processing"
    }


@app.get("/jobs/{job_id}")
def get_job_status(job_id: str):
    """
    Récupère le statut d'un job.

    Args:
        job_id: L'identifiant du job

    Returns:
        Informations sur le job
    """
    if job_id not in jobs:
        raise HTTPException(status_code=404, detail=f"Job {job_id} not found")

    job = jobs[job_id]
    return {
        "jobId": job.jobId,
        "status": job.status.value,
        "started_at": job.started_at.isoformat(),
        "completed_at": job.completed_at.isoformat() if job.completed_at else None,
        "error": job.error
    }


@app.get("/jobs")
def list_jobs():
    """
    Liste tous les jobs.

    Returns:
        Liste des jobs avec leur statut
    """
    return {
        "jobs": [
            {
                "jobId": job.jobId,
                "status": job.status.value,
                "started_at": job.started_at.isoformat()
            }
            for job in jobs.values()
        ],
        "total": len(jobs)
    }
