import platform
import sqlite

def main():
    os_name: str = str(platform.os())
    arch_name: str = str(platform.arch())
    core_count: int = platform.cores()

    print("--- HOST SNAPSHOT ---")
    print(os_name)
    print(arch_name)
    print(core_count)

    db: sqlite_db = sqlite.connect("system.db")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    sqlite.execute(db, "DROP TABLE IF EXISTS host_snapshot;")
    sqlite.execute(
        db,
        "CREATE TABLE host_snapshot (os TEXT, arch TEXT, cores INTEGER);"
    )
    sqlite.execute(
        db,
        "INSERT INTO host_snapshot (os, arch, cores) VALUES (?, ?, ?);",
        [os_name, arch_name, core_count]
    )

    rows = sqlite.query(db, "SELECT os, arch, cores FROM host_snapshot")
    print("Database stored:")
    print(str(rows[0]["os"]))
    print(str(rows[0]["arch"]))
    print(int(rows[0]["cores"]))

main()
