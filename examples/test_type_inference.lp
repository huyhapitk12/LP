def normalize_score(base: int):
    return base * 10

def test_type_inference():
    retries = 3
    project = "LP"
    total = normalize_score(retries)
    label = project + " examples"

    if total != 30:
        raise "inferred int wrong"
    if label != "LP examples":
        raise "inferred string wrong"
