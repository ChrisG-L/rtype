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

        stage('Debug') {
            steps {
                echo '🔍 Vérification structure...'
                sh 'ls -la scripts/'
                sh 'ls -la scripts/vcpkg/'
                sh 'find scripts/ -type f'
                sh 'pwd'  // Voir le vrai chemin
                // Vérification dans le conteneur
                sh '''
                    docker run --rm \
                        -v "$(pwd)":/workspace \
                        -w /workspace \
                        rtype-builder:latest \
                        bash -c "echo '=== Dans le conteneur ===' && pwd && ls -la && ls -la scripts/"
                '''
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
                        bash -c "ls -la && ./scripts/vcpkg/install_vcpkg.sh"
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