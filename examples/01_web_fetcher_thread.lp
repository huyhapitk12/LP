import http
import json
import sqlite
import thread

def fetch_post_worker(url: str) -> int:
    body: str = http.get(url)
    payload = json.loads(body)

    post_id: int = int(payload["id"])
    user_id: int = int(payload["userId"])
    title_text: str = str(payload["title"])
    clean_title: str = title_text.strip()
    title_ok: int = 0
    if clean_title != "":
        title_ok = 1

    db: sqlite_db = sqlite.connect("analytics.db")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    sqlite.execute(
        db,
        "INSERT INTO fetch_log (url, post_id, user_id, title_ok) VALUES (?, ?, ?, ?);",
        [url, post_id, user_id, title_ok]
    )
    return post_id

def main():
    print("Fetching remote posts in parallel...")

    db: sqlite_db = sqlite.connect("analytics.db")
    sqlite.execute(db, "PRAGMA journal_mode = WAL;")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    sqlite.execute(db, "DROP TABLE IF EXISTS fetch_log;")
    sqlite.execute(
        db,
        "CREATE TABLE fetch_log (url TEXT, post_id INTEGER, user_id INTEGER, title_ok INTEGER);"
    )

    url1 = "https://jsonplaceholder.typicode.com/posts/1"
    url2 = "https://jsonplaceholder.typicode.com/posts/2"

    t1: thread = thread.spawn(fetch_post_worker, url1)
    t2: thread = thread.spawn(fetch_post_worker, url2)

    first_id: int = thread.join(t1)
    second_id: int = thread.join(t2)

    if first_id != 1:
        raise "unexpected post id for worker 1"
    if second_id != 2:
        raise "unexpected post id for worker 2"

    rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM fetch_log")
    total_rows: int = int(rows[0]["n"])
    if total_rows != 2:
        raise "expected two rows in fetch_log"

    print("Stored rows:")
    print(total_rows)
    print("Done.")

main()
