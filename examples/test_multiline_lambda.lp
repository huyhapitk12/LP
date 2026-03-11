def test_multiline_lambda():
    line_total = lambda quantity, unit_price:
        subtotal: int = quantity * unit_price
        return subtotal

    result: int = line_total(8, 7)
    if result != 56:
        raise "multiline lambda returned unexpected result"
