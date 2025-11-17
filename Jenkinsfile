@Library('shared-library') _

pipeline {
    agent any

    // Déclencheur pour surveiller les changements dans le SCM toutes les 3 minutes
    triggers {
        pollSCM("H/3 * * * *")
    }
    
    // Options globales pour le pipeline
    options {
        timeout(time: 2, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '10'))
    }
    
    stages {
        stage('Checkout') {
            steps {
                echo '📥 Récupération du code source...'
                checkout scm
            }
        }
        
        stage('Launch Build Container') {
            steps {
                script {
                    echo '🐳 Lancement du conteneur builder...'
                    sh 'docker-compose -f ci_cd/docker/docker-compose.build.yml up -d --build'
                    
                    // Wait for container to be ready
                    echo '⏳ Attente du démarrage du serveur builder...'
                    sleep(time: 10, unit: 'SECONDS')
                }
            }
        }
        
        stage('Health Check') {
            steps {
                script {
                    echo '🏥 Vérification de la santé du builder...'
                    def builderAPI = load('ci_cd/jenkins/BuilderAPI.groovy')
                    def api = new builderAPI.BuilderAPI(this, 'localhost', 8080)
                    
                    retry(5) {
                        if (!api.healthCheck()) {
                            sleep(time: 5, unit: 'SECONDS')
                            error('Builder not healthy')
                        }
                    }
                    echo '✅ Builder opérationnel'
                }
            }
        }
        
        stage('Build Project') {
            steps {
                script {
                    echo '🔨 Lancement de la compilation via API...'
                    def builderAPI = load('ci_cd/jenkins/BuilderAPI.groovy')
                    def api = new builderAPI.BuilderAPI(this, 'localhost', 8080)
                    
                    // Submit build job and wait for completion
                    // Poll every 10 seconds, max 2 hours
                    def result = api.runAndWait('build', 10, 7200)
                    
                    echo "✅ Build terminé avec succès (returncode: ${result.returncode})"
                }
            }
        }
    }

    post {
        always {
            script {
                echo '🧹 Nettoyage...'
                // Stop and remove the builder container
                sh 'docker-compose -f ci_cd/docker/docker-compose.build.yml down || true'
            }
            echo '🏁 Pipeline terminé'
        }
        success {
            echo '✅ Build réussi !'
        }
        failure {
            echo '❌ Build échoué !'
        }
    }
}