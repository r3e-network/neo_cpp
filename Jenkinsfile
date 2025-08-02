pipeline {
    agent {
        label 'docker'
    }
    
    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timeout(time: 2, unit: 'HOURS')
        timestamps()
        parallelsAlwaysFailFast()
    }
    
    environment {
        DOCKER_REGISTRY = credentials('docker-registry')
        SONAR_TOKEN = credentials('sonar-token')
        CMAKE_BUILD_TYPE = 'Release'
    }
    
    stages {
        stage('Checkout') {
            steps {
                checkout scm
                sh 'git submodule update --init --recursive'
            }
        }
        
        stage('Build Matrix') {
            parallel {
                stage('Linux GCC') {
                    agent {
                        docker {
                            image 'gcc:11'
                            args '-v /tmp/ccache:/tmp/ccache'
                        }
                    }
                    steps {
                        sh '''
                            apt-get update && apt-get install -y cmake ninja-build libboost-all-dev libssl-dev
                            mkdir -p build-gcc
                            cd build-gcc
                            cmake .. -G Ninja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DNEO_BUILD_TESTS=ON
                            ninja
                        '''
                    }
                    post {
                        always {
                            archiveArtifacts artifacts: 'build-gcc/**/*.so', fingerprint: true
                        }
                    }
                }
                
                stage('Linux Clang') {
                    agent {
                        docker {
                            image 'silkeh/clang:13'
                            args '-v /tmp/ccache:/tmp/ccache'
                        }
                    }
                    steps {
                        sh '''
                            apt-get update && apt-get install -y cmake ninja-build libboost-all-dev libssl-dev
                            mkdir -p build-clang
                            cd build-clang
                            cmake .. -G Ninja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DNEO_BUILD_TESTS=ON
                            ninja
                        '''
                    }
                }
                
                stage('Windows') {
                    agent {
                        label 'windows'
                    }
                    steps {
                        bat '''
                            mkdir build-windows
                            cd build-windows
                            cmake .. -G "Visual Studio 17 2022" -A x64
                            cmake --build . --config %CMAKE_BUILD_TYPE%
                        '''
                    }
                }
            }
        }
        
        stage('Test') {
            parallel {
                stage('Unit Tests') {
                    agent {
                        docker {
                            image 'gcc:11'
                            args '-v /tmp/ccache:/tmp/ccache'
                        }
                    }
                    steps {
                        sh '''
                            cd build-gcc
                            ctest --output-on-failure --no-compress-output -T Test
                        '''
                    }
                    post {
                        always {
                            junit 'build-gcc/Testing/**/*.xml'
                        }
                    }
                }
                
                stage('Integration Tests') {
                    agent {
                        docker {
                            image 'gcc:11'
                            args '-v /tmp/ccache:/tmp/ccache'
                        }
                    }
                    steps {
                        sh '''
                            cd build-gcc
                            ./tests/integration/run_integration_tests.sh
                        '''
                    }
                }
                
                stage('Coverage') {
                    agent {
                        docker {
                            image 'gcc:11'
                            args '-v /tmp/ccache:/tmp/ccache'
                        }
                    }
                    steps {
                        sh '''
                            apt-get update && apt-get install -y lcov
                            mkdir -p build-coverage
                            cd build-coverage
                            cmake .. -DCMAKE_BUILD_TYPE=Debug -DNEO_ENABLE_COVERAGE=ON
                            make
                            ctest
                            lcov --capture --directory . --output-file coverage.info
                            lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage.info
                        '''
                        publishHTML([
                            allowMissing: false,
                            alwaysLinkToLastBuild: true,
                            keepAll: true,
                            reportDir: 'build-coverage',
                            reportFiles: 'coverage/index.html',
                            reportName: 'Coverage Report'
                        ])
                    }
                }
            }
        }
        
        stage('Static Analysis') {
            parallel {
                stage('Clang-Tidy') {
                    agent {
                        docker {
                            image 'ubuntu:22.04'
                        }
                    }
                    steps {
                        sh '''
                            apt-get update && apt-get install -y clang-tidy
                            find src include -name '*.cpp' -o -name '*.h' | xargs clang-tidy -p build-gcc
                        '''
                    }
                }
                
                stage('Cppcheck') {
                    agent {
                        docker {
                            image 'ubuntu:22.04'
                        }
                    }
                    steps {
                        sh '''
                            apt-get update && apt-get install -y cppcheck
                            cppcheck --enable=all --xml --xml-version=2 src include 2> cppcheck-results.xml
                        '''
                        recordIssues(
                            tools: [cppCheck(pattern: 'cppcheck-results.xml')]
                        )
                    }
                }
                
                stage('SonarQube') {
                    agent {
                        docker {
                            image: 'sonarsource/sonar-scanner-cli'
                        }
                    }
                    steps {
                        withSonarQubeEnv('SonarQube') {
                            sh '''
                                sonar-scanner \
                                  -Dsonar.projectKey=neo-cpp \
                                  -Dsonar.sources=src,include \
                                  -Dsonar.tests=tests \
                                  -Dsonar.coverageReportPaths=build-coverage/coverage.info
                            '''
                        }
                    }
                }
            }
        }
        
        stage('Security Scan') {
            parallel {
                stage('Dependency Check') {
                    steps {
                        dependencyCheck additionalArguments: '''
                            --enableRetired
                            --enableExperimental
                            --format HTML
                            --format JSON
                        ''', odcInstallation: 'dependency-check'
                        
                        dependencyCheckPublisher pattern: 'dependency-check-report.json'
                    }
                }
                
                stage('Container Scan') {
                    when {
                        branch pattern: "(main|develop|release/.*)", comparator: "REGEXP"
                    }
                    steps {
                        sh '''
                            docker run --rm -v /var/run/docker.sock:/var/run/docker.sock \
                                aquasec/trivy image --format json --output trivy-report.json \
                                ${DOCKER_REGISTRY}/neo-cpp:${GIT_COMMIT}
                        '''
                    }
                }
            }
        }
        
        stage('Build Artifacts') {
            when {
                anyOf {
                    branch 'main'
                    branch 'develop'
                    tag pattern: "v\\d+\\.\\d+\\.\\d+", comparator: "REGEXP"
                }
            }
            parallel {
                stage('Docker Image') {
                    steps {
                        script {
                            docker.build("${DOCKER_REGISTRY}/neo-cpp:${GIT_COMMIT}")
                            if (env.TAG_NAME) {
                                docker.tag("${DOCKER_REGISTRY}/neo-cpp:${GIT_COMMIT}", "${DOCKER_REGISTRY}/neo-cpp:${TAG_NAME}")
                                docker.tag("${DOCKER_REGISTRY}/neo-cpp:${GIT_COMMIT}", "${DOCKER_REGISTRY}/neo-cpp:latest")
                            }
                        }
                    }
                }
                
                stage('Binary Packages') {
                    steps {
                        sh '''
                            cd build-gcc
                            cpack -G DEB
                            cpack -G RPM
                            cpack -G TGZ
                        '''
                        archiveArtifacts artifacts: 'build-gcc/*.deb,build-gcc/*.rpm,build-gcc/*.tar.gz', fingerprint: true
                    }
                }
            }
        }
        
        stage('Deploy') {
            when {
                tag pattern: "v\\d+\\.\\d+\\.\\d+", comparator: "REGEXP"
            }
            stages {
                stage('Staging') {
                    steps {
                        sh '''
                            kubectl --context=staging set image deployment/neo-node \
                                neo-node=${DOCKER_REGISTRY}/neo-cpp:${TAG_NAME} -n neo-staging
                            kubectl --context=staging rollout status deployment/neo-node -n neo-staging
                        '''
                    }
                }
                
                stage('Production Approval') {
                    steps {
                        timeout(time: 24, unit: 'HOURS') {
                            input message: 'Deploy to production?', ok: 'Deploy'
                        }
                    }
                }
                
                stage('Production') {
                    steps {
                        sh '''
                            kubectl --context=production set image deployment/neo-node \
                                neo-node=${DOCKER_REGISTRY}/neo-cpp:${TAG_NAME} -n neo-production
                            kubectl --context=production rollout status deployment/neo-node -n neo-production
                        '''
                    }
                }
            }
        }
    }
    
    post {
        always {
            cleanWs()
        }
        success {
            slackSend(
                color: 'good',
                message: "Build Successful: ${env.JOB_NAME} - ${env.BUILD_NUMBER} (<${env.BUILD_URL}|Open>)"
            )
        }
        failure {
            slackSend(
                color: 'danger',
                message: "Build Failed: ${env.JOB_NAME} - ${env.BUILD_NUMBER} (<${env.BUILD_URL}|Open>)"
            )
        }
    }
}