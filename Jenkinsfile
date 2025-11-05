pipeline {
    agent any

    parameters {
        string(name: "BRANCH", defaultValue: 'main', description: 'Branche à builder')
    }

    triggers {
        pollSCM("H/15 * * * *")
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

        stage('Build') {
            steps {
                echo 'Je builde!'
            }
        }
    }

    post {
        always {
            echo 'Pipeline terminé'
        }
        success {
            echo 'Succès!'
        }
        failure {
            echo 'Echec!'
        }
    }
}