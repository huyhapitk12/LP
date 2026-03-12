def test_call_growth():
    # Define a lambda with 12 parameters to test ParamList growth during parsing
    # and code generation for anonymous functions as well.
    f = lambda a, b, c, d, e, f, g, h, i, j, k, l: a + b + c + d + e + f + g + h + i + j + k + l
    res = f(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
    if res != 12:
        raise "Parameter growth test failed"
    print("Parameter growth test passed!")
