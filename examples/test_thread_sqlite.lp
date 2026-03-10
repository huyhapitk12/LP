import sqlite
import thread

def store_worker(worker_id: int) -> int:
    db: sqlite_db = sqlite.connect("thread_test.db")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    if worker_id == 1:
        sqlite.execute(db, "INSERT INTO thread_results (worker_id, status) VALUES (1, 11);")
    else:
        sqlite.execute(db, "INSERT INTO thread_results (worker_id, status) VALUES (2, 12);")
    return worker_id + 10

def test_thread_join_and_sqlite():
    db: sqlite_db = sqlite.connect("thread_test.db")
    sqlite.execute(db, "PRAGMA journal_mode = WAL;")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    sqlite.execute(db, "DROP TABLE IF EXISTS thread_results;")
    sqlite.execute(db, "CREATE TABLE thread_results (worker_id INTEGER, status INTEGER);")

    t1: thread = thread.spawn(store_worker, 1)
    t2: thread = thread.spawn(store_worker, 2)

    r1: int = thread.join(t1)
    r2: int = thread.join(t2)

    if r1 != 11:
        raise "thread.join returned unexpected result for worker 1"
    if r2 != 12:
        raise "thread.join returned unexpected result for worker 2"

    total_rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM thread_results")
    total: int = int(total_rows[0]["n"])
    if total != 2:
        raise "Expected 2 rows in thread_results"

    worker1_rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM thread_results WHERE worker_id = 1 AND status = 11")
    worker1: int = int(worker1_rows[0]["n"])
    if worker1 != 1:
        raise "Missing worker 1 row"

    worker2_rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM thread_results WHERE worker_id = 2 AND status = 12")
    worker2: int = int(worker2_rows[0]["n"])
    if worker2 != 1:
        raise "Missing worker 2 row"