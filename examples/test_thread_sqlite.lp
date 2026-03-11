import sqlite
import thread

def store_worker(worker_id: int) -> int:
    db: sqlite_db = sqlite.connect("thread_test.db")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")

    if worker_id == 1:
        sqlite.execute(
            db,
            "INSERT INTO job_results (worker_id, jobs_done, ok) VALUES (1, 3, 1);"
        )
        return 3

    sqlite.execute(
        db,
        "INSERT INTO job_results (worker_id, jobs_done, ok) VALUES (2, 5, 1);"
    )
    return 5

def test_thread_join_and_sqlite():
    db: sqlite_db = sqlite.connect("thread_test.db")
    sqlite.execute(db, "PRAGMA journal_mode = WAL;")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    sqlite.execute(db, "DROP TABLE IF EXISTS job_results;")
    sqlite.execute(
        db,
        "CREATE TABLE job_results (worker_id INTEGER, jobs_done INTEGER, ok INTEGER);"
    )

    t1: thread = thread.spawn(store_worker, 1)
    t2: thread = thread.spawn(store_worker, 2)

    jobs_worker_1: int = thread.join(t1)
    jobs_worker_2: int = thread.join(t2)

    if jobs_worker_1 != 3:
        raise "thread.join returned unexpected result for worker 1"
    if jobs_worker_2 != 5:
        raise "thread.join returned unexpected result for worker 2"

    row_count = sqlite.query(db, "SELECT COUNT(*) AS n FROM job_results")
    if int(row_count[0]["n"]) != 2:
        raise "expected 2 rows in job_results"

    total_jobs = sqlite.query(db, "SELECT SUM(jobs_done) AS n FROM job_results")
    if int(total_jobs[0]["n"]) != 8:
        raise "unexpected total jobs"

    ok_rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM job_results WHERE ok = 1")
    if int(ok_rows[0]["n"]) != 2:
        raise "expected both rows to be marked ok"
