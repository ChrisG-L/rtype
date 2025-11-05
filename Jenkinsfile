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

        stage('Build Docker Image') {
            steps {
                echo '🐳 Construction de l\'image Docker...'
                dir('ci_cd/docker') {
                    sh 'docker compose -f docker-compose.build.yml build'
                }
            }
        }

        stage('Install Dependencies') {
            steps {
                echo '📦 Installation des dépendances...'
                sh '''
                docker run --rm \
                    -v "$(pwd)":/workspace \
                    -w /workspace \
                    rtype-builder:latest \
                    ./scripts/vcpkg/vcpkg.sh install
                '''
            }
        }

        stage('Build') {
            steps {
                echo '🏗️ Compilation...'
                sh '''
                    docker run --rm \
                        -v "$(pwd)":/workspace \
                        -w /workspace \
                        rtype-builder:latest \
                        ./scripts/build.sh
                '''
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