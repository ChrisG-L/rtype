/**
 * Helper class to interact with the Python Builder API
 * Provides methods to submit build/compile jobs and wait for completion
 */
class BuilderAPI implements Serializable {
    def script
    String baseUrl

    BuilderAPI(script, String host = 'localhost', int port = 8082) {
        this.script = script
        this.baseUrl = "http://${host}:${port}"
    }

    /**
     * Parse JSON string using Groovy's native JsonSlurperClassic (no plugin needed)
     * Note: Using JsonSlurperClassic instead of JsonSlurper because it returns
     * serializable objects (HashMap) instead of LazyMap, which is required for
     * Jenkins Pipeline serialization.
     */
    def parseJson(String jsonString) {
        def jsonSlurper = new groovy.json.JsonSlurperClassic()
        return jsonSlurper.parseText(jsonString)
    }

    /**
     * Get job status
     * @param jobId UUID of the job
     * @param tailLines number of log lines to retrieve (optional)
     * @return Map with status info
     */
    Map getStatus(String jobId, Integer tailLines = null) {
        def url = "${baseUrl}/status/${jobId}"
        if (tailLines) {
            url += "?tail=${tailLines}"
        }

        def response = script.sh(
            script: "curl -s ${url}",
            returnStdout: true
        ).trim()

        return parseJson(response)
    }

    /**
     * Submit a job in a specific workspace
     * @param workspaceId workspace identifier (e.g., "build_123")
     * @param command 'build' or 'compile'
     * @param args optional command arguments (e.g., '--platform=windows')
     * @return job UUID
     */
    String runInWorkspace(String workspaceId, String command, String args = '') {
        def commandWithArgs = args ? "${command} ${args}" : command
        script.echo "📤 Soumission du job '${commandWithArgs}' dans workspace '${workspaceId}'..."

        // Retirer -f pour voir le contenu même en cas d'erreur HTTP
        def response = script.sh(
            script: """
                curl -s -w '\\nHTTP_CODE:%{http_code}' -X POST \
                    -H 'Content-Type: application/json' \
                    -d '{"command":"${commandWithArgs}"}' \
                    ${baseUrl}/workspace/${workspaceId}/run
            """,
            returnStdout: true
        ).trim()

        // Séparer le body et le code HTTP
        def parts = response.split('\nHTTP_CODE:')
        def body = parts[0]
        def httpCode = parts.length > 1 ? parts[1] : '000'

        script.echo "Response HTTP ${httpCode}: ${body}"

        if (httpCode != '200') {
            script.error("HTTP ${httpCode} error when submitting job: ${body}")
        }

        def json = parseJson(body)

        if (!json.job_id) {
            script.error("Failed to submit job in workspace: ${body}")
        }

        script.echo "✅ Job soumis avec UUID: ${json.job_id}"
        return json.job_id
    }

    /**
     * Wait for job completion (alias for waitForCompletion with better name)
     * @param jobId UUID of the job
     * @param pollInterval seconds between status checks (default: 10)
     * @param maxWaitTime maximum wait time in seconds (default: 7200 = 2h)
     * @return final job status Map
     */
    Map waitForJob(String jobId, int pollInterval = 10, int maxWaitTime = 7200) {
        script.echo "⏳ Attente de la completion du job ${jobId}..."

        def startTime = System.currentTimeMillis()
        def status = null

        while (true) {
            status = getStatus(jobId, 20)  // Get last 20 lines of log

            script.echo "📊 Status: ${status.status} | Command: ${status.command}"

            // Afficher les dernières lignes du log si disponibles
            if (status.log_tail) {
                script.echo "📋 Dernières lignes du log:"
                script.echo status.log_tail
            }

            if (status.status == 'finished') {
                if (status.returncode == 0) {
                    script.echo "✅ Job terminé avec succès"
                    return status
                } else {
                    script.echo "❌ Job échoué avec returncode: ${status.returncode}"
                    script.error("Job failed with returncode ${status.returncode}")
                }
            }

            if (status.status == 'failed') {
                script.echo "❌ Job failed: ${status}"
                script.error("Job failed with status: ${status.status}")
            }

            def elapsed = (System.currentTimeMillis() - startTime) / 1000
            if (elapsed > maxWaitTime) {
                script.error("Timeout waiting for job ${jobId} (waited ${elapsed}s)")
            }

            script.sleep(pollInterval)
        }
    }

    /**
     * Check if builder is healthy
     * @return true if healthy
     */
    boolean healthCheck() {
        try {
            def response = script.sh(
                script: "curl -s ${baseUrl}/health",
                returnStdout: true
            ).trim()

            def json = parseJson(response)
            return json.status == 'ok'
        } catch (Exception e) {
            script.echo "❌ Health check failed: ${e.message}"
            return false
        }
    }

    /**
     * Get list of artifacts from a workspace
     * @param workspaceId workspace identifier (e.g., "build_123")
     * @return Map with artifacts list and rsync path
     */
    Map getArtifacts(String workspaceId) {
        script.echo "📦 Récupération de la liste des artefacts du workspace '${workspaceId}'..."

        def response = script.sh(
            script: "curl -s -f ${baseUrl}/workspace/${workspaceId}/artifacts",
            returnStdout: true
        ).trim()

        def json = parseJson(response)
        script.echo "✅ ${json.artifacts.size()} artefact(s) trouvé(s)"

        return json
    }

    /**
     * Download artifacts from a workspace using rsync
     * @param workspaceId workspace identifier
     * @param localPath local destination path
     * @param excludePatterns optional list of patterns to exclude (default: [])
     * @return number of artifacts downloaded
     */
    int downloadArtifacts(String workspaceId, String localPath, List<String> excludePatterns = []) {
        script.echo "📥 Téléchargement des artefacts du workspace '${workspaceId}' vers '${localPath}'..."

        // Get artifacts info
        def artifactsInfo = getArtifacts(workspaceId)

        if (artifactsInfo.artifacts.size() == 0) {
            script.echo "⚠️  Aucun artefact à télécharger"
            return 0
        }

        // Log artifacts directory BEFORE mkdir
        script.echo "📋 Contenu du dossier artifacts AVANT mkdir:"
        script.sh """
            ls -la ${localPath}/../ || echo "Dossier parent n'existe pas encore"
        """

        // Create local directory
        script.sh "mkdir -p ${localPath}"

        // Log artifacts directory BEFORE rsync
        script.echo "📋 Contenu du dossier artifacts AVANT rsync:"
        script.sh """
            ls -la ${localPath}/../ || echo "Dossier parent n'existe pas encore"
        """

        // Build rsync exclude arguments
        def excludeArgs = excludePatterns.collect { "--exclude='${it}'" }.join(' ')

        // Download via rsync
        script.sh """
            rsync -avz ${excludeArgs} \
                ${artifactsInfo.rsync_path} \
                ${localPath}/
        """

        // Log artifacts directory AFTER rsync
        script.echo "📋 Contenu du dossier artifacts APRÈS rsync:"
        script.sh """
            ls -la ${localPath}/../
        """

        script.echo "✅ ${artifactsInfo.artifacts.size()} artefact(s) téléchargé(s)"
        return artifactsInfo.artifacts.size()
    }
}

// Factory method to create a BuilderAPI instance from the loaded script
def create(scriptArg, host = 'localhost', port = 8082) {
    return new BuilderAPI(scriptArg, host, port)
}

return this
