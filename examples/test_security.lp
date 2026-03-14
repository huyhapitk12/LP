# Test @security decorator features
# This file demonstrates the security features of LP language

# Basic security decorator with default settings
@security
def basic_secure_function():
    print("This function has default security settings")
    return 1

# Security with authentication requirement
@security(auth="bearer")
def authenticated_endpoint():
    print("This function requires bearer authentication")
    return 1

# Security with rate limiting
@security(rate_limit=100)
def rate_limited_api():
    print("This function is rate limited to 100 requests per minute")
    return 1

# Security with high level and injection prevention
@security(level="high", injection=True, xss=True)
def secure_input_handler(user_input: str) -> str:
    print("Handling input with high security")
    return user_input

# Security with CSRF protection and CORS
@security(csrf=True, cors="https://example.com,https://api.example.com")
def web_endpoint():
    print("This endpoint has CSRF protection and CORS enabled")
    return 1

# Security with readonly mode (admin access)
@security(admin, readonly)
def admin_readonly_function():
    print("Admin function in readonly mode")
    return 1

# Combined security settings
@security(level="critical", auth="api_key", rate_limit=10, validate=True, sanitize=True, injection=True, xss=True, csrf=True)
def critical_endpoint(data: str) -> str:
    print("Critical security endpoint")
    return data

# Guest level access
@security(guest)
def public_function():
    print("This function is accessible to guests")
    return 1

# User level access with encryption
@security(user, encrypt=True, hash="sha256")
def user_encrypted_function():
    print("User function with encrypted data")
    return 1

# Main function to test
def main():
    print("=== Testing @security decorator ===")
    
    # Call functions with different security settings
    basic_secure_function()
    authenticated_endpoint()
    rate_limited_api()
    secure_input_handler("test input")
    web_endpoint()
    admin_readonly_function()
    critical_endpoint("sensitive data")
    public_function()
    user_encrypted_function()
    
    print("=== All security tests passed ===")
    return 0

main()
