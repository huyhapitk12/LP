# @security Explained Visually

## Simple Concept

```
┌─────────────────────────────────────────────────────────────┐
│                    WITHOUT @security                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   User ──────────────────► Function ────────► Result        │
│                                                             │
│   • Anyone can call                                        │
│   • No rate limiting                                       │
│   • No input validation                                    │
│   • Vulnerable to attacks                                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                      WITH @security                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   User                                                     │
│     │                                                       │
│     ▼                                                       │
│   ┌──────────────┐                                         │
│   │  CHECK #1    │  Rate limit?                            │
│   │  Exceeded    │───YES──► ❌ BLOCK (too many requests)   │
│   │  limit?      │                                         │
│   └──────┬───────┘                                         │
│          │ NO                                               │
│          ▼                                                  │
│   ┌──────────────┐                                         │
│   │  CHECK #2    │  Authentication?                        │
│   │  Logged in?  │───NO───► ❌ BLOCK (not logged in)       │
│   │              │                                         │
│   └──────┬───────┘                                         │
│          │ YES                                              │
│          ▼                                                  │
│   ┌──────────────┐                                         │
│   │  CHECK #3    │  Input validation?                      │
│   │  Malicious   │───YES──► ❌ BLOCK (malicious input)     │
│   │  input?      │                                         │
│   └──────┬───────┘                                         │
│          │ NO                                               │
│          ▼                                                  │
│   ┌──────────────┐                                         │
│   │  CHECK #4    │  Permission?                            │
│   │  Has access? │───NO───► ❌ BLOCK (no permission)       │
│   │              │                                         │
│   └──────┬───────┘                                         │
│          │ YES                                              │
│          ▼                                                  │
│        Function ───────────► ✅ Result                     │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Real-World Example

### LP Code:
```lp
@security(rate_limit=10, auth="bearer", injection=True)
def transfer_money(to_account: str, amount: int) -> str:
    # Transfer money
    return f"Transferred {amount} to {to_account}"
```

### When compiled, it generates equivalent C code:

```c
const char* lp_transfer_money(const char* to_account, int64_t amount) {
    // ========== SECURITY CHECKS ==========
    
    // CHECK 1: Rate limiting
    static time_t last_request = 0;
    static int request_count = 0;
    time_t now = time(NULL);
    
    if (now - last_request >= 60) {
        request_count = 0;
        last_request = now;
    }
    request_count++;
    
    if (request_count > 10) {
        fprintf(stderr, "ERROR: Rate limit exceeded (max 10/minute)\n");
        return "ERROR: Too many requests";
    }
    
    // CHECK 2: Authentication
    if (!current_user_is_authenticated()) {
        fprintf(stderr, "ERROR: Authentication required\n");
        return "ERROR: Please login first";
    }
    
    // CHECK 3: SQL Injection detection
    if (lp_detect_sql_injection(to_account)) {
        fprintf(stderr, "ERROR: Potential SQL injection detected\n");
        return "ERROR: Invalid input";
    }
    
    // ========== END SECURITY CHECKS ==========
    
    // Actual function (if all checks pass)
    char result[256];
    snprintf(result, sizeof(result), "Transferred %lld to %s", 
             (long long)amount, to_account);
    return lp_strdup(result);
}
```

## Before/After Comparison

### Before (no @security):
```lp
def transfer_money(to_account: str, amount: int):
    return f"Transferred {amount} to {to_account}"
```
```
→ Hacker can:
  ✓ Call 1000 times in 1 second (spam)
  ✓ No login required
  ✓ Pass: to_account = "'; DROP TABLE accounts; --"
  → Delete database!
```

### After (with @security):
```lp
@security(rate_limit=10, auth="bearer", injection=True)
def transfer_money(to_account: str, amount: int):
    return f"Transferred {amount} to {to_account}"
```
```
→ Hacker CANNOT:
  ✗ Call more than 10 times/min → BLOCK
  ✗ Not logged in → BLOCK  
  ✗ Pass SQL injection → BLOCK
  
→ Only logged-in users with clean input can use it
```

## @security Options

| Option | Effect | Example |
|--------|--------|---------|
| `rate_limit=10` | Max 10 requests/minute | Prevent spam |
| `auth="bearer"` | Require Bearer token | Only logged-in users |
| `auth="api_key"` | Require API key | Only apps with key |
| `injection=True` | Prevent SQL injection | Block `' OR '1'='1` |
| `xss=True` | Prevent XSS | Block `<script>` |
| `csrf=True` | Prevent CSRF | Require token |
| `admin` | Admin only | Sensitive functions |
| `readonly` | Read-only mode | No data modification |
| `level="high"` | High security | More checks |

## Combining Multiple Options

```lp
# Money transfer API - maximum security
@security(
    level="critical",      # Highest level
    auth="bearer",         # Require login
    rate_limit=5,          # Only 5 times/min
    injection=True,        # Prevent SQL injection
    xss=True,              # Prevent XSS
    csrf=True              # Prevent CSRF
)
def transfer_money(to: str, amount: int):
    # Fully protected
    return f"Transferred {amount} to {to}"

# User info API - moderate security
@security(
    auth="api_key",        # Require API key
    rate_limit=100         # 100 times/min
)
def get_user_info(user_id: int):
    return f"User {user_id}"

# Public API - low security
@security(
    rate_limit=1000        # 1000 times/min
)
def public_search(query: str):
    return f"Results for: {query}"
```

## Summary

```
@security is like a "security guard" standing before the function:

      👤 User
         │
         ▼
    👮 @security (CHECK)
         │
    ┌────┴────┐
    │         │
   PASS      FAIL
    │         │
    ▼         ▼
  🚪 Func    ❌ Block!
```

## See Also

- [Security Module Reference](Security-Module-Reference) - Full API reference
- [Calling Security Functions](Calling-Security-Functions) - Usage examples
