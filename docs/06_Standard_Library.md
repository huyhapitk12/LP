# Standard Libraries Matrix
LP embeds native modules dynamically loaded inside compiler steps without complex dynamic environments.

### `sqlite` Module
Map lists natively leveraging Official amalgamation schemas smoothly.
```python
import sqlite
db: sqlite_db = sqlite.connect("test.db")
sqlite.execute(db, "CREATE TABLE users (id INTEGER);")
rows: val = sqlite.query(db, "SELECT * FROM users")
```

### `http` Module
Network evaluation commands hooked locally onto WinHTTP.
```python
import http
res: str = http.get("https://api.example.com")
post_res: str = http.post("https://api.example.com", '{"token": "xyz"}')
```

### `json` Module
JSON recursively deserialized onto native internal C-dictionaries (`LpDict` / `LpVal`).
```python
import json
json_str = '{"data": "lp_string"}'
pyobj: val = json.loads(json_str)

string_out: str = json.dumps(pyobj)
```

### `platform`
Operating system structural constants mapped cleanly.
```python
import platform
os: val = platform.os()    # Outputs "windows" or "linux"
arch: val = platform.arch() # Outputs architectures
cores: int = platform.cores() # CPU hardware bounds
```
