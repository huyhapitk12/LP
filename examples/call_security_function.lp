# ===========================================
# DEMO: Gọi hàm có @security
# ===========================================

# Hàm có bảo mật - rate limit 5 lần/phút
@security(rate_limit=5)
def api_get_data(query: str) -> str:
    return f"Data for: {query}"

# Hàm yêu cầu xác thực
@security(auth="bearer")
def protected_api() -> str:
    return "Secret data"

# Hàm chỉ admin được dùng
@security(admin)
def admin_delete(id: int) -> str:
    return f"Deleted {id}"

# Hàm có validate input
@security(injection=True, xss=True)
def safe_query(input: str) -> str:
    return f"Query: {input}"

# ===========================================
# MAIN - Gọi các hàm có @security
# ===========================================
def main():
    print("=== Gọi hàm có @security ===")
    print("")
    
    # Gọi hàm với rate_limit
    print("1. Gọi api_get_data (rate_limit=5):")
    result = api_get_data("test")
    print("   Result: " + result)
    print("")
    
    # Nếu gọi quá 5 lần/phút → bị chặn
    # api_get_data("test1")
    # api_get_data("test2")
    # ... lần thứ 6 sẽ bị BLOCK
    
    # Gọi hàm cần auth
    print("2. Gọi protected_api (auth=bearer):")
    result = protected_api()
    print("   Result: " + result)
    print("")
    
    # Gọi hàm admin
    print("3. Gọi admin_delete (admin only):")
    result = admin_delete(123)
    print("   Result: " + result)
    print("")
    
    # Gọi hàm có validate
    print("4. Gọi safe_query (injection/xss protection):")
    result = safe_query("SELECT * FROM users")
    print("   Result: " + result)
    print("")
    
    # Nếu input độc hại → bị chặn
    # safe_query("'; DROP TABLE users; --")  # BLOCK!
    
    print("=== Kết thúc ===")
    return 0

main()
