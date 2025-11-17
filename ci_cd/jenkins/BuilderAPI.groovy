/**
 * Helper class to interact with the Python Builder API
 * Provides methods to submit build/compile jobs and wait for completion
 */
class BuilderAPI implements Serializable {
    def script
    String baseUrl
    
    BuilderAPI(script, String host = 'localhost', int port = 8080) {
        this.script = script
        this.baseUrl = "http://${host}:${port}"
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
        
        def json = script.readJSON(text: response)
        
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
        
        return script.readJSON(text: response)
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
     * Check if builder is healthy
     * @return true if healthy
     */
    boolean healthCheck() {
        try {
            def response = script.sh(
                script: "curl -s ${baseUrl}/health",
                returnStdout: true
            ).trim()
            
            def json = script.readJSON(text: response)
            return json.status == 'ok'
        } catch (Exception e) {
            script.echo "❌ Health check failed: ${e.message}"
            return false
        }
    }
}

// Factory closure to create a BuilderAPI instance from the loaded script
def create = { scriptArg, host = 'localhost', port = 8080 ->
    return new BuilderAPI(scriptArg, host, port)
}

return this
