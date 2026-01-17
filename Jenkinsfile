def builderAPI

pipeline {
    agent any

    /* // Déclencheur pour surveiller les changements dans le SCM toutes les 3 minutes
    triggers {
        pollSCM("H/3 * * * *")
    } */

    // Options globales pour le pipeline
    options {
        timeout(time: 3, unit: 'HOURS')
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

                // Générer version_history.txt pour le système de version checking
                echo '📋 Génération de version_history.txt...'
                sh 'git log --format="%h" -n 50 > version_history.txt'
                sh 'echo "Version history generated with $(wc -l < version_history.txt) entries"'
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
                                // catchError permet de continuer même si ce stage échoue
                                // Les artifacts partiels pourront être récupérés
                                catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                                    script {
                                        echo '🔨 [LINUX] Configuration CMake et vcpkg...'

                                        def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                        // Lancer le build dans le workspace Linux (plateforme linux par défaut)
                                        def jobId = api.runInWorkspace(env.WORKSPACE_ID_LINUX, 'build')

                                        echo "[LINUX] Job créé: ${jobId}"

                                        // Attendre la fin du build
                                        api.waitForJob(jobId, 10, 7200)

                                        echo "✅ [LINUX] Build terminé avec succès"
                                    }
                                }
                            }
                        }

                        stage('🔧 Compile Linux') {
                            steps {
                                catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                                    script {
                                        echo '🔧 [LINUX] Compilation du projet...'

                                        def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                        // Lancer la compilation dans le workspace Linux
                                        def jobId = api.runInWorkspace(env.WORKSPACE_ID_LINUX, 'compile')

                                        echo "[LINUX] Job créé: ${jobId}"

                                        // Attendre la fin de la compilation
                                        api.waitForJob(jobId, 10, 7200)

                                        echo "✅ [LINUX] Compilation terminée avec succès"
                                    }
                                }
                            }
                        }
                    }
                }

                stage('🪟 Windows Build (cross-compilation)') {
                    stages {
                        stage('🔨 Build Windows') {
                            steps {
                                catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                                    script {
                                        echo '🔨 [WINDOWS] Configuration CMake et vcpkg avec MinGW...'

                                        def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                        // Lancer le build Windows dans le workspace Windows avec --platform=windows
                                        def jobId = api.runInWorkspace(env.WORKSPACE_ID_WINDOWS, 'build', '--platform=windows')

                                        echo "[WINDOWS] Job créé: ${jobId}"

                                        // Attendre la fin du build
                                        api.waitForJob(jobId, 10, 7200)

                                        echo "✅ [WINDOWS] Build terminé avec succès"
                                    }
                                }
                            }
                        }

                        stage('🔧 Compile Windows') {
                            steps {
                                catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                                    script {
                                        echo '🔧 [WINDOWS] Compilation du projet avec MinGW...'

                                        def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                                        // Lancer la compilation dans le workspace Windows
                                        def jobId = api.runInWorkspace(env.WORKSPACE_ID_WINDOWS, 'compile', '--platform=windows')

                                        echo "[WINDOWS] Job créé: ${jobId}"

                                        // Attendre la fin de la compilation
                                        api.waitForJob(jobId, 10, 7200)

                                        echo "✅ [WINDOWS] Compilation terminée avec succès"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        stage('🧪 Run Linux Test') {
            // Tests uniquement sur Linux (Windows = cross-compile, non exécutable)
            steps {
                catchError(buildResult: 'SUCCESS', stageResult: 'FAILURE') {
                    script {
                        echo '🧪 Exécution des tests sur Linux...'

                        def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                        // Lancer les tests dans le workspace Linux
                        def jobId = api.runInWorkspace(env.WORKSPACE_ID_LINUX, 'test')

                        echo "[LINUX] Job créé: ${jobId}"

                        // Attendre la fin des tests
                        api.waitForJob(jobId, 10, 7200)

                        echo "✅ [LINUX] Tests exécutés avec succès"
                    }
                }
            }
        }
    }

    post {
        always {
            script {
                // Récupération des artifacts AVANT le nettoyage (même en cas d'échec)
                echo '📦 Récupération des artefacts (même en cas d\'échec)...'

                try {
                    def api = builderAPI.create(this, env.BUILDER_HOST, env.BUILDER_PORT.toInteger())

                    // Télécharger les artefacts Linux
                    try {
                        def artifactPathLinux = "${WORKSPACE}/artifacts/${env.WORKSPACE_ID_LINUX}"
                        def countLinux = api.downloadArtifacts(
                            env.BUILDER_HOST,
                            env.WORKSPACE_ID_LINUX,
                            artifactPathLinux
                        )
                        echo "✅ ${countLinux} artefact(s) Linux téléchargé(s)"
                    } catch (Exception e) {
                        echo "⚠️  Impossible de récupérer les artefacts Linux: ${e.message}"
                    }

                    // Télécharger les artefacts Windows
                    try {
                        def artifactPathWindows = "${WORKSPACE}/artifacts/${env.WORKSPACE_ID_WINDOWS}"
                        def countWindows = api.downloadArtifacts(
                            env.BUILDER_HOST,
                            env.WORKSPACE_ID_WINDOWS,
                            artifactPathWindows
                        )
                        echo "✅ ${countWindows} artefact(s) Windows téléchargé(s)"
                    } catch (Exception e) {
                        echo "⚠️  Impossible de récupérer les artefacts Windows: ${e.message}"
                    }

                    // Archiver tous les artefacts disponibles (Linux + Windows)
                    archiveArtifacts artifacts: "artifacts/**/*",
                                    fingerprint: true,
                                    allowEmptyArchive: true  // Ne pas échouer si aucun artifact

                    echo "✅ Artefacts archivés dans Jenkins"

                } catch (Exception e) {
                    echo "⚠️  Erreur lors de la récupération des artefacts: ${e.message}"
                }

                // Nettoyage des workspaces APRÈS la récupération
                echo '🧹 Nettoyage des workspaces...'

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