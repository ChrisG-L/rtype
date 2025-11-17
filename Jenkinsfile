pipeline {
    agent any

    // Déclencheur pour surveiller les changements dans le SCM toutes les 3 minutes
    triggers {
        pollSCM("H/3 * * * *")
    }
    
    // Options globales pour le pipeline
    options {
        timeout(time: 1, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '10'))
    }
    
    stages {
        stage('Checkout') {
            steps {
                echo '📥 Récupération du code source...'
                checkout scm
            }
        }
        
    stages {
        stage('Launch Build Container') {
            steps {
                echo '🐳 Lancement du conteneur de build...'
                sh 'docker-compose -f ci_cd/docker/docker-compose.build.yml up -d --build'
            }
        }
        stage('Build Project') {
            steps {
                echo ('🔨 Compilation du projet...')
                sh 'docker exec rtype_builder ./scripts/compile.sh'
            }
        }
    }

    post {
        always {
            echo '🧹 Pipeline terminé'
        }
        success {
            echo '✅ Build réussi !'
        }
        failure {
            echo '❌ Build échoué !'
        }
    }
}