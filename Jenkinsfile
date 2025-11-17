def builderPort = ""
def buildPrefix = ""

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

    // Paramètres du pipeline
    parameters {
        booleanParam(name: 'BUILD_IMAGE', defaultValue: false, description: 'Construire l\'image `rtype-builder:latest` avant de lancer le conteneur')
    }

    stages {
        stage('Checkout') {
            steps {
                echo '📥 Récupération du code source...'
                checkout scm
            }
        }

        stage('Setup Build Environment') {
            steps {
                script {
                    // Calculer les variables de build
                    buildPrefix = "build_${env.BUILD_NUMBER}_"
                    def buildNumber = env.BUILD_NUMBER ? env.BUILD_NUMBER.toInteger() : 0
                    builderPort = (8082 + (buildNumber % 1000)).toString()
                    
                    echo "🔧 Configuration de l'environnement de build"
                    echo "   PREFIX: ${buildPrefix}"
                    echo "   PORT: ${builderPort}"
                    echo "   Container: ${buildPrefix}rtype_builder"
                }
            }
        }

        stage('Launch Build Container') {
            steps {
                script {
                    echo '🐳 Lancement du conteneur builder...'

                    // Optionnel: reconstruire l'image si demandé
                    if (params.BUILD_IMAGE) {
                        echo '📦 Construction de l\'image rtype-builder:latest demandée'
                        sh """
                            cd ci_cd/docker
                            chmod +x build_image.sh
                            ./build_image.sh
                        """
                    }

                    // Lancer le builder avec le script
                    sh """
                        cd ci_cd/docker
                        chmod +x launch_builder.sh
                        ./launch_builder.sh ${buildPrefix} ${builderPort}
                    """

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
                    echo "DEBUG: builderAPI object: ${builderAPI}"
                    echo "DEBUG: builderAPI class: ${builderAPI.getClass().name}"
                    try {
                        echo "DEBUG: builderAPI methods: ${builderAPI.getClass().methods*.name.sort().unique()}"
                    } catch (e) {
                        echo "DEBUG: failed to list builderAPI methods: ${e}"
                    }
                    def api = builderAPI.create(this, 'localhost', builderPort.toInteger())

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
                    echo "DEBUG: builderAPI object: ${builderAPI}"
                    echo "DEBUG: builderAPI class: ${builderAPI.getClass().name}"
                    try {
                        echo "DEBUG: builderAPI methods: ${builderAPI.getClass().methods*.name.sort().unique()}"
                    } catch (e) {
                        echo "DEBUG: failed to list builderAPI methods: ${e}"
                    }
                    def api = builderAPI.create(this, 'localhost', builderPort.toInteger())

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
                    sh """
                        cd ci_cd/docker
                        chmod +x stop_builder.sh
                    """
                    echo '🏁 Pipeline terminé'
                }
        }
        success {
            echo '✅ Build réussi !'
        }
        failure {
            echo '❌ Build échoué !'
        }
    }
}