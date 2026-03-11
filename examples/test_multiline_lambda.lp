def test_multiline_lambda():
    multiplier = lambda x, y:
        res: int = x * y
        return res

    result: int = multiplier(10, 5)
    if result != 50:
        raise "multiline lambda returned unexpected result"
