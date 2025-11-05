pipeline {
    
    agent an

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
                    sh 'docker-compose -f docker-compose.build.yml build'
                }
            }
        }

        stage('Install Dependencies') {
            agent {
                docker {
                    image 'rtype-builder:latest'
                    reuseNode true
                }
            }
            steps {
                echo '📦 Installation des dépendances...'
                sh './scripts/install_vcpkg.sh'
                sh './scripts/vcpkg.sh install'
            }
        }

        stage('Build') {
            steps {
                sh './scripts/vcpkg.sh install'
                sh './scripts/build.sh'
            }
        }

        stage('Build') {
            agent {
                docker {
                    image 'rtype-builder:latest'
                    reuseNode true
                }
            }
            steps {
                echo '🏗️ Compilation...'
                sh './scripts/build.sh'
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