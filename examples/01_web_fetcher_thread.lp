# Web Server using HTTP & Threads
import http
import thread
import json
import sqlite

def fetch_worker(url: str) -> int:
    # Worker grabs internet connections!
    data: str = http.get(url)

    # Parses them isolated implicitly!
    db: sqlite_db = sqlite.connect("analytics.db")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    sqlite.execute(db, "INSERT INTO server (responses) VALUES ('" + url + "');")
    return 1

def main():
    print("Initiating Multi-Thread Web Server Scrape!")

    db: sqlite_db = sqlite.connect("analytics.db")
    sqlite.execute(db, "PRAGMA journal_mode = WAL;")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    sqlite.execute(db, "DROP TABLE IF EXISTS server;")
    sqlite.execute(db, "CREATE TABLE server (responses TEXT);")

    link1 = "https://jsonplaceholder.typicode.com/posts/1"
    link2 = "https://jsonplaceholder.typicode.com/posts/2"

    # Spawn parallel HTTP requests natively mapped onto OS threads
    t1: thread = thread.spawn(fetch_worker, link1)
    t2: thread = thread.spawn(fetch_worker, link2)

    # Re-sync execution and verify worker results.
    r1: int = thread.join(t1)
    r2: int = thread.join(t2)
    if r1 != 1:
        raise "fetch_worker 1 failed"
    if r2 != 1:
        raise "fetch_worker 2 failed"

    print("Finished downloads!")

main()