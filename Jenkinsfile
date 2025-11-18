def builderAPI

pipeline {
    agent any

    /* // Déclencheur pour surveiller les changements dans le SCM toutes les 3 minutes
    triggers {
        pollSCM("H/3 * * * *")
    } */

    // Options globales pour le pipeline
    options {
        timeout(time: 2, unit: 'HOURS')
        buildDiscarder(logRotator(numToKeepStr: '10'))
    }

    environment {
        BUILDER_HOST = "rtype_builder"
        BUILDER_PORT = "8082"
        WORKSPACE_ID = "build_${BUILD_NUMBER}"
    }

    stages {
        stage('Checkout') {
            steps {
                echo '📥 Récupération du code source...'
                checkout scm
            }
        }

        stage('📋 Create Workspace') {
            steps {
                script {
                    echo "📋 Création du workspace ${env.WORKSPACE_ID} sur le builder..."

                    builderAPI = load('ci_cd/jenkins/BuilderAPI.groovy')
                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                    // Vérifier que le builder est accessible
                    if (!api.healthCheck()) {
                        error("❌ Le builder permanent n'est pas accessible. Lancez d'abord le job d'initialisation (Jenkinsfile.init).")
                    }

                    // Créer workspace via API
                    def createResponse = sh(
                        script: """
                            curl -s -f -X POST http://${env.BUILDER_HOST}:${env.BUILDER_PORT}/workspace/create \
                                -H 'Content-Type: application/json' \
                                -d '{"build_number": ${env.BUILD_NUMBER}}'
                        """,
                        returnStdout: true
                    ).trim()

                    echo "✅ Workspace créé: ${createResponse}"
                }
            }
        }

        stage('📤 Upload Source Code') {
            steps {
                script {
                    echo "📤 Upload du code source via rsync..."

                    // Utiliser rsync pour transférer le code vers le builder
                    sh """
                        rsync -avz --delete \
                            --exclude='.git' \
                            --exclude='build/*' \
                            --exclude='cmake-build-*' \
                            --exclude='*.o' \
                            --exclude='*.a' \
                            --exclude='.gitignore' \
                            --exclude='third_party/vcpkg/.git' \
                            --exclude='artifacts' \
                            ${WORKSPACE}/ \
                            rsync://${env.BUILDER_HOST}:873/workspace/${env.WORKSPACE_ID}/
                    """

                    echo "✅ Code source uploadé (diff seulement grâce à rsync)"
                }
            }
        }

        stage('🏗️  Build Matrix (Linux + Windows)') {
            parallel {
                stage('🐧 Linux Build') {
                    stages {
                        stage('🔨 Build Linux') {
                            steps {
                                script {
                                    echo '🔨 [LINUX] Configuration CMake et vcpkg...'

                                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                    // Lancer le build dans le workspace (plateforme linux par défaut)
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID, 'build')

                                    echo "[LINUX] Job créé: ${jobId}"

                                    // Attendre la fin du build
                                    def result = api.waitForJob(jobId, 10, 7200)

                                    echo "✅ [LINUX] Build terminé avec succès"
                                }
                            }
                        }

                        stage('🔧 Compile Linux') {
                            steps {
                                script {
                                    echo '🔧 [LINUX] Compilation du projet...'

                                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                    // Lancer la compilation dans le workspace
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID, 'compile')

                                    echo "[LINUX] Job créé: ${jobId}"

                                    // Attendre la fin de la compilation
                                    def result = api.waitForJob(jobId, 10, 7200)

                                    echo "✅ [LINUX] Compilation terminée avec succès"
                                }
                            }
                        }
                    }
                }

                stage('🪟 Windows Build (cross-compilation)') {
                    stages {
                        stage('🔨 Build Windows') {
                            steps {
                                script {
                                    echo '🔨 [WINDOWS] Configuration CMake et vcpkg avec MinGW...'

                                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                    // Lancer le build Windows dans le workspace avec --platform=windows
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID, 'build', '--platform=windows')

                                    echo "[WINDOWS] Job créé: ${jobId}"

                                    // Attendre la fin du build
                                    def result = api.waitForJob(jobId, 10, 7200)

                                    echo "✅ [WINDOWS] Build terminé avec succès"
                                }
                            }
                        }

                        stage('🔧 Compile Windows') {
                            steps {
                                script {
                                    echo '🔧 [WINDOWS] Compilation du projet avec MinGW...'

                                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                    // Lancer la compilation dans le workspace
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID, 'compile')

                                    echo "[WINDOWS] Job créé: ${jobId}"

                                    // Attendre la fin de la compilation
                                    def result = api.waitForJob(jobId, 10, 7200)

                                    echo "✅ [WINDOWS] Compilation terminée avec succès"
                                }
                            }
                        }
                    }
                }
            }
        }

        stage('📦 Download Artifacts') {
            steps {
                script {
                    echo '📦 Récupération des artefacts...'

                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                    // Créer un dossier spécifique pour ce build
                    def artifactPath = "${WORKSPACE}/artifacts/${env.WORKSPACE_ID}"

                    // Télécharger les artefacts via l'API dans le dossier dédié
                    def count = api.downloadArtifacts(
                        env.WORKSPACE_ID,
                        artifactPath
                    )

                    // Archiver les artefacts dans Jenkins si des fichiers ont été téléchargés
                    if (count > 0) {
                        archiveArtifacts artifacts: "artifacts/${env.WORKSPACE_ID}/**/*",
                                        fingerprint: true,
                                        allowEmptyArchive: false

                        echo "✅ ${count} artefact(s) archivé(s) dans Jenkins sous artifacts/${env.WORKSPACE_ID}/"
                    } else {
                        echo "⚠️  Aucun artefact à archiver"
                    }
                }
            }
        }
    }

    post {
        always {
            script {
                echo '🧹 Nettoyage du workspace...'

                // Supprimer le workspace sur le builder
                sh """
                    curl -s -X DELETE http://${env.BUILDER_HOST}:${env.BUILDER_PORT}/workspace/${env.WORKSPACE_ID} || true
                """

                echo '✅ Workspace nettoyé'
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