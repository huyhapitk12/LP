# Security Policy

## Supported Versions

We actively support the following versions of LP with security updates:

| Version | Supported          |
| ------- | ------------------ |
| 0.1.x   | ✅ Active support  |
| < 0.1   | ❌ No longer supported |

## Reporting a Vulnerability

We take security vulnerabilities seriously. If you discover a security vulnerability in LP, please report it responsibly.

### How to Report

**Preferred Method**: Use GitHub's private vulnerability reporting feature.

1. Go to the [Security Advisory](https://github.com/huyhapitk12/LP/security/advisories) page
2. Click "Report a vulnerability"
3. Fill in the details

**Alternative**: Email the maintainer directly if the vulnerability is sensitive.

### What to Include

Please include the following information:

- **Description** of the vulnerability
- **Steps to reproduce** the issue
- **Affected versions**
- **Potential impact**
- **Suggested fix** (if any)
- **Your contact information** for follow-up

### Response Timeline

| Stage | Expected Time |
|-------|---------------|
| Initial response | Within 48 hours |
| Triage & confirmation | Within 7 days |
| Fix development | Depends on severity |
| Advisory publication | After fix is released |

### Severity Levels

| Level | Description | Response Time |
|-------|-------------|---------------|
| 🔴 Critical | Remote code execution, data leak | 24-48 hours |
| 🟠 High | Privilege escalation, significant bypass | 7 days |
| 🟡 Medium | Limited impact vulnerabilities | 14 days |
| 🟢 Low | Minor issues, best practice improvements | 30 days |

## Security Features in LP

LP provides built-in security features through the `@security` decorator:

```lp
# Rate limiting
@security(rate_limit=100)
def api_endpoint():
    pass

# Authentication required
@security(auth=True, level=3)
def protected_function():
    pass

# Input validation & sanitization
@security(validate=True, sanitize=True, prevent_injection=True)
def process_input(data: str) -> str:
    return data

# Access control
@security(admin=True)
def admin_function():
    pass

# Read-only mode
@security(readonly=True)
def read_data():
    pass
```

## Best Practices

When using LP, follow these security best practices:

### 1. Input Validation

```lp
@security(validate=True)
def process_user_input(data: dict) -> dict:
    # Input is automatically validated
    return data
```

### 2. SQL Injection Prevention

```lp
@security(prevent_injection=True)
def query_database(user_input: str) -> list:
    # SQL injection is automatically detected and blocked
    return sqlite.query(db, f"SELECT * FROM users WHERE name = '{user_input}'")
```

### 3. XSS Prevention

```lp
@security(prevent_xss=True, sanitize=True)
def render_html(content: str) -> str:
    # HTML is automatically sanitized
    return content
```

### 4. Rate Limiting

```lp
@security(rate_limit=60)  # 60 requests per minute
def public_api():
    pass
```

## Security Changelog

| Date | Issue | Fix |
|------|-------|-----|
| 2025-03 | Initial security policy | Added @security decorator support |

## Contact

For security concerns, contact:
- **GitHub Security Advisories**: [Report a vulnerability](https://github.com/huyhapitk12/LP/security/advisories)
- **Project Maintainer**: [@huyhapitk12](https://github.com/huyhapitk12)

## Acknowledgments

We thank all security researchers who responsibly disclose vulnerabilities. Contributors who report valid security issues will be acknowledged in our security advisories (with their permission).
