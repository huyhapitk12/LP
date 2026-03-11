invoice_total = lambda quantity, unit_price:
    subtotal: int = quantity * unit_price
    return subtotal

def test_global_multiline_lambda():
    result: int = invoice_total(12, 5)
    if result != 60:
        raise "global multiline lambda returned unexpected result"
