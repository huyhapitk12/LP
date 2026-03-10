print('START')  
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
    sqlite.execute(db, "CREATE TABLE IF NOT EXISTS server (responses TEXT);")
    sqlite.execute(db, "INSERT INTO server (responses) VALUES ('" + url + "');")
    return 1

def main():
    print("Initiating Multi-Thread Web Server Scrape!")
    
    link1 = "https://jsonplaceholder.typicode.com/posts/1"
    link2 = "https://jsonplaceholder.typicode.com/posts/2"
    
    # Spawn parallel HTTP requests natively mapped onto OS threads
    t1: thread = thread.spawn(fetch_worker, link1)
    t2: thread = thread.spawn(fetch_worker, link2)
    
    # Re-sync Execution thread.
    thread.join(t1)
    thread.join(t2)
    
    print("Finished downloads!")
    
main()
