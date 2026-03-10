# Tiến độ kiểm tra tính năng LP

- [x] Kiểm tra tính năng OOP (`02_oop_game_engine.lp`) - Đã fix và hoạt động tốt.
- [x] Kiểm tra tính năng Web Fetcher & Threading (`01_web_fetcher_thread.lp`)
- [x] Kiểm tra tính năng Memory Arena (`03_memory_arena.lp`)
- [x] Kiểm tra tính năng Parallel For (`04_parallel_for.lp`)
- [x] Kiểm tra tính năng SQLite (`05_system_sqlite.lp`)
- [x] Tổng hợp lại lỗi và sửa lỗi (nếu có)
- [x] Kiểm tra compiler có lỗi không
- [ ] Kiểm tra tính năng HTTP
- [x] Kiểm tra tính năng Threading
- [ ] Kiểm tra tính năng JSON
- [ ] Kiểm tra tính năng REPL
- [x] Kiểm tra tính năng Built-in Testing Framework
- [ ] Kiểm tra tính năng Profiler Mode
- [ ] Kiểm tra tính năng C API Generator
- [ ] Kiểm tra tính năng Hot Reload Development Mode
- [ ] Kiểm tra tính năng Enhanced String Operations
- [ ] Kiểm tra tính năng Type Inference
- [ ] Kiểm tra tính năng Class Inheritance & Polymorphism
- [ ] Kiểm tra tính năng Better Error Messages
- [x] Kiểm tra tính năng Parallel For - "Multi-threading zero effort"
- [x] Kiểm tra tính năng True Multithreading
- [ ] Kiểm tra tính năng Multiline Lambda
- [ ] Kiểm tra tính năng Private / Protected thật sự
- [ ] Kiểm tra tính năng Performance Memory Control
- [ ] Kiểm tra tính năng Binary Distribution dễ dàng
- [ ] Kiểm tra tính năng Cross-Platform Support

Hiện tại:

- `thread.spawn(...)` đã được harden ở compiler/runtime LP:
  - chỉ chấp nhận named LP function
  - tối đa 1 argument
  - return type phải là `int` hoặc `void`
- `thread.join(...)` đã trả về `int`
- Regression local đã pass:
  - `build\lp.exe test examples`
- Smoke test Windows đã pass bằng `cmd.exe /c`:
  - `build\lp.exe examples\01_web_fetcher_thread.lp -c web_thread_smoke.exe`
  - `cmd.exe /c web_thread_smoke.exe`
  - `build\lp.exe verify_sqlite.lp -c verify.exe`
  - `cmd.exe /c verify.exe`
- Re-smoke SQLite đã pass:
  - `build\lp.exe examples\05_system_sqlite.lp -c system_sqlite_smoke.exe`
  - `cmd.exe /c system_sqlite_smoke.exe`
- Negative checks đã pass:
  - worker không phải named LP function bị reject ở compile-time
  - worker có hơn 1 tham số bị reject
  - worker return type khác `int`/`void` bị reject