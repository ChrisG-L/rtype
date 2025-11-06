pipeline {
    agent any
    
    // Définir les paramètres du pipeline
    parameters {
        string(name: "BRANCH", defaultValue: 'main', description: 'Branche à builder')
    }
    
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
        
        stage('Install System Dependencies') {
            steps {
                echo '🔧 Installation des dépendances système...'
                sh '''
                    # Vérifier si on a les droits root
                    if [ "$(id -u)" = "0" ]; then
                        apt-get update
                        apt-get install -y \
                            build-essential \
                            cmake \
                            git \
                            curl \
                            zip \
                            unzip \
                            tar \
                            pkg-config \
                            ninja-build
                    else
                        echo "⚠️  Pas de droits root, vérification des outils..."
                        command -v cmake || echo "❌ cmake manquant"
                        command -v g++ || echo "❌ g++ manquant"
                        command -v zip || echo "❌ zip manquant"
                        command -v unzip || echo "❌ unzip manquant"
                    fi
                '''
            }
        }
        
        stage('Install vcpkg') {
            steps {
                echo '📦 Installation de vcpkg...'
                sh '''
                    chmod +x scripts/vcpkg/install_vcpkg.sh
                    ./scripts/vcpkg/install_vcpkg.sh
                '''
            }
        }
        
        stage('Install Project Dependencies') {
            steps {
                echo '📚 Installation des dépendances du projet via vcpkg...'
                sh '''
                    chmod +x scripts/vcpkg/vcpkg.sh
                    
                    # Installer les dépendances depuis vcpkg.json
                    if [ -f "vcpkg.json" ]; then
                        echo "📄 Installation depuis vcpkg.json..."
                        ./scripts/vcpkg/vcpkg.sh install
                    else
                        echo "⚠️  Pas de vcpkg.json trouvé"
                    fi
                '''
            }
        }
        
        stage('Build') {
            steps {
                echo '🔨 Compilation du projet...'
                sh '''
                    chmod +x scripts/build.sh
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
            echo '✅ Build réussi !'
        }
        failure {
            echo '❌ Build échoué !'
        }
    }
}