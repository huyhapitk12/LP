# ============================================
# DEMO @security - Giải thích dễ hiểu
# ============================================

# ❌ VẤN ĐỀ: Hàm KHÔNG có bảo mật
def danger_function(user_input: str) -> str:
    # Ai cũng có thể gọi, không giới hạn
    # Input không được kiểm tra
    # Có thể bị SQL injection, XSS, v.v.
    return user_input

# ✅ GIẢI PHÁP: Hàm CÓ @security

# 1. Giới hạn số lần gọi (Rate Limiting)
# Chỉ cho phép 10 lần gọi mỗi phút
# → Ngăn chặn spam, brute force
@security(rate_limit=10)
def limited_api():
    print("API called")
    return "ok"

# 2. Yêu cầu xác thực (Authentication)
# Người dùng phải đăng nhập mới được gọi
@security(auth="bearer")
def protected_api():
    print("Authenticated API")
    return "secret data"

# 3. Kiểm tra input (Validation)
# Tự động phát hiện input nguy hiểm
@security(validate=True, injection=True)
def safe_query(user_input: str):
    # Nếu user_input chứa SQL injection → bị chặn
    # Nếu user_input chứa XSS → bị chặn
    print("Safe query with: " + user_input)
    return user_input

# 4. Chỉ admin mới được dùng
@security(admin)
def delete_database():
    print("Database deleted!")
    return "deleted"

# 5. Chế độ chỉ đọc (không cho phép ghi/sửa)
@security(readonly)
def view_only():
    print("You can only read")
    return "read only"

# 6. Kết hợp nhiều bảo mật
@security(
    level="high",           # Bảo mật cao
    auth="api_key",         # Yêu cầu API key
    rate_limit=5,           # Tối đa 5 lần/phút
    injection=True,         # Chống SQL injection
    xss=True                # Chống XSS
)
def super_secure_api(data: str):
    print("Super secure: " + data)
    return data

# ============================================
# MAIN - Thử nghiệm
# ============================================
def main():
    print("=== DEMO @security decorator ===")
    print("")
    
    # Gọi các hàm
    print("1. Gọi danger_function (không bảo vệ):")
    danger_function("normal input")
    danger_function("'; DROP TABLE users; --")  # Nguy hiểm!
    print("")
    
    print("2. Gọi limited_api (rate limit 10/phút):")
    limited_api()
    print("")
    
    print("3. Gọi protected_api (cần auth):")
    protected_api()
    print("")
    
    print("4. Gọi safe_query (validate input):")
    safe_query("normal name")
    print("")
    
    print("5. Gọi delete_database (admin only):")
    delete_database()
    print("")
    
    print("6. Gọi view_only (readonly mode):")
    view_only()
    print("")
    
    print("7. Gọi super_secure_api (full protection):")
    super_secure_api("safe data")
    print("")
    
    print("=== Kết thúc demo ===")
    return 0

main()
