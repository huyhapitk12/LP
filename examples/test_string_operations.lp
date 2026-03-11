def test_string_operations():
    s = "  Build-42  "

    if s.strip() != "Build-42":
        raise "strip failed"
    if s.lstrip() != "Build-42  ":
        raise "lstrip failed"
    if s.rstrip() != "  Build-42":
        raise "rstrip failed"
    if s.lower().strip() != "build-42":
        raise "lower failed"
    if s.upper().strip() != "BUILD-42":
        raise "upper failed"
    if s.find("Build") != 2:
        raise "find failed"
    if s.replace("Build", "Release").strip() != "Release-42":
        raise "replace failed"
    if s.count("4") != 1:
        raise "count failed"

    parts = "alpha,beta,gamma".split(",")
    if ",".join(parts) != "alpha,beta,gamma":
        raise "split/join failed"

    if not "release".startswith("re"):
        raise "startswith failed"
    if not "release".endswith("se"):
        raise "endswith failed"
    if not "42".isdigit():
        raise "isdigit failed"
    if not "LP".isalpha():
        raise "isalpha failed"
    if not "LP42".isalnum():
        raise "isalnum failed"
