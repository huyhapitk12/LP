# ============================================
# TEST COMPLETE @security FEATURE
# ============================================

import security

# === 1. Test @security decorator ===

@security(rate_limit=5)
def rate_limited_api(data: str) -> str:
    return f"Processed: {data}"

@security(auth="bearer")
def protected_api() -> str:
    return "Secret data"

@security(admin)
def admin_only_function() -> str:
    return "Admin function executed"

@security(user, readonly)
def user_readonly_function() -> str:
    return "User readonly function"

# === 2. Test security module functions ===

def test_validation():
    print("=== Testing Validation Functions ===")
    
    # Email validation
    result = security.validate_email("test@example.com")
    print(f"validate_email('test@example.com') = {result}")
    
    result = security.validate_email("invalid-email")
    print(f"validate_email('invalid-email') = {result}")
    
    # URL validation
    result = security.validate_url("https://google.com")
    print(f"validate_url('https://google.com') = {result}")
    
    result = security.validate_url("ftp://bad.com")
    print(f"validate_url('ftp://bad.com') = {result}")
    
    # Numeric validation
    result = security.validate_numeric("123.45")
    print(f"validate_numeric('123.45') = {result}")
    
    # Alphanumeric validation
    result = security.validate_alphanumeric("hello_world-123")
    print(f"validate_alphanumeric('hello_world-123') = {result}")
    
    # Identifier validation
    result = security.validate_identifier("my_function_name")
    print(f"validate_identifier('my_function_name') = {result}")
    
    print("")

def test_detection():
    print("=== Testing Detection Functions ===")
    
    # SQL injection detection
    result = security.detect_sql_injection("'; DROP TABLE users; --")
    print(f"detect_sql_injection('...DROP TABLE...') = {result}")
    
    result = security.detect_sql_injection("normal text")
    print(f"detect_sql_injection('normal text') = {result}")
    
    # XSS detection
    result = security.detect_xss("<script>alert('xss')</script>")
    print(f"detect_xss('<script>...') = {result}")
    
    result = security.detect_xss("Hello World")
    print(f"detect_xss('Hello World') = {result}")
    
    print("")

def test_escape():
    print("=== Testing Escape Functions ===")
    
    # SQL escape
    result = security.sql_escape("O'Brien")
    print(f"sql_escape('O\\'Brien') = {result}")
    
    # HTML escape
    result = security.html_escape("<b>Hello</b>")
    print(f"html_escape('<b>Hello</b>') = {result}")
    
    # Strip HTML
    result = security.strip_html("<p>Hello <b>World</b></p>")
    print(f"strip_html('<p>Hello <b>World</b></p>') = {result}")
    
    print("")

def test_hash():
    print("=== Testing Hash Functions ===")
    
    # MD5 hash
    result = security.hash_md5("password123")
    print(f"hash_md5('password123') = {result}")
    
    # SHA256 hash
    result = security.hash_sha256("password123")
    print(f"hash_sha256('password123') = {result}")
    
    print("")

def test_auth_context():
    print("=== Testing Auth Context ===")
    
    # Check initial state
    print(f"is_authenticated() = {security.is_authenticated()}")
    print(f"get_access_level() = {security.get_access_level()}")
    
    # Set as user
    print("\nSetting as user...")
    security.set_authenticated(1, "john_doe")
    print(f"is_authenticated() = {security.is_authenticated()}")
    print(f"get_access_level() = {security.get_access_level()}")
    
    # Set as admin
    print("\nSetting as admin...")
    security.set_authenticated(2, "admin_user")
    print(f"is_authenticated() = {security.is_authenticated()}")
    print(f"get_access_level() = {security.get_access_level()}")
    
    # Logout
    print("\nLogout...")
    security.set_guest()
    print(f"is_authenticated() = {security.is_authenticated()}")
    print(f"get_access_level() = {security.get_access_level()}")
    
    print("")

def test_security_decorator():
    print("=== Testing @security Decorator ===")
    
    # Login as admin to test protected functions
    security.set_authenticated(2, "test_admin")
    
    # Call rate limited API
    print("Calling rate_limited_api()...")
    result = rate_limited_api("test data")
    print(f"Result: {result}")
    
    # Call protected API
    print("\nCalling protected_api()...")
    result = protected_api()
    print(f"Result: {result}")
    
    # Call admin function
    print("\nCalling admin_only_function()...")
    result = admin_only_function()
    print(f"Result: {result}")
    
    # Test access denied
    print("\nLogout and try admin function...")
    security.set_guest()
    result = admin_only_function()  # Should fail
    print(f"Result (should fail): {result}")
    
    print("")

# ============================================
# MAIN
# ============================================
def main():
    print("=" * 50)
    print("    LP Security Module Complete Test")
    print("=" * 50)
    print("")
    
    test_validation()
    test_detection()
    test_escape()
    test_hash()
    test_auth_context()
    test_security_decorator()
    
    print("=" * 50)
    print("    All tests completed!")
    print("=" * 50)
    return 0

main()
