# Ví dụ đơn giản về @security

def main():
    print("=== @security DEMO ===")
    print("")
    
    # GIẢ SỬ: Chúng ta có 2 hàm
    
    # Hàm 1: KHÔNG có bảo mật
    print("1. Hàm KHÔNG có @security:")
    print("   def danger_query(input):")
    print("       return 'SELECT * FROM ' + input")
    print("")
    print("   ❌ Vấn đề:")
    print("      • Ai cũng gọi được")
    print("      • Không giới hạn số lần")
    print("      • Input không kiểm tra")
    print("      • Hacker có thể truyền: users; DROP TABLE users;--")
    print("")
    
    # Hàm 2: CÓ bảo mật
    print("2. Hàm CÓ @security:")
    print("   @security(rate_limit=10, injection=True)")
    print("   def safe_query(input):")
    print("       return 'SELECT * FROM ' + input")
    print("")
    print("   ✅ Bảo vệ:")
    print("      • Tối đa 10 lần gọi/phút")
    print("      • Tự động phát hiện SQL injection")
    print("      • Input độc hại → BỊ CHẶN")
    print("")
    
    print("=== KẾT LUẬN ===")
    print("@security = Tự động thêm code kiểm tra bảo mật")
    print("Giống như có một 'lính gác' trước mỗi hàm!")
    return 0

main()
