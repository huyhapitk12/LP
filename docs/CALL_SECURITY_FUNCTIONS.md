# Cách gọi hàm có @security

## 1. Gọi hàm bình thường

Khi một hàm có `@security`, bạn vẫn gọi nó như bình thường. Các check bảo mật sẽ chạy tự động:

```lp
# Định nghĩa hàm có security
@security(rate_limit=10, auth="bearer")
def transfer_money(to: str, amount: int) -> str:
    return f"Sent {amount} to {to}"

# Gọi hàm - security checks chạy tự động!
result = transfer_money("account1", 100)

# Nếu:
# - Rate limit exceeded → return 0, in "Rate limit exceeded"
# - Not authenticated → return 0, in "Authentication required"
# - All checks pass → chạy hàm bình thường
```

## 2. Kết quả khi gọi

```
┌─────────────────────────────────────────────────────────────┐
│  transfer_money("account1", 100)                            │
│                                                             │
│  Bước 1: Check rate_limit                                   │
│          → Nếu > 10 lần/phút → ❌ return 0                  │
│          → Nếu OK → tiếp                                    │
│                                                             │
│  Bước 2: Check authentication                               │
│          → Nếu chưa login → ❌ return 0                     │
│          → Nếu OK → tiếp                                    │
│                                                             │
│  Bước 3: Chạy hàm                                           │
│          → return "Sent 100 to account1"                    │
└─────────────────────────────────────────────────────────────┘
```

## 3. Thiết lập authentication context

Trước khi gọi hàm cần auth, bạn cần thiết lập context:

```lp
# Import security module
import security

# Thiết lập user đã login (access level: 0=guest, 1=user, 2=admin)
security.set_authenticated(2, "admin_user")

# Bây giờ có thể gọi hàm cần auth
transfer_money("account1", 100)  # ✅ OK

# Logout
security.set_guest()

# Gọi hàm cần auth
transfer_money("account1", 100)  # ❌ FAIL - not authenticated
```

## 4. Module security - gọi hàm tiện ích

Bạn cũng có thể gọi các hàm security trực tiếp:

```lp
import security

# === VALIDATION ===

# Kiểm tra email
if security.validate_email("test@example.com"):
    print("Email OK")
else:
    print("Email không hợp lệ")

# Kiểm tra URL
if security.validate_url("https://google.com"):
    print("URL OK")

# Kiểm tra input có an toàn
if security.is_safe_string(user_input):
    print("Input an toàn")

# === DETECTION ===

# Phát hiện SQL injection
dangerous = "'; DROP TABLE users; --"
if security.detect_sql_injection(dangerous):
    print("SQL injection detected!")  # → In ra warning

# Phát hiện XSS
xss_attempt = "<script>alert('xss')</script>"
if security.detect_xss(xss_attempt):
    print("XSS detected!")

# === ESCAPE/SANITIZE ===

# Escape cho SQL
safe = security.sql_escape(user_input)
# "O'Brien" → "O''Brien"

# Escape cho HTML
safe_html = security.html_escape(content)
# "<b>" → "&lt;b&gt;"

# === RATE LIMIT ===

# Kiểm tra rate limit thủ công
if security.check_rate_limit("my_function", 10):
    # Còn quota
    do_something()
else:
    # Hết quota
    print("Rate limit exceeded")
```

## 5. Ví dụ hoàn chỉnh

```lp
import security

# Thiết lập user context
security.set_authenticated(1, "john_doe")

# Hàm API với bảo vệ
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
    print("=== Gọi hàm có @security ===")
    
    # Gọi API (security check tự động)
    result = api_create_user("John", "john@example.com")
    print(result)
    
    # Nếu gọi quá 5 lần/phút → bị chặn
    # Nếu chưa login → bị chặn
    
    return 0

main()
```

## 6. Quy trình hoạt động

```
User code:  transfer_money("acc", 100)
                    │
                    ▼
         ┌─────────────────────┐
         │  @security checks   │
         │  (tự động chèn vào  │
         │   đầu hàm)          │
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
