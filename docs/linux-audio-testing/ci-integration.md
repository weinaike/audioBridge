# CI/CD Integration Guide for audioBridge Linux Testing

**Purpose**: Integrate audioBridge Linux virtual audio testing into CI/CD pipelines
**Supported Platforms**: GitHub Actions, GitLab CI, Jenkins
**Test Types**: Automated loopback testing, validation, reporting

---

## Overview

audioBridge Linux virtual audio testing can be integrated into any CI/CD platform that supports:
- Linux environments (Ubuntu, Fedora, Arch)
- Kernel module loading (snd-aloop)
- Audio device access
- JUnit XML report parsing

---

## Prerequisites for CI/CD

### 1. Runner Requirements

**System Requirements**:
- Linux OS (Ubuntu 20.04+, Fedora 38+, or Arch)
- sudo access (for kernel module loading)
- Audio device access

**Software Requirements**:
```bash
# Ubuntu/Debian
sudo apt install alsa-utils sox libsndfile1 build-essential cmake git

# Fedora
sudo dnf install alsa-utils sox libsndfile gcc-c++ cmake git

# Arch
sudo pacman -S alsa-utils sox libsndfile gcc cmake make base-devel
```

### 2. Permissions

The CI runner must have:
- **sudo access**: To load snd-aloop kernel module
- **Audio group access**: To access audio devices
- **Build permissions**: To compile the project

---

## GitHub Actions Integration

### Basic Setup

1. **Copy workflow file**:
```bash
cp examples/ci/github-actions.yml .github/workflows/linux-tests.yml
```

2. **Commit and push**:
```bash
git add .github/workflows/linux-tests.yml
git commit -m "Add GitHub Actions workflow for Linux audio testing"
git push
```

3. **View results**:
   - Go to Actions tab in your GitHub repository
   - Select "AudioBridge Linux Tests" workflow
   - View test results and artifacts

### Advanced Configuration

#### Manual Trigger with Parameters

```yaml
on:
  workflow_dispatch:
    inputs:
      test_suite:
        description: 'Test suite to run'
        required: false
        default: 'default'
        type: choice
        options:
          - default
          - quick
          - comprehensive
```

#### Scheduled Runs

```yaml
on:
  schedule:
    - cron: '0 2 * * *'  # Run daily at 2 AM UTC
    - cron: '0 6 * * 1'  # Run weekly on Monday at 6 AM UTC
```

#### Matrix Builds

```yaml
strategy:
  matrix:
    os: [ubuntu-20.04, ubuntu-22.04]
    suite: [default, comprehensive]
```

### Example Workflows

**Quick Test** (5 minutes):
```yaml
name: Quick Audio Tests
on: [push, pull_request]
jobs:
  quick-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: sudo apt install -y alsa-utils sox libsndfile1 cmake
      - name: Load module
        run: sudo modprobe snd-aloop
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make audioBridge-test
      - name: Quick test
        run: cd build && ./tests/tools/audioBridge-test run ../test-data/audio/1khz-sine.wav
```

**Comprehensive Test Suite** (15 minutes):
```yaml
name: Comprehensive Audio Tests
on:
  schedule:
    - cron: '0 2 * * *'
jobs:
  comprehensive:
    runs-on: ubuntu-latest
    steps:
      # ... (see examples/ci/github-actions.yml)
```

---

## GitLab CI Integration

### Basic Setup

1. **Copy CI configuration**:
```bash
cp examples/ci/gitlab-ci.yml .gitlab-ci.yml
```

2. **Commit and push**:
```bash
git add .gitlab-ci.yml
git commit -m "Add GitLab CI pipeline for Linux audio testing"
git push
```

3. **View results**:
   - Go to CI/CD > Pipelines in your GitLab project
   - View pipeline stages and results

### Pipeline Stages

The GitLab CI pipeline includes:
```
build → test:setup → test:devices → test:audio → test:suite → report:summary
```

### Advanced Configuration

#### Manual Pipeline Trigger

```yaml
test:manual:
  stage: test
  when: manual
  variables:
    CUSTOM_SUITE: "default"
  script:
    - cd build
    - ./tests/tools/audioBridge-test run-suite ${CUSTOM_SUITE}
```

#### Dynamic Test Suite Selection

```yaml
variables:
  TEST_SUITE: "$CI_COMMIT_REF_SLUG"

test:suite:
  stage: test
  script:
    - cd build
    - ./tests/tools/audioBridge-test run-suite ${TEST_SUITE}
```

### Example Pipelines

**Merge Request Pipeline**:
```yaml
test:mr:
  stage: test
  only:
    - merge_requests
  script:
    - cd build
    - ./tests/tools/audioBridge-test run-suite default
  allow_failure: false
```

**Scheduled Pipeline**:
```yaml
test:scheduled:
  stage: test
  only:
    - schedules
  script:
    - cd build
    - ./tests/tools/audioBridge-test run-suite comprehensive
```

---

## Jenkins Integration

### Basic Setup

1. **Create Jenkinsfile**:
```bash
cp examples/ci/Jenkinsfile Jenkinsfile
```

2. **Create Pipeline job**:
   - New Item > Pipeline
   - Definition: Pipeline script from SCM
   - SCM: Git
   - Script Path: Jenkinsfile

3. **Build with parameters**:
   - Check "This project is parameterized"
   - Add parameters:
     - TEST_SUITE (String, default: "default")
     - CONTINUE_ON_ERROR (Boolean, default: false)
     - LOG_LEVEL (Choice, default: "info")

### Pipeline Stages

```
Setup → Build → Generate Test Audio → Setup Check → List Devices →
Run Single Test → Run Test Suite → Publish Results
```

### Advanced Configuration

#### Multi-Branch Pipeline

```groovy
properties([
    buildDiscarder(logRotator(numToKeepStr: '10')),
    pipelineTriggers([
        branch('*/main'),
        branch('*/master'),
        branch('*/develop')
    ])
])
```

#### Tool Installation

```groovy
stage('Setup') {
    steps {
        sh 'sudo apt-get update'
        sh 'sudo apt-get install -y alsa-utils sox libsndfile1 cmake'
    }
}
```

### Example Pipelines

**Simple Pipeline** (Declarative):
```groovy
pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                sh 'mkdir -p build && cd build && cmake .. && make'
            }
        }
        stage('Test') {
            steps {
                sh 'cd build && ./tests/tools/audioBridge-test run-suite default'
            }
        }
    }
}
}
```

---

## Test Report Formats

### JUnit XML Format

All three CI platforms support JUnit XML format:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="audioBridge-test" tests="4" failures="1">
    <testcase name="1kHz Sine Wave Test" classname="audioBridge.loopback">
      <system-out>Frequency: 1000 Hz
SNR: 72 dB
Latency: 12 ms</system-out>
    </testcase>
    <testcase name="White Noise Test" classname="audioBridge.loopback">
      <failure message="SNR below threshold">
Expected: SNR > 40 dB
Actual: SNR = 35 dB
      </failure>
    </testcase>
  </testsuite>
</testsuites>
```

### Generating JUnit Reports

The run-suite command automatically generates JUnit XML reports:

```bash
./audioBridge-test run-suite default --report-format junit --report-file test-results.xml
```

---

## Troubleshooting CI/CD Issues

### Issue 1: Permission Denied

**Error**: `modprobe: ERROR: could not insert 'snd_aloop'`

**Solution**:
- Ensure runner has sudo access
- Check if runner is unprivileged container
- Add `sudo` to modprobe command

### Issue 2: No Audio Devices

**Error**: `No loopback devices found`

**Solution**:
```bash
# Verify module loaded
lsmod | grep snd_aloop

# List devices
aplay -l | grep Loopback

# If missing, load module
sudo modprobe snd-aloop
```

### Issue 3: Test Audio Missing

**Error**: `Test audio file not found`

**Solution**:
```yaml
- name: Generate test audio
  run: |
    chmod +x scripts/utils/generate-test-audio.sh
    ./scripts/utils/generate-test-audio.sh
```

### Issue 4: Compilation Errors

**Error**: `CMake configuration failed`

**Solution**:
```yaml
- name: Install build dependencies
  run: |
    sudo apt-get update
    sudo apt-get install -y build-essential cmake
```

---

## Best Practices

### 1. Stage Optimization

Group tests by type:
- Quick tests: Run on every commit
- Full tests: Run nightly or on release
- Specific tests: Run on file changes

### 2. Caching

Cache dependencies between runs:

**GitHub Actions**:
```yaml
- name: Cache build
  uses: actions/cache@v3
  with:
    path: build/
    key: ${{ runner.os }}-build-${{ hashFiles('**/*.cpp') }}
```

**GitLab CI**:
```yaml
cache:
  key: "${CI_COMMIT_REF_SLUG}"
  paths:
    - build/
```

**Jenkins**:
```groovy
options {
    buildDiscarder(logRotator(numToKeepStr: '10'))
}
```

### 3. Parallel Execution

Run tests in parallel (requires multiple devices):

```yaml
strategy:
  matrix:
    test: [1khz-sine, 440hz-tone, white-noise]
```

### 4. Artifact Retention

Keep test results for debugging:

```yaml
- name: Upload artifacts
  uses: actions/upload-artifact@v4
  with:
    name: test-results
    path: build/test-results/
    retention-days: 30
```

### 5. Notifications

Send notifications on test failures:

**GitHub Actions**:
```yaml
- name: Notify on failure
  if: failure()
  uses: actions/github-script@v7
  with:
    script: |
      github.rest.issues.createComment({...})
```

**GitLab CI**:
```yaml
notify:
  email:
    recipients:
      - team@example.com
    on_success: false
    on_failure: true
```

**Jenkins**:
```groovy
post {
    failure {
        emailext (
            subject: "Build Failed",
            body: "Check the console output for details."
        )
    }
}
```

---

## Security Considerations

### 1. sudo Access

CI runners require sudo to load kernel modules:
- Ensure runner is trusted
- Use self-hosted runners if security concern
- Limit sudo to specific commands (modprobe only)

### 2. Device Access

Audio device access requires:
- Runner in audio group
- No other processes using devices
- Proper cleanup between tests

### 3. Secrets Management

For private dependencies:
```yaml
- name: Install private dependencies
  env:
    TOKEN: ${{ secrets.AUTH_TOKEN }}
  run: |
    curl -H "Authorization: Bearer $TOKEN" ...
```

---

## Performance Optimization

### 1. Build Caching

Cache build artifacts between runs:
- GitHub Actions: `actions/cache@v3`
- GitLab CI: `cache:` directive
- Jenkins: Workspace caching

### 2. Test Parallelization

Run tests in parallel where possible:
- Use multiple runners
- Split test suite across jobs
- Aggregate results at end

### 3. Fast-Fail Strategy

Fail fast on errors:
```yaml
continue-on-error: false
```

### 4. Resource Limits

Set appropriate timeouts:
```yaml
timeout-minutes: 30
```

---

## Monitoring and Debugging

### View Test Logs

**GitHub Actions**:
- Actions > Select workflow run > View logs

**GitLab CI**:
- CI/CD > Pipelines > Select pipeline > View logs

**Jenkins**:
- Build > Console Output

### Download Artifacts

**GitHub Actions**:
- Summary page > Artifacts section

**GitLab CI**:
- Job > Artifacts > Download

**Jenkins**:
- Build > Build Artifacts

### Common Issues

1. **Module not loaded**: Check sudo access
2. **No devices found**: Check aplay -l output
3. **Tests timeout**: Increase timeout value
4. **Compilation errors**: Check dependencies installed

---

## Quick Reference

### Platform-Specific Commands

| Platform | Config Location | Trigger | Logs |
|----------|----------------|---------|------|
| GitHub Actions | `.github/workflows/*.yml` | Push, PR, Manual | Actions tab |
| GitLab CI | `.gitlab-ci.yml` | Push, MR, Manual | CI/CD > Pipelines |
| Jenkins | `Jenkinsfile` | SCM trigger, Manual | Build > Console Output |

### Common Commands

```bash
# Build
mkdir -p build && cd build
cmake ..
make audioBridge-test

# Setup check
./tests/tools/audioBridge-test setup-check

# List devices
./tests/tools/audioBridge-test list-devices

# Run single test
./tests/tools/audioBridge-test run test.wav

# Run test suite
./tests/tools/audioBridge-test run-suite default

# Run with options
./tests/tools/audioBridge-test run-suite default --continue-on-error
```

---

## Additional Resources

- **GitHub Actions Docs**: https://docs.github.com/en/actions
- **GitLab CI Docs**: https://docs.gitlab.com/ee/ci/
- **Jenkins Docs**: https://www.jenkins.io/doc/
- **audioBridge Docs**: `docs/linux-audio-testing/`

---

**Last Updated**: 2025-12-24
**Maintained By**: audioBridge Development Team
