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
        WORKSPACE_ID_LINUX = "build_${BUILD_NUMBER}_linux"
        WORKSPACE_ID_WINDOWS = "build_${BUILD_NUMBER}_windows"
    }

    stages {
        stage('Checkout') {
            steps {
                echo '📥 Récupération du code source...'
                checkout scm
            }
        }

        stage('🔌 Initialize Builder API') {
            steps {
                script {
                    builderAPI = load('ci_cd/jenkins/BuilderAPI.groovy')
                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                    // Vérifier que le builder est accessible
                    if (!api.healthCheck()) {
                        error("❌ Le builder permanent n'est pas accessible. Lancez d'abord le job d'initialisation (Jenkinsfile.init).")
                    }
                }
            }
        }

        stage('📋 Create Workspaces') {
            parallel {
                stage('Create Linux Workspace') {
                    steps {
                        script {
                            echo "📋 Création du workspace Linux ${env.WORKSPACE_ID_LINUX}..."

                            def createResponse = sh(
                                script: """
                                    curl -s -f -X POST http://${env.BUILDER_HOST}:${env.BUILDER_PORT}/workspace/create \
                                        -H 'Content-Type: application/json' \
                                        -d '{"build_number": "${env.BUILD_NUMBER}_linux"}'
                                """,
                                returnStdout: true
                            ).trim()

                            echo "✅ Workspace Linux créé: ${createResponse}"
                        }
                    }
                }
                stage('Create Windows Workspace') {
                    steps {
                        script {
                            echo "📋 Création du workspace Windows ${env.WORKSPACE_ID_WINDOWS}..."

                            def createResponse = sh(
                                script: """
                                    curl -s -f -X POST http://${env.BUILDER_HOST}:${env.BUILDER_PORT}/workspace/create \
                                        -H 'Content-Type: application/json' \
                                        -d '{"build_number": "${env.BUILD_NUMBER}_windows"}'
                                """,
                                returnStdout: true
                            ).trim()

                            echo "✅ Workspace Windows créé: ${createResponse}"
                        }
                    }
                }
            }
        }

        stage('📤 Upload Source Code') {
            parallel {
                stage('Upload to Linux Workspace') {
                    steps {
                        script {
                            echo "📤 Upload du code source vers workspace Linux via rsync..."

                            sh """
                                rsync -avz --delete \
                                    --exclude='.git' \
                                    --exclude='build/*' \
                                    --exclude='buildLinux/*' \
                                    --exclude='buildWin/*' \
                                    --exclude='buildMac/*' \
                                    --exclude='cmake-build-*' \
                                    --exclude='*.o' \
                                    --exclude='*.a' \
                                    --exclude='.gitignore' \
                                    --exclude='third_party/vcpkg/.git' \
                                    --exclude='artifacts' \
                                    ${WORKSPACE}/ \
                                    rsync://${env.BUILDER_HOST}:873/workspace/${env.WORKSPACE_ID_LINUX}/
                            """

                            echo "✅ Code source uploadé vers workspace Linux"
                        }
                    }
                }
                stage('Upload to Windows Workspace') {
                    steps {
                        script {
                            echo "📤 Upload du code source vers workspace Windows via rsync..."

                            sh """
                                rsync -avz --delete \
                                    --exclude='.git' \
                                    --exclude='build/*' \
                                    --exclude='buildLinux/*' \
                                    --exclude='buildWin/*' \
                                    --exclude='buildMac/*' \
                                    --exclude='cmake-build-*' \
                                    --exclude='*.o' \
                                    --exclude='*.a' \
                                    --exclude='.gitignore' \
                                    --exclude='third_party/vcpkg/.git' \
                                    --exclude='artifacts' \
                                    ${WORKSPACE}/ \
                                    rsync://${env.BUILDER_HOST}:873/workspace/${env.WORKSPACE_ID_WINDOWS}/
                            """

                            echo "✅ Code source uploadé vers workspace Windows"
                        }
                    }
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

                                    // Lancer le build dans le workspace Linux (plateforme linux par défaut)
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID_LINUX, 'build')

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

                                    // Lancer la compilation dans le workspace Linux
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID_LINUX, 'compile')

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

                                    // Lancer le build Windows dans le workspace Windows avec --platform=windows
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID_WINDOWS, 'build', '--platform=windows')

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

                                    // Lancer la compilation dans le workspace Windows
                                    def jobId = api.runInWorkspace(env.WORKSPACE_ID_WINDOWS, 'compile')

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
            parallel {
                stage('Download Linux Artifacts') {
                    steps {
                        script {
                            echo '📦 Récupération des artefacts Linux...'

                            def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                            // Créer un dossier spécifique pour les artefacts Linux
                            def artifactPath = "${WORKSPACE}/artifacts/${env.WORKSPACE_ID_LINUX}"

                            // Télécharger les artefacts Linux via l'API
                            def count = api.downloadArtifacts(
                                env.WORKSPACE_ID_LINUX,
                                artifactPath
                            )

                            echo "✅ ${count} artefact(s) Linux téléchargé(s)"
                        }
                    }
                }
                stage('Download Windows Artifacts') {
                    steps {
                        script {
                            echo '📦 Récupération des artefacts Windows...'

                            def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                            // Créer un dossier spécifique pour les artefacts Windows
                            def artifactPath = "${WORKSPACE}/artifacts/${env.WORKSPACE_ID_WINDOWS}"

                            // Télécharger les artefacts Windows via l'API
                            def count = api.downloadArtifacts(
                                env.WORKSPACE_ID_WINDOWS,
                                artifactPath
                            )

                            echo "✅ ${count} artefact(s) Windows téléchargé(s)"
                        }
                    }
                }
            }
        }

        stage('📦 Archive Artifacts') {
            steps {
                script {
                    echo '📦 Archivage des artefacts dans Jenkins...'

                    // Archiver tous les artefacts (Linux + Windows)
                    archiveArtifacts artifacts: "artifacts/**/*",
                                    fingerprint: true,
                                    allowEmptyArchive: false

                    echo "✅ Artefacts archivés dans Jenkins"
                }
            }
        }
    }

    post {
        always {
            script {
                echo '🧹 Nettoyage des workspaces...'

                // Supprimer les deux workspaces sur le builder
                sh """
                    curl -s -X DELETE http://${env.BUILDER_HOST}:${env.BUILDER_PORT}/workspace/${env.WORKSPACE_ID_LINUX} || true
                    curl -s -X DELETE http://${env.BUILDER_HOST}:${env.BUILDER_PORT}/workspace/${env.WORKSPACE_ID_WINDOWS} || true
                """

                echo '✅ Workspaces nettoyés'
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