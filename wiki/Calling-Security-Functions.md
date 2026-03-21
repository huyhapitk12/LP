# Security Calling Patterns

## 1. Normal Function Calls

When a function has `@security`, you call it normally. Security checks run automatically:

```lp
# Define a function with security
@security(rate_limit=10, auth="bearer")
def transfer_money(to: str, amount: int) -> str:
    return f"Sent {amount} to {to}"

# Call the function - security checks run automatically!
result = transfer_money("account1", 100)

# Results:
# - Rate limit exceeded → return 0, print "Rate limit exceeded"
# - Not authenticated → return 0, print "Authentication required"
# - All checks pass → execute function normally
```

## 2. Call Flow

```
┌─────────────────────────────────────────────────────────────┐
│  transfer_money("account1", 100)                            │
│                                                             │
│  Step 1: Check rate_limit                                   │
│          → If > 10 calls/min → ❌ return 0                  │
│          → If OK → continue                                 │
│                                                             │
│  Step 2: Check authentication                               │
│          → If not logged in → ❌ return 0                   │
│          → If OK → continue                                 │
│                                                             │
│  Step 3: Execute function                                   │
│          → return "Sent 100 to account1"                    │
└─────────────────────────────────────────────────────────────┘
```

## 3. Setting Authentication Context

Before calling functions that require auth, set up the context:

```lp
# Import security module
import security

# Set authenticated user (access level: 0=guest, 1=user, 2=admin)
security.set_authenticated(2, "admin_user")

# Now you can call auth-required functions
transfer_money("account1", 100)  # ✅ OK

# Logout
security.set_guest()

# Try to call auth-required function
transfer_money("account1", 100)  # ❌ FAIL - not authenticated
```

## 4. Security Module - Utility Functions

You can also call security functions directly:

```lp
import security

# === VALIDATION ===

# Validate email
if security.validate_email("test@example.com"):
    print("Email OK")
else:
    print("Invalid email")

# Validate URL
if security.validate_url("https://google.com"):
    print("URL OK")

# Check if input is safe
if security.is_safe_string(user_input):
    print("Input is safe")

# === DETECTION ===

# Detect SQL injection
dangerous = "'; DROP TABLE users; --"
if security.detect_sql_injection(dangerous):
    print("SQL injection detected!")

# Detect XSS
xss_attempt = "<script>alert('xss')</script>"
if security.detect_xss(xss_attempt):
    print("XSS detected!")

# === ESCAPE/SANITIZE ===

# Escape for SQL
safe = security.sql_escape(user_input)
# "O'Brien" → "O''Brien"

# Escape for HTML
safe_html = security.html_escape(content)
# "<b>" → "&lt;b&gt;"

# === RATE LIMIT ===

# Manual rate limit check
if security.check_rate_limit("my_function", 10):
    # Quota available
    do_something()
else:
    # Quota exceeded
    print("Rate limit exceeded")
```

## 5. Complete Example

```lp
import security

# Set user context
security.set_authenticated(1, "john_doe")

# API function with protection
@security(rate_limit=5, auth="bearer")
def api_create_user(name: str, email: str) -> str:
    # Validate input
    if not security.validate_email(email):
        return "Error: Invalid email"
    
    # Sanitize input
    safe_name = security.html_escape(name)
    
    return f"Created user: {safe_name}"

# Main
def main():
    print("=== Calling function with @security ===")
    
    # Call API (security check is automatic)
    result = api_create_user("John", "john@example.com")
    print(result)
    
    # If called more than 5 times/min → blocked
    # If not logged in → blocked
    
    return 0

main()
```

## 6. Execution Flow

```
User code:  transfer_money("acc", 100)
                    │
                    ▼
         ┌─────────────────────┐
         │  @security checks   │
         │  (automatically     │
         │   inserted at       │
         │   function start)   │
         │                     │
         │  1. Rate limit?     │──NO──► return 0
         │  2. Authenticated?  │──NO──► return 0
         │  3. Access level?   │──NO──► return 0
         └─────────────────────┘
                    │
                   YES
                    │
                    ▼
         ┌─────────────────────┐
         │  Function body      │
         │  return "Sent..."   │
         └─────────────────────┘
```

## See Also

- [Security Overview](Security-Explained) - How @security works
- [Security Reference](Security-Module-Reference) - Full API reference
