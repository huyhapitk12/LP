# Platform & SQLite demo
import platform
import sqlite

def main():
    os: val = platform.os()
    cores: int = platform.cores()
    arch: val = platform.arch()
    
    print("--- SYSTEM DIAGNOSTIC ---")
    print(str(os))
    print(str(arch))
    
    print("Initializing Database...")
    db = sqlite.connect("system.db")
    
    sqlite.execute(db, "CREATE TABLE IF NOT EXISTS sys_logs (os TEXT, cores INTEGER);")
    sqlite.execute(db, "INSERT INTO sys_logs (os, cores) VALUES ('windows', 8);")
    
    rows = sqlite.query(db, "SELECT * FROM sys_logs")
    print("Database persisted entries correctly.")

main()
