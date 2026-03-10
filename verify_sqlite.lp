import sqlite

def main():
    print("Checking analytics.db...")
    db: sqlite_db = sqlite.connect("analytics.db")

    total_rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM server")
    total: int = int(total_rows[0]["n"])
    if total != 2:
        raise "Expected 2 rows in analytics.db"

    post1_rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM server WHERE responses = 'https://jsonplaceholder.typicode.com/posts/1'")
    post1: int = int(post1_rows[0]["n"])
    if post1 != 1:
        raise "Missing post 1 row"

    post2_rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM server WHERE responses = 'https://jsonplaceholder.typicode.com/posts/2'")
    post2: int = int(post2_rows[0]["n"])
    if post2 != 1:
        raise "Missing post 2 row"

    print("analytics.db verification passed.")

main()