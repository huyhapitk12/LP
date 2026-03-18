# LP Security Module - Complete Reference

## Table of Contents
- [Overview](#overview)
- [@security Decorator](#security-decorator)
- [Security Module Functions](#security-module-functions)
- [Examples](#examples)

---

## Overview

LP Security provides 2 ways to protect your application:

1. **@security decorator** - Automatically injects security checks at function start
2. **security module** - Call security functions directly from code

---

## @security Decorator

### Syntax

```lp
@security(options...)
def function_name(params):
    # function body
```

### Options

| Option | Type | Description | Example |
|--------|------|-------------|---------|
| `level` | int/str | Security level (0-4 or "none"/"low"/"medium"/"high"/"critical") | `level="high"` |
| `auth` | str | Authentication type ("basic", "bearer", "api_key", "oauth") | `auth="bearer"` |
| `rate_limit` | int | Max requests per minute | `rate_limit=100` |
| `validate` | bool | Enable input validation | `validate=True` |
| `sanitize` | bool | Enable output sanitization | `sanitize=True` |
| `injection` | bool | Prevent SQL injection | `injection=True` |
| `xss` | bool | Prevent XSS attacks | `xss=True` |
| `csrf` | bool | Enable CSRF protection | `csrf=True` |
| `cors` | str/bool | Enable CORS with origins | `cors="https://example.com"` |
| `headers` | bool | Enable security headers | `headers=True` |
| `encrypt` | bool | Enable data encryption | `encrypt=True` |
| `hash` | str | Hash algorithm ("md5", "sha256") | `hash="sha256"` |
| `readonly` | flag | Enable readonly mode | `readonly` |
| `admin` | flag | Require admin access | `admin` |
| `user` | flag | Require user access | `user` |
| `guest` | flag | Allow guest access | `guest` |

### Examples

```lp
# Basic security
@security
def api_function():
    return "OK"

# Rate limiting
@security(rate_limit=10)
def limited_api():
    return "Limited to 10 req/min"

# Authentication required
@security(auth="bearer")
def protected_api():
    return "Protected data"

# Admin only
@security(admin)
def admin_delete(id: int):
    return f"Deleted {id}"

# Combined options
@security(
    level="critical",
    auth="api_key",
    rate_limit=5,
    injection=True,
    xss=True
)
def secure_api(data: str):
    return f"Processed: {data}"
```

---

## Security Module Functions

Import and use:

```lp
import security
```

### Authentication Context

| Function | Description | Returns |
|----------|-------------|---------|
| `security.set_authenticated(level, user_id)` | Set user as authenticated | void |
| `security.set_guest()` | Logout (set as guest) | void |
| `security.is_authenticated()` | Check if authenticated | bool |
| `security.get_access_level()` | Get current access level | int |

```lp
# Login as user
security.set_authenticated(1, "john_doe")

# Login as admin  
security.set_authenticated(2, "admin_user")

# Logout
security.set_guest()

# Check status
if security.is_authenticated():
    level = security.get_access_level()
```

### Validation Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `security.validate_email(email)` | Validate email format | bool |
| `security.validate_url(url)` | Validate URL format | bool |
| `security.validate_numeric(str)` | Validate numeric string | bool |
| `security.validate_alphanumeric(str)` | Validate alphanumeric | bool |
| `security.validate_identifier(id)` | Validate identifier name | bool |
| `security.is_safe_string(str)` | Check for safe characters | bool |

```lp
if security.validate_email("user@example.com"):
    print("Valid email")

if security.validate_url("https://google.com"):
    print("Valid URL")

if security.validate_identifier("my_function"):
    print("Valid identifier")
```

### Detection Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `security.detect_sql_injection(str)` | Detect SQL injection patterns | bool |
| `security.detect_xss(str)` | Detect XSS patterns | bool |

```lp
dangerous = "'; DROP TABLE users; --"
if security.detect_sql_injection(dangerous):
    print("SQL injection detected!")

xss = "<script>alert('xss')</script>"
if security.detect_xss(xss):
    print("XSS detected!")
```

### Escape Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `security.sql_escape(str)` | Escape for SQL | str |
| `security.html_escape(str)` | Escape HTML entities | str |
| `security.strip_html(str)` | Remove HTML tags | str |

```lp
# SQL escape
safe = security.sql_escape("O'Brien")
# Result: "O''Brien"

# HTML escape
safe = security.html_escape("<b>Hello</b>")
# Result: "&lt;b&gt;Hello&lt;/b&gt;"

# Strip HTML
clean = security.strip_html("<p>Hello</p>")
# Result: "Hello"
```

### Hash Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `security.hash_md5(str)` | MD5 hash | str (32 hex chars) |
| `security.hash_sha256(str)` | SHA256 hash | str (64 hex chars) |

```lp
# Hash password
hashed = security.hash_sha256("password123")
print(f"Hashed: {hashed}")
```

### Rate Limiting

| Function | Description | Returns |
|----------|-------------|---------|
| `security.check_rate_limit(name, limit)` | Check rate limit | bool |
| `security.rate_limit_remaining(name)` | Get remaining requests | int |

```lp
# Check rate limit manually
if security.check_rate_limit("my_function", 10):
    # Still within limit
    do_work()
else:
    print("Rate limit exceeded")

# Get remaining
remaining = security.rate_limit_remaining("my_function")
print(f"Remaining: {remaining}")
```

---

## Complete Example

```lp
import security

# === Authentication Context ===

# Login as admin
security.set_authenticated(2, "admin_user")

# === Protected Functions ===

@security(rate_limit=5, auth="bearer")
def transfer_money(to: str, amount: int) -> str:
    return f"Transferred {amount} to {to}"

@security(admin)
def delete_user(id: int) -> str:
    return f"Deleted user {id}"

# === Main Program ===

def main():
    # Validate inputs
    email = "user@example.com"
    if not security.validate_email(email):
        print("Invalid email")
        return 1
    
    # Escape user input
    user_input = "'; DROP TABLE users; --"
    if security.detect_sql_injection(user_input):
        print("SQL injection blocked!")
        return 1
    
    safe_input = security.sql_escape(user_input)
    print(f"Safe: {safe_input}")
    
    # Call protected API
    result = transfer_money("account1", 100)
    print(f"Result: {result}")
    
    # Hash sensitive data
    password_hash = security.hash_sha256("secret123")
    print(f"Hash: {password_hash}")
    
    # Logout
    security.set_guest()
    
    return 0

main()
```

---

## Access Levels

| Level | Constant | Description |
|-------|----------|-------------|
| 0 | `LP_ACCESS_GUEST` | Guest (not authenticated) |
| 1 | `LP_ACCESS_USER` | Normal user |
| 2 | `LP_ACCESS_ADMIN` | Administrator |
| 3 | `LP_ACCESS_SUPER` | Super admin |

---

## Security Levels

| Level | Description |
|-------|-------------|
| 0 - none | No security checks |
| 1 - low | Basic validation |
| 2 - medium | Default - standard checks |
| 3 - high | Strict validation, character filtering |
| 4 - critical | Maximum security |

---

## See Also

- [Security Explained](Security-Explained) - How @security works
- [Calling Security Functions](Calling-Security-Functions) - Usage examples
