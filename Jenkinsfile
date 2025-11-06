pipeline {
    agent any
    
    parameters {
        string(name: "BRANCH", defaultValue: 'main', description: 'Branche à builder')
    }
    
    triggers {
        pollSCM("H/3 * * * *")
    }
    
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
        
        stage('Install Dependencies') {
            steps {
                echo '📦 Installation des dépendances...'
                sh './scripts/vcpkg/install_vcpkg.sh'
            }
        }
        
        stage('Build') {
            steps {
                echo '🔨 Compilation du projet...'
                sh './scripts/build.sh'
            }
        }
    }
    
    post {
        always {
            echo '🧹 Pipeline terminé'
        }
        success {
            echo '✅ Succès !'
        }
        failure {
            echo '❌ Échec !'
        }
    }
}