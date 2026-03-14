# GitHub Repository Configuration Guide

## 📋 Tab Security - Recommended Settings

### 1. Security Policy ✅ (Created)

File: `SECURITY.md`

- Supported versions table
- Vulnerability reporting process
- Response timeline
- Security features documentation

### 2. Security Advisories

**Action Required**: Enable private vulnerability reporting

```
Settings → Security → Code security and analysis
→ Private vulnerability reporting: Enable
```

**Benefits**:
- Researchers can report vulnerabilities privately
- Maintainers can fix issues before disclosure
- CVE IDs can be requested through GitHub

### 3. Dependabot Alerts

**Recommended Settings**:

```
Settings → Security → Code security and analysis

✅ Dependabot alerts: Enable
✅ Dependabot security updates: Enable
✅ Dependabot version updates: Enable
```

Create `.github/dependabot.yml`:

```yaml
version: 2
updates:
  - package-ecosystem: "gomod"
    directory: "/"
    schedule:
      interval: "weekly"
    open-pull-requests-limit: 5

  - package-ecosystem: "github-actions"
    directory: "/"
    schedule:
      interval: "weekly"
```

### 4. Code Scanning

**Recommendation**: Set up CodeQL analysis

Create `.github/workflows/codeql.yml`:

```yaml
name: "CodeQL Analysis"

on:
  push:
    branches: [ "master" ]
  pull_request:
    branches: [ "master" ]
  schedule:
    - cron: '0 0 * * 0'  # Weekly

jobs:
  analyze:
    name: Analyze
    runs-on: ubuntu-latest
    permissions:
      actions: read
      contents: read
      security-events: write

    strategy:
      fail-fast: false
      matrix:
        language: [ 'c' ]

    steps:
    - name: Checkout repository
      uses: actions/checkout@v4

    - name: Initialize CodeQL
      uses: github/codeql-action/init@v3
      with:
        languages: ${{ matrix.language }}

    - name: Autobuild
      uses: github/codeql-action/autobuild@v3

    - name: Perform CodeQL Analysis
      uses: github/codeql-action/analyze@v3
```

### 5. Secret Scanning

```
Settings → Security → Code security and analysis

✅ Secret scanning: Enable
✅ Push protection: Enable
```

---

## ⚙️ Tab Settings - Recommended Configuration

### 1. General Settings

#### Features
```
✅ Wiki: Enable (for documentation)
✅ Issues: Enable
✅ Discussions: Enable (for community Q&A)
✅ Projects: Enable
❌ Sponsorships: Optional
```

#### Pull Requests
```
✅ Allow merge commits
✅ Allow squash merging
✅ Allow rebase merging
✅ Always suggest updating pull request branches
✅ Allow auto-merge
✅ Automatically delete head branches
```

#### Suggested PR Settings:
```
Settings → General → Pull Requests

☑ Require a pull request before merging
  ☑ Require approvals: 1
  ☑ Dismiss stale pull request approvals when new commits are pushed
  ☑ Require review from Code Owners
```

### 2. Branch Protection Rules

**For `master` branch**:

```
Settings → Branches → Add branch protection rule

Branch name pattern: master

☑ Require a pull request before merging
  ☑ Require approvals: 1
  ☑ Dismiss stale pull request approvals

☑ Require status checks to pass before merging
  ☑ Require branches to be up to date before merging
  Status checks: build, test

☑ Require conversation resolution before merging

☑ Do not allow bypassing the above settings
```

Create `.github/CODEOWNERS`:

```
# CODEOWNERS - Code Review Requirements

# Default owners
* @huyhapitk12

# Compiler code requires owner review
/compiler/ @huyhapitk12

# Runtime code
/runtime/ @huyhapitk12

# Documentation
/docs/ @huyhapitk12
/README.md @huyhapitk12
```

### 3. GitHub Actions

```
Settings → Actions → General

✅ Allow all actions and reusable workflows
   OR
⚠️ Allow [organization] actions and reusable workflows

☑ Require approval for all fork pull requests
☑ Require approval for new workflows from forks
```

Create `.github/workflows/ci.yml`:

```yaml
name: CI

on:
  push:
    branches: [ master ]
  pull_request:
    branches: [ master ]

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential binutils

    - name: Build compiler
      run: make

    - name: Run tests
      run: ./build/lp test examples

    - name: Test compile example
      run: |
        ./build/lp examples/hello.lp -o test_output.c
        gcc -fopenmp test_output.c -o test_output -lm
        ./test_output
```

### 4. Pages (Optional)

For project documentation site:

```
Settings → Pages

Source: Deploy from a branch
Branch: gh-pages / (root)
```

### 5. Webhooks (Optional)

For CI/CD integration:

```
Settings → Webhooks → Add webhook

Payload URL: [Your CI/CD server]
Content type: application/json
Secret: [Generate secure secret]
Events: 
  ☑ Push
  ☑ Pull requests
```

### 6. Labels

Create issue labels:

```bash
# Via GitHub UI or API
gh label create "bug" --color "d73a4a" --description "Something isn't working"
gh label create "documentation" --color "0075ca" --description "Improvements or additions to documentation"
gh label create "enhancement" --color "a2eeef" --description "New feature or request"
gh label create "good first issue" --color "7057ff" --description "Good for newcomers"
gh label create "help wanted" --color "008672" --description "Extra attention is needed"
gh label create "security" --color "b60205" --description "Security related issue"
gh label create "parallel" --color "1d76db" --description "Parallel computing related"
gh label create "gpu" --color "5319e7" --description "GPU computing related"
```

### 7. Issue Templates

Create `.github/ISSUE_TEMPLATE/bug_report.yml`:

```yaml
name: Bug Report
description: Report a bug in LP
labels: ["bug"]
body:
  - type: markdown
    attributes:
      value: |
        Thanks for taking the time to report this bug!

  - type: textarea
    id: description
    attributes:
      label: Bug Description
      description: A clear description of the bug
    validations:
      required: true

  - type: textarea
    id: steps
    attributes:
      label: Steps to Reproduce
      description: Steps to reproduce the behavior
      placeholder: |
        1. Write code '...'
        2. Run '...'
        3. See error
    validations:
      required: true

  - type: textarea
    id: code
    attributes:
      label: Code Sample
      description: Minimal reproducible code
      render: lp
    validations:
      required: true

  - type: textarea
    id: expected
    attributes:
      label: Expected Behavior
    validations:
      required: true

  - type: textarea
    id: actual
    attributes:
      label: Actual Behavior
    validations:
      required: true

  - type: input
    id: version
    attributes:
      label: LP Version
      placeholder: "0.3.0"
    validations:
      required: true

  - type: input
    id: os
    attributes:
      label: Operating System
      placeholder: "Ubuntu 22.04"
    validations:
      required: true
```

Create `.github/ISSUE_TEMPLATE/feature_request.yml`:

```yaml
name: Feature Request
description: Suggest a new feature for LP
labels: ["enhancement"]
body:
  - type: textarea
    id: description
    attributes:
      label: Feature Description
      description: A clear description of the feature
    validations:
      required: true

  - type: textarea
    id: usecase
    attributes:
      label: Use Case
      description: Why would this feature be useful?
    validations:
      required: true

  - type: textarea
    id: example
    attributes:
      label: Proposed API
      description: How would it look in code?
      render: lp
    validations:
      required: false
```

### 8. Discussion Categories

Recommended categories:

1. **📢 Announcements** - Official announcements
2. **💬 General** - General discussion
3. **💡 Ideas** - Feature ideas and suggestions
4. **❓ Q&A** - Questions and answers
5. **🎉 Show and Tell** - Showcase projects using LP

---

## 📊 Summary Checklist

### Security Tab
| Setting | Status | Priority |
|---------|--------|----------|
| SECURITY.md | ✅ Created | High |
| Private vulnerability reporting | ⬜ Enable | High |
| Dependabot alerts | ⬜ Enable | High |
| Code scanning (CodeQL) | ⬜ Enable | Medium |
| Secret scanning | ⬜ Enable | High |
| Push protection | ⬜ Enable | High |

### Settings Tab
| Setting | Status | Priority |
|---------|--------|----------|
| Wiki enabled | ⬜ Enable | Medium |
| Discussions enabled | ⬜ Enable | Medium |
| Branch protection (master) | ⬜ Configure | High |
| CODEOWNERS file | ⬜ Create | Medium |
| CI workflow | ⬜ Create | High |
| Issue templates | ⬜ Create | Medium |
| Labels | ⬜ Create | Low |

---

## 🚀 Quick Setup Commands

```bash
# Create .github directory structure
mkdir -p .github/ISSUE_TEMPLATE
mkdir -p .github/workflows

# Create CODEOWNERS
echo "* @huyhapitk12" > .github/CODEOWNERS

# Create dependabot config
cat > .github/dependabot.yml << 'EOF'
version: 2
updates:
  - package-ecosystem: "github-actions"
    directory: "/"
    schedule:
      interval: "weekly"
EOF

# Create labels (requires gh CLI)
gh label create "security" --color "b60205"
gh label create "parallel" --color "1d76db"
gh label create "gpu" --color "5319e7"
```

---

**Document created for LP Language repository configuration**
