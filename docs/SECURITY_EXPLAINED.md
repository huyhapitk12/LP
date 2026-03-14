# Giải thích @security bằng hình ảnh

## 🎯 Ý tưởng đơn giản

```
┌─────────────────────────────────────────────────────────────┐
│                    KHÔNG CÓ @security                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   User ──────────────────► Hàm ──────────► Kết quả         │
│                                                             │
│   • Ai cũng gọi được                                        │
│   • Không giới hạn số lần                                   │
│   • Input không kiểm tra                                    │
│   • Có thể bị tấn công                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                      CÓ @security                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   User                                                     │
│     │                                                       │
│     ▼                                                       │
│   ┌──────────────┐                                         │
│   │  CHECK #1    │  Rate limit?                            │
│   │  Có vượt quá │───YES──► ❌ BLOCK (quá nhiều request)   │
│   │  giới hạn?   │                                         │
│   └──────┬───────┘                                         │
│          │ NO                                               │
│          ▼                                                  │
│   ┌──────────────┐                                         │
│   │  CHECK #2    │  Authentication?                        │
│   │  Có đăng nhập│───NO───► ❌ BLOCK (chưa đăng nhập)      │
│   │  không?      │                                         │
│   └──────┬───────┘                                         │
│          │ YES                                              │
│          ▼                                                  │
│   ┌──────────────┐                                         │
│   │  CHECK #3    │  Input validation?                      │
│   │  Input có    │───YES──► ❌ BLOCK (input độc hại)       │
│   │  độc hại?    │                                         │
│   └──────┬───────┘                                         │
│          │ NO                                               │
│          ▼                                                  │
│   ┌──────────────┐                                         │
│   │  CHECK #4    │  Permission?                            │
│   │  Có quyền    │───NO───► ❌ BLOCK (không có quyền)      │
│   │  không?      │                                         │
│   └──────┬───────┘                                         │
│          │ YES                                              │
│          ▼                                                  │
│        Hàm ─────────────► ✅ Kết quả                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 📝 Ví dụ thực tế

### Code LP:
```lp
@security(rate_limit=10, auth="bearer", injection=True)
def transfer_money(to_account: str, amount: int) -> str:
    # Chuyển tiền
    return f"Transferred {amount} to {to_account}"
```

### Khi biên dịch, nó sinh ra code C tương đương:

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
    
    // Hàm thực sự (nếu pass tất cả checks)
    char result[256];
    snprintf(result, sizeof(result), "Transferred %lld to %s", 
             (long long)amount, to_account);
    return lp_strdup(result);
}
```

## 🔥 So sánh trước/sau

### Trước (không có @security):
```lp
def transfer_money(to_account: str, amount: int):
    return f"Transferred {amount} to {to_account}"
```
```
→ Hacker có thể:
  ✓ Gọi 1000 lần trong 1 giây (spam)
  ✓ Không cần đăng nhập
  ✓ Truyền: to_account = "'; DROP TABLE accounts; --"
  → Xóa database!
```

### Sau (có @security):
```lp
@security(rate_limit=10, auth="bearer", injection=True)
def transfer_money(to_account: str, amount: int):
    return f"Transferred {amount} to {to_account}"
```
```
→ Hacker KHÔNG thể:
  ✗ Gọi quá 10 lần/phút → BLOCK
  ✗ Không đăng nhập → BLOCK  
  ✗ Truyền SQL injection → BLOCK
  
→ Chỉ user đã đăng nhập, input sạch mới được dùng
```

## 🎨 Các tùy chọn @security

| Tùy chọn | Tác dụng | Ví dụ |
|----------|----------|-------|
| `rate_limit=10` | Giới hạn 10 request/phút | Chống spam |
| `auth="bearer"` | Yêu cầu Bearer token | Chỉ user đã login |
| `auth="api_key"` | Yêu cầu API key | Chỉ app có key |
| `injection=True` | Chống SQL injection | Chặn `' OR '1'='1` |
| `xss=True` | Chống XSS | Chặn `<script>` |
| `csrf=True` | Chống CSRF | Yêu cầu token |
| `admin` | Chỉ admin được dùng | Hàm nhạy cảm |
| `readonly` | Chế độ chỉ đọc | Không ghi dữ liệu |
| `level="high"` | Bảo mật cao | Nhiều check hơn |

## 🤝 Kết hợp nhiều tùy chọn

```lp
# API chuyển tiền - bảo mật tối đa
@security(
    level="critical",      # Level cao nhất
    auth="bearer",         # Cần đăng nhập
    rate_limit=5,          # Chỉ 5 lần/phút
    injection=True,        # Chống SQL injection
    xss=True,              # Chống XSS
    csrf=True              # Chống CSRF
)
def transfer_money(to: str, amount: int):
    # Được bảo vệ đầy đủ
    return f"Transferred {amount} to {to}"

# API xem thông tin - bảo mật vừa phải
@security(
    auth="api_key",        # Cần API key
    rate_limit=100         # 100 lần/phút
)
def get_user_info(user_id: int):
    return f"User {user_id}"

# API công khai - bảo mật thấp
@security(
    rate_limit=1000        # 1000 lần/phút
)
def public_search(query: str):
    return f"Results for: {query}"
```

## 💡 Tóm lại

```
@security giống như một "người bảo vệ" đứng trước cửa hàm:

      🧑‍💼 User
         │
         ▼
    👮‍♂️ @security (KIỂM TRA)
         │
    ┌────┴────┐
    │         │
   PASS      FAIL
    │         │
    ▼         ▼
  🚪 Hàm    ❌ Block!
```

Bạn hiểu hơn chưa? Nếu cần tôi giải thích thêm phần nào, hãy hỏi nhé!
