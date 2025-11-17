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
     * Submit a job to the builder
     * @param command 'build' or 'compile'
     * @return job UUID
     */
    String submitJob(String command) {
        script.echo "📤 Soumission du job: ${command}"

        def response = script.sh(
            script: """
                curl -s -X POST \
                    -H 'Content-Type: application/json' \
                    -d '{"command":"${command}"}' \
                    ${baseUrl}/run
            """,
            returnStdout: true
        ).trim()

        def json = parseJson(response)

        if (!json.job_id) {
            script.error("Failed to submit job: ${response}")
        }

        script.echo "✅ Job soumis avec UUID: ${json.job_id}"
        return json.job_id
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
     * Wait for job completion with polling
     * @param jobId UUID of the job
     * @param pollInterval seconds between status checks (default: 5)
     * @param maxWaitTime maximum wait time in seconds (default: 3600 = 1h)
     * @return final job status Map
     */
    Map waitForCompletion(String jobId, int pollInterval = 5, int maxWaitTime = 3600) {
        script.echo "⏳ Attente de la completion du job ${jobId}..."
        
        def startTime = System.currentTimeMillis()
        def status = null
        
        while (true) {
            status = getStatus(jobId)
            
            script.echo "📊 Status: ${status.status} | Command: ${status.command}"
            
            if (status.status == 'finished' || status.status == 'failed') {
                break
            }
            
            def elapsed = (System.currentTimeMillis() - startTime) / 1000
            if (elapsed > maxWaitTime) {
                script.error("Timeout waiting for job ${jobId} (waited ${elapsed}s)")
            }
            
            script.sleep(pollInterval)
        }
        
        return status
    }
    
    /**
     * Submit job and wait for completion
     * @param command 'build' or 'compile'
     * @param pollInterval seconds between checks
     * @param maxWaitTime maximum wait time
     * @return final job status
     */
    Map runAndWait(String command, int pollInterval = 5, int maxWaitTime = 3600) {
        def jobId = submitJob(command)
        def status = waitForCompletion(jobId, pollInterval, maxWaitTime)
        
        script.echo "🏁 Job terminé avec returncode: ${status.returncode}"
        
        // Display last 50 lines of log
        if (status.returncode != 0) {
            script.echo "📋 Dernières lignes du log:"
            def statusWithLog = getStatus(jobId, 50)
            if (statusWithLog.log_tail) {
                script.echo statusWithLog.log_tail
            }
        }
        
        // Fail the pipeline if the job failed
        if (status.returncode != 0) {
            script.error("Job '${command}' failed with returncode ${status.returncode}")
        }
        
        return status
    }
    
    /**
     * Submit a job in a specific workspace
     * @param workspaceId workspace identifier (e.g., "build_123")
     * @param command 'build' or 'compile'
     * @return job UUID
     */
    String runInWorkspace(String workspaceId, String command) {
        script.echo "📤 Soumission du job '${command}' dans workspace '${workspaceId}'..."

        // Retirer -f pour voir le contenu même en cas d'erreur HTTP
        def response = script.sh(
            script: """
                curl -s -w '\\nHTTP_CODE:%{http_code}' -X POST \
                    -H 'Content-Type: application/json' \
                    -d '{"command":"${command}"}' \
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
}

// Factory method to create a BuilderAPI instance from the loaded script
def create(scriptArg, host = 'localhost', port = 8082) {
    return new BuilderAPI(scriptArg, host, port)
}

return this
