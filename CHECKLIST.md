# Checklist

Bản này là checklist đầy đủ đã phục dựng từ nội dung checklist cũ.

Quy ước dùng:

- `[ ]` Chưa kiểm tra hoặc chưa đánh dấu
- `[x]` Đã kiểm tra và ổn
- `[-]` Đã kiểm tra một phần / còn mơ hồ

Nếu cần kết quả audit thực tế hiện tại của repo, xem thêm [Repo-Audit](Repo-Audit).

## 1. Tài liệu và định hướng

- [ ] README.md khớp với code thực tế
- [ ] Wiki khớp với README và code
- [ ] Có chỗ nào mô tả feature nhưng chưa implement
- [ ] Có chỗ nào code đã có nhưng tài liệu chưa nói
- [ ] Ví dụ trong docs có chạy được thật
- [ ] Roadmap, changelog, FAQ, troubleshooting có rõ ràng
- [ ] Link trong docs không bị chết
- [ ] Tên project, version, author, license có đồng bộ

## 2. Cấu trúc repo

- [ ] Thư mục được chia hợp lý: src, tests, runtime, docs, vscode-lp
- [ ] File đặt đúng chỗ, không lẫn lộn
- [ ] Không có file thừa, file rác, file cũ
- [ ] Tên file và tên module nhất quán
- [ ] Không có naming typo gây nhầm

## 3. Build và chạy

- [ ] Makefile chạy được
- [ ] compile.sh chạy được
- [ ] build.bat chạy được
- [ ] CI/workflow build thành công
- [ ] Build trên Windows/Linux/macOS không vỡ
- [ ] Output file sinh đúng chỗ
- [ ] Clean target xóa đủ
- [ ] Không có hardcode đường dẫn nguy hiểm

## 4. CLI / main

- [ ] --help hoạt động đúng
- [ ] --version hoặc flag tương đương hoạt động đúng
- [ ] Thiếu tham số thì báo lỗi đúng
- [ ] Input file không tồn tại thì xử lý đúng
- [ ] Output file ghi đè có kiểm soát
- [ ] Exit code đúng cho success/fail
- [ ] CLI không crash khi input xấu

## 5. Lexer / tokenizer

- [ ] Token nhận đúng từ khóa
- [ ] Token nhận đúng identifier, keyword, operator, delimiter
- [ ] Comment và whitespace xử lý đúng
- [ ] String literal và escape sequence đúng
- [ ] Number literal đúng
- [ ] Unicode, CRLF, tab, newline không làm lệch vị trí
- [ ] Token chưa dùng hoặc token thiếu được phát hiện
- [ ] Input rỗng / input rất dài không làm crash
- [ ] Vị trí line/column chính xác
- [ ] Không có overflow, out-of-bounds, null pointer

## 6. Parser

- [ ] Parser khớp với grammar thật
- [ ] Block, statement, expression parse đúng
- [ ] Precedence và associativity đúng
- [ ] Unary/binary operator không bị nhầm
- [ ] Parentheses, call, index, member access đúng
- [ ] if/else, while, for, break, continue, return đúng
- [ ] match nếu có thì parse đủ nhánh
- [ ] class, import, decorator/annotation nếu có thì parse thật
- [ ] async/await nếu có thì parse đúng ngữ cảnh
- [ ] Generic/template nếu có thì parse đúng
- [ ] Parser recovery khi gặp lỗi không quá tệ
- [ ] Không có left recursion / recursion sâu gây crash
- [ ] Không có nhánh TODO quan trọng còn bỏ trống
- [ ] AST tạo ra đúng cấu trúc

## 7. AST / tree

- [ ] Node type đúng
- [ ] Field trong node đầy đủ
- [ ] Không mất thông tin khi đi từ parse sang AST
- [ ] Không cast sai kiểu node
- [ ] Không có node null không được kiểm tra
- [ ] AST dump / debug print hoạt động
- [ ] AST free được sạch

## 8. Semantic analysis / type checking

- [ ] Biến dùng trước khi khai báo được bắt
- [ ] Scope hoạt động đúng
- [ ] Shadowing hợp lý
- [ ] Type mismatch được báo đúng
- [ ] Return type đúng
- [ ] Function call sai số lượng/kiểu tham số bị bắt
- [ ] Generic type nếu có hoạt động đúng
- [ ] Control flow không hợp lệ bị phát hiện
- [ ] Unreachable code / dead code nếu cần thì có cảnh báo

## 9. Codegen / backend

- [ ] Node nào parse được thì codegen cũng xử lý đúng
- [ ] Không có AST node bị bỏ qua âm thầm
- [ ] Sinh mã đúng thứ tự và đúng scope
- [ ] Biến, hàm, call, block, control flow sinh đúng
- [ ] match, class, async/await, generic nếu có thì sinh mã thật
- [ ] Không sinh mã sai kiểu, sai offset, sai tên biến
- [ ] Không có backend path chỉ là stub
- [ ] Có debug info / line mapping nếu cần

## 10. Runtime

- [ ] Runtime có implementation thật, không chỉ header
- [ ] Module json, http, sqlite, thread, gpu, security, parallel có phần chạy thật
- [ ] API khai báo khớp implementation
- [ ] Không có header trống hoặc module chỉ tồn tại trên giấy
- [ ] Resource lifecycle đúng: file handle, socket, thread, memory
- [ ] Không có race condition hoặc global state nguy hiểm
- [ ] Có fallback khi thiếu môi trường/phần cứng

## 11. Standard library / built-in

- [ ] Built-in module có đủ file và hàm
- [ ] API nhất quán giữa docs và code
- [ ] Module load/import hoạt động
- [ ] Import trùng, import vòng, path sai đều được xử lý
- [ ] Các built-in cơ bản như string, math, IO không thiếu

## 12. Feature lớn của ngôn ngữ

- [ ] class/object hoạt động đúng
- [ ] Constructor/method/field access đúng
- [ ] import/module hoạt động đúng
- [ ] match/pattern matching hoạt động đúng
- [ ] async/await có thật hay chỉ là cú pháp giả
- [ ] thread/parallel có thật và an toàn
- [ ] gpu có backend thật hoặc ghi rõ là stub
- [ ] lambda/closure nếu có thì capture đúng
- [ ] yield/generator nếu có thì hoạt động đúng
- [ ] operator overloading nếu có thì nhất quán

## 13. Control flow / expression

- [ ] if/else, vòng lặp, break/continue/return đúng
- [ ] try/catch/finally nếu có thì đúng
- [ ] Nested block không làm lệch scope
- [ ] Biểu thức lồng nhau không làm parser hỏng
- [ ] Gọi hàm, access member, index chain đúng
- [ ] Toán tử gán và toán tử so sánh đúng
- [ ] Literal, string, number xử lý đúng

## 14. Memory safety

- [ ] Không leak bộ nhớ
- [ ] Không double free
- [ ] Không use-after-free
- [ ] Không null dereference
- [ ] Không out-of-bounds
- [ ] Không overflow chỉ số / kích thước buffer
- [ ] Ownership của pointer rõ ràng
- [ ] Free token/AST/symbol table/module cache đầy đủ

## 15. Error handling

- [ ] Lỗi có đúng vị trí dòng/cột
- [ ] Message dễ hiểu
- [ ] Có snippet ngữ cảnh nếu cần
- [ ] Lỗi parse không kéo theo quá nhiều cascade error
- [ ] Lỗi runtime không làm crash vô ích
- [ ] Warning và error được phân biệt rõ
- [ ] Fatal error/panic có báo đầy đủ
- [ ] Không nuốt lỗi âm thầm

## 16. Test

- [ ] Test lexer có thật sự test hành vi
- [ ] Test parser có thật sự test parse
- [ ] Test codegen có thật sự test output
- [ ] Có integration test end-to-end
- [ ] Có regression test cho bug cũ
- [ ] Có test âm tính cho input lỗi
- [ ] Có test edge case: rỗng, rất dài, lồng sâu
- [ ] Test không chỉ là init-state
- [ ] Test không flaky

## 17. VS Code extension

- [ ] package.json đúng
- [ ] Command chạy được
- [ ] Grammar/syntax highlight khớp ngôn ngữ thật
- [ ] Snippet không sai cú pháp
- [ ] Keybinding hoạt động
- [ ] File language id đúng
- [ ] Extension không hứa quá mức so với compiler
- [ ] Highlight không tô sai keyword

## 18. Wiki riêng

- [ ] Wiki tồn tại và có dùng thật
- [ ] Wiki khớp README
- [ ] Wiki mô tả architecture rõ
- [ ] Wiki mô tả syntax rõ
- [ ] Wiki mô tả semantics rõ
- [ ] Wiki có getting started
- [ ] Wiki có troubleshooting
- [ ] Wiki có roadmap/release notes
- [ ] Wiki có API reference nếu cần
- [ ] Wiki không vẽ trước quá xa so với code

## 19. Chất lượng code

- [ ] Prototype khớp định nghĩa
- [ ] Include guard đầy đủ
- [ ] Enum/token/opcode đồng bộ
- [ ] Struct field không lệch
- [ ] Init/cleanup đầy đủ
- [ ] Không có TODO/FIXME quan trọng còn treo
- [ ] Không có warning compiler lớn
- [ ] Không có UB
- [ ] Không có code copy/paste cứng nhắc khó bảo trì

## 20. Độ khớp tổng thể

- [ ] Feature nào là thật
- [ ] Feature nào mới chỉ là roadmap
- [ ] Feature nào chỉ là syntax highlight
- [ ] Feature nào parse được nhưng chưa codegen
- [ ] Feature nào codegen có nhưng runtime chưa có
- [ ] Feature nào docs nói có nhưng repo chưa làm
- [ ] Feature nào có code nhưng chưa có test
- [ ] Mức hoàn thiện tổng thể được ghi rõ

## 21. Wiki và docs audit sâu

- [ ] Wiki có tồn tại và có được dùng không
- [ ] Có wiki thật hay chỉ để trống
- [ ] Wiki có được link từ README không
- [ ] Người mới có tìm thấy không
- [ ] Wiki khớp với README không
- [ ] README nói một kiểu, Wiki nói một kiểu
- [ ] Tên feature, cú pháp, lệnh có đồng nhất không
- [ ] Ví dụ trong Wiki có lỗi thời không
- [ ] Wiki có mô tả kiến trúc không
- [ ] Có sơ đồ tổng thể compiler không
- [ ] Có giải thích luồng lexer → parser → codegen không
- [ ] Có giải thích runtime/module không
- [ ] Wiki có mô tả syntax ngôn ngữ không
- [ ] Từ khóa
- [ ] Biểu thức
- [ ] Block
- [ ] Class/function/import
- [ ] Decorator/annotation nếu có
- [ ] Wiki có mô tả semantics không
- [ ] Biến, scope, type, return
- [ ] async/await, match, generic hoạt động thế nào
- [ ] Cái nào là thật, cái nào là kế hoạch
- [ ] Wiki có ví dụ chạy được không
- [ ] Example code có compile thật không
- [ ] Có file output minh họa không
- [ ] Ví dụ có đồng bộ với test không
- [ ] Wiki có mục getting started không
- [ ] Cài đặt
- [ ] Build
- [ ] Run
- [ ] REPL
- [ ] Compile file đầu tiên
- [ ] Wiki có troubleshooting không
- [ ] Lỗi build thường gặp
- [ ] Lỗi dependency
- [ ] Lỗi path
- [ ] Lỗi Windows/Linux/macOS
- [ ] Wiki có FAQ không
- [ ] Tại sao feature chưa chạy
- [ ] Tại sao cú pháp khác README
- [ ] Tại sao code sample fail
- [ ] Tại sao module không import được
- [ ] Wiki có roadmap không
- [ ] Feature nào planned
- [ ] Feature nào in progress
- [ ] Feature nào done
- [ ] Có đánh dấu rõ mức độ hoàn thiện không
- [ ] Wiki có changelog không
- [ ] Phiên bản nào thêm gì
- [ ] Phiên bản nào sửa gì
- [ ] Có breaking change không
- [ ] Wiki có API reference không
- [ ] Runtime functions
- [ ] Standard library
- [ ] CLI flags
- [ ] Extension commands
- [ ] Wiki có architecture decision records không
- [ ] Vì sao chọn syntax đó
- [ ] Vì sao chọn codegen đó
- [ ] Vì sao chọn runtime đó
- [ ] Có ghi lại trade-off không
- [ ] Wiki có contribution guide không
- [ ] Cách mở PR
- [ ] Cách viết test
- [ ] Cách chạy build
- [ ] Coding style
- [ ] Wiki có release guide không
- [ ] Cách đóng gói
- [ ] Cách tag version
- [ ] Cách publish
- [ ] Checklist trước release
- [ ] Wiki có trang module riêng không
- [ ] json, http, sqlite, thread, gpu, security
- [ ] Từng module có mô tả và ví dụ riêng không
- [ ] Wiki có hình ảnh / sơ đồ không
- [ ] Lexer/parser pipeline
- [ ] AST structure
- [ ] Memory model
- [ ] Module graph
- [ ] Wiki có link chết không
- [ ] Link sang file cũ
- [ ] Link sang ảnh bị hỏng
- [ ] Link sang page chưa tồn tại
- [ ] Wiki có phân quyền hoặc nhầm quyền truy cập không
- [ ] Có trang chỉ maintainer xem được không
- [ ] Có trang nhạy cảm public nhầm không
- [ ] Wiki có phản ánh đúng hiện trạng dự án không
- [ ] Wiki có đang vẽ trước so với code không
- [ ] Có nói feature đã xong trong khi thực tế chưa có không
- [ ] Có làm người đọc hiểu nhầm mức hoàn thiện không

## 22. Bootstrapping, build loop, ecosystem tooling

- [ ] Bootstrapping compiler
- [ ] Compiler có tự compile chính nó được không
- [ ] Có circular dependency khi bootstrap không
- [ ] Build từ source sạch có chạy được không
- [ ] Self-hosting readiness
- [ ] Ngôn ngữ có đủ mạnh để viết chính compiler không
- [ ] Có phần nào vẫn phải viết bằng C vì thiếu feature không
- [ ] Incremental compilation
- [ ] Có compile lại phần thay đổi không
- [ ] Dependency graph có đúng không
- [ ] Có rebuild dư không
- [ ] Hot reload / live compile
- [ ] Có hỗ trợ reload code khi chạy không
- [ ] State có bị mất khi reload không
- [ ] Plugin system
- [ ] Có hỗ trợ plugin cho compiler không
- [ ] Plugin có thể phá compiler không
- [ ] API plugin có ổn định không
- [ ] Extension API ngoài VS Code
- [ ] Có API cho tool khác dùng không
- [ ] Có tách core compiler thành lib không
- [ ] Language server (LSP)
- [ ] Có autocomplete không
- [ ] Có go-to-definition không
- [ ] Có hover type info không
- [ ] Có sync với compiler không
- [ ] Refactoring support
- [ ] Rename symbol
- [ ] Extract function
- [ ] Inline variable
- [ ] Có phá code không
- [ ] Static analysis tools
- [ ] Lint
- [ ] Format
- [ ] Type checker riêng
- [ ] Có reuse AST không
- [ ] Documentation generator
- [ ] Có generate docs từ code không
- [ ] Comment có parse được không
- [ ] Có hỗ trợ annotation không
- [ ] Standard library completeness
- [ ] Có đủ IO, string, math không
- [ ] Có thiếu module cơ bản không
- [ ] API có nhất quán không
- [ ] Error handling philosophy
- [ ] Dùng exception hay return code
- [ ] Có thống nhất toàn hệ thống không
- [ ] Có leak khi error không
- [ ] Panic / fatal error
- [ ] Có crash toàn bộ không
- [ ] Có recover được không
- [ ] Có log đầy đủ không
- [ ] Resource management
- [ ] File handle có đóng không
- [ ] Socket có close không
- [ ] Thread có join không
- [ ] Sandbox / security
- [ ] Code user có thể phá hệ thống không
- [ ] Có hạn chế file/network không
- [ ] Có unsafe mode không
- [ ] Permission model
- [ ] Module có quyền riêng không
- [ ] Có sandbox runtime không
- [ ] Có kiểm soát syscall không
- [ ] Deterministic execution
- [ ] Code có chạy giống nhau mọi lần không
- [ ] Có phụ thuộc timing/thread không
- [ ] Time-dependent behavior
- [ ] Có phụ thuộc system time không
- [ ] Có test reproducibility không
- [ ] Randomness control
- [ ] Có seed được RNG không
- [ ] Có deterministic mode không
- [ ] Floating determinism
- [ ] Kết quả float có ổn định cross-platform không
- [ ] Packaging system
- [ ] Có package manager không
- [ ] Có dependency lock không
- [ ] Có version constraint không
- [ ] Module versioning
- [ ] Có conflict version không
- [ ] Có multiple version cùng lúc không
- [ ] Build profiles
- [ ] Debug vs release
- [ ] Optimization level
- [ ] Feature flags
- [ ] Feature gating
- [ ] Có bật/tắt feature không
- [ ] Feature chưa stable có được kiểm soát không
- [ ] Backward compatibility
- [ ] Version mới có chạy code cũ không
- [ ] Có breaking change không
- [ ] Forward compatibility
- [ ] Code mới có degrade gracefully không
- [ ] Có warning cho feature cũ không
- [ ] Deprecation system
- [ ] Có đánh dấu deprecated không
- [ ] Có warning trước khi remove không
- [ ] Migration tools
- [ ] Có tool chuyển code version cũ không
- [ ] Có auto-fix không
- [ ] Code style consistency
- [ ] Naming convention
- [ ] Formatting rule
- [ ] Có enforce bằng tool không
- [ ] Internal documentation (dev docs)
- [ ] Có doc cho contributor không
- [ ] Có giải thích architecture không
- [ ] Architecture clarity
- [ ] Compiler có chia layer rõ không
- [ ] Có coupling cao không
- [ ] Có module quá lớn không
- [ ] Separation of concerns
- [ ] Lexer/parser/codegen có tách rõ không
- [ ] Runtime có phụ thuộc compiler không
- [ ] Extensibility
- [ ] Thêm syntax mới có dễ không
- [ ] Thêm backend mới có dễ không
- [ ] Maintainability
- [ ] Code có dễ đọc không
- [ ] Có comment outdated không
- [ ] Có hack tạm thời không
- [ ] Test maintainability
- [ ] Test có dễ thêm không
- [ ] Có test bị flaky không
- [ ] Scalability của codebase
- [ ] Khi thêm feature có vỡ kiến trúc không
- [ ] Có cần refactor lớn không
- [ ] Contributor friendliness
- [ ] Có guide đóng góp không
- [ ] Có issue template không
- [ ] Có CI check không
- [ ] Release process
- [ ] Có pipeline release không
- [ ] Có version tagging chuẩn không
- [ ] Có changelog không
- [ ] Project vision consistency
- [ ] Feature có đi đúng hướng không
- [ ] Có nhồi nhét quá nhiều thứ không
- [ ] Reality vs ambition
- [ ] Bao nhiêu % feature là thật
- [ ] Bao nhiêu % là roadmap
- [ ] Có đánh dấu rõ không
- [ ] Người ngoài đọc có bị hiểu nhầm không

## 23. Parser / lexer / AST edge cases

- [ ] Parser recovery khi gặp lỗi
- [ ] Có bỏ qua token để tiếp tục parse không
- [ ] Hay gặp lỗi là dừng ngay
- [ ] Có sinh thêm lỗi dây chuyền quá nhiều không
- [ ] Lookahead / backtracking
- [ ] Có nhìn trước đủ token không
- [ ] Có backtrack sai làm parse lệch không
- [ ] Có nhập nhằng giữa call, index, generic không
- [ ] Ambiguous grammar
- [ ] Cùng một chuỗi có nhiều cách hiểu không
- [ ] Có ưu tiên nhánh sai không
- [ ] Có cần left factoring nhưng chưa làm không
- [ ] Left recursion / right recursion
- [ ] Có gây đệ quy vô hạn không
- [ ] Có stack overflow ở biểu thức dài không
- [ ] Có chỗ recursion sâu hơn cần thiết không
- [ ] AST printing / debug dump
- [ ] Có in cây AST đúng không
- [ ] Có thấy hết node không
- [ ] Có debug dump nào thiếu field quan trọng không
- [ ] Pretty printer / formatter
- [ ] Có format lại code đúng cú pháp không
- [ ] Có giữ semantics không
- [ ] Có làm mất comment hoặc khoảng trắng quan trọng không
- [ ] Tokenizer state machine
- [ ] Trạng thái string/comment/number có chuyển đúng không
- [ ] Có bị kẹt state sau lỗi không
- [ ] Có reset state đúng giữa các file không
- [ ] Tokenizer edge cases
- [ ] Chuỗi rỗng
- [ ] Khoảng trắng liên tiếp
- [ ] Tab, newline, CRLF
- [ ] Comment lồng nhau
- [ ] Ký tự Unicode / non-ASCII
- [ ] String literal / escape sequence
- [ ] \n, \t, \", \\
- [ ] Multiline string
- [ ] String chưa đóng
- [ ] Unicode escape nếu có
- [ ] Number literal
- [ ] Số nguyên lớn
- [ ] Số âm
- [ ] Số thực
- [ ] Exponent 1e10
- [ ] Hex / binary / octal nếu ngôn ngữ có
- [ ] Boolean / null / nil
- [ ] Có keyword riêng không
- [ ] So sánh với null đúng chưa
- [ ] Default value có nhất quán không
- [ ] Comment system
- [ ] //, #, /* */ nếu có
- [ ] Comment sau biểu thức
- [ ] Comment trong block
- [ ] Comment làm hỏng line mapping không
- [ ] Preprocessor / directive nếu có
- [ ] @settings
- [ ] @security
- [ ] import đặc biệt
- [ ] Directive có bị coi nhầm là expression không
- [ ] Constant folding / optimization
- [ ] Biểu thức hằng có được gộp không
- [ ] Có tối ưu sai không
- [ ] Có làm mất side effect không
- [ ] Dead code / unreachable code
- [ ] return rồi còn code phía sau
- [ ] if false
- [ ] while false
- [ ] Code unreachable có báo hay không

## 24. File I/O, artifacts, exit codes, logging

- [ ] File I/O
- [ ] Mở file không tồn tại có báo gì
- [ ] Đọc file lớn có ổn không
- [ ] Encode UTF-8/ANSI có bị lỗi không
- [ ] Đường dẫn có dấu cách có xử lý không
- [ ] Output file generation
- [ ] Ghi file đích có đúng tên không
- [ ] Có overwrite ngoài ý muốn không
- [ ] Có tạo file rác khi build fail không
- [ ] Temporary files
- [ ] Có file tạm bị quên xóa không
- [ ] Có collision giữa nhiều lần build không
- [ ] Có dùng hardcoded temp path không
- [ ] Build artifacts
- [ ] Binary output đúng thư mục chưa
- [ ] Có ghi đè file cũ không
- [ ] Clean target có xóa đủ không
- [ ] Build incremental có an toàn không
- [ ] Debug info
- [ ] Có line number / column number không
- [ ] Stack trace có rõ không
- [ ] Symbol name có đúng không
- [ ] Process exit codes
- [ ] Thành công/trượt có exit code đúng không
- [ ] CLI failure có trả code khác compile error không
- [ ] Script gọi ngoài có phân biệt được không
- [ ] Logging
- [ ] Có log debug/verbose không
- [ ] Log có quá ồn không
- [ ] Có thể tắt log không
- [ ] Có in nhầm log vào stdout thay vì stderr không
- [ ] Error context
- [ ] Lỗi có kèm line snippet không
- [ ] Có chỉ đúng file/source không
- [ ] Có chỉ đúng token gần nhất không
- [ ] Warning system
- [ ] Có cảnh báo thay vì lỗi ở case nhẹ không
- [ ] Warning có bị nhầm thành error không
- [ ] Có option tắt warning không

## 25. Determinism, concurrency, reproducibility, security

- [ ] Recursion depth limit
- [ ] AST/parse tree quá sâu có chết không
- [ ] Có safeguard không
- [ ] Có test input sâu hàng nghìn lớp không
- [ ] Thread safety của compiler
- [ ] Có global state dùng chung không
- [ ] Nhiều compile cùng lúc có đụng nhau không
- [ ] REPL và CLI có lẫn trạng thái không
- [ ] Concurrency safety
- [ ] Có thể compile song song không
- [ ] REPL và compiler chạy đồng thời có bị đụng không
- [ ] Determinism
- [ ] Cùng input có ra output giống nhau không
- [ ] Có phụ thuộc hash order / memory address không
- [ ] Có khác nhau giữa lần chạy không
- [ ] Reproducible build
- [ ] Build cùng môi trường có giống nhau không
- [ ] Có timestamp bẩn vào output không
- [ ] Có path tuyệt đối trong binary không
- [ ] Dependency management
- [ ] Có phụ thuộc ngoài không
- [ ] Có ghi rõ phiên bản không
- [ ] Có runtime dependency thiếu trong README không
- [ ] Platform-specific code
- [ ] Windows-only / Linux-only / POSIX-only
- [ ] #ifdef có bao phủ đủ không
- [ ] Có code path chưa test trên từng OS không
- [ ] CI / workflow
- [ ] Build có chạy tự động không
- [ ] Test có chạy trong CI không
- [ ] Có lỗi do thiếu file sinh ra khi build không
- [ ] Bảo mật và input độc hại
- [ ] Input rất dài
- [ ] Nesting rất sâu
- [ ] Token lạ
- [ ] File rỗng
- [ ] File lỗi encoding
- [ ] Có chống crash do input xấu không
- [ ] thread / parallel
- [ ] Có thread-safe không
- [ ] Có race condition không
- [ ] Shared state xử lý ra sao
- [ ] Join / detach / mutex có đủ không
- [ ] gpu / native module
- [ ] Có backend thật hay chỉ là tên
- [ ] Có gọi API ngoài không
- [ ] Có fallback khi máy không có GPU không
- [ ] Sandbox / security runtime
- [ ] Code user có thể phá hệ thống không
- [ ] Có hạn chế file/network không
- [ ] Có unsafe mode không
- [ ] Permission model runtime
- [ ] Module có quyền riêng không
- [ ] Có sandbox runtime không
- [ ] Có kiểm soát syscall không

## 26. Numeric, Unicode, hashing, buffer, allocator, memory internals

- [ ] Memory fragmentation
- [ ] Có cấp phát quá nhỏ lẻ không
- [ ] Có arena allocator / pool không
- [ ] Có tối ưu cấp phát token/AST không
- [ ] Allocator abstraction
- [ ] Có dùng allocator thống nhất không
- [ ] Có thể đổi allocator không
- [ ] Có chỗ nào gọi malloc trực tiếp bừa bãi không
- [ ] Compiler warnings
- [ ] Có warning nặng nào bị bỏ qua không
- [ ] -Wall -Wextra có sạch không
- [ ] Có cast nguy hiểm không
- [ ] Undefined behavior
- [ ] Dereference null
- [ ] Out-of-bounds
- [ ] Use-after-free
- [ ] Uninitialized read
- [ ] Signed overflow
- [ ] Integer overflow
- [ ] Đếm token dài
- [ ] Kích thước buffer
- [ ] Số lượng node AST
- [ ] Index và offset
- [ ] Parse số quá lớn
- [ ] Floating-point edge cases
- [ ] NaN, inf, -0.0
- [ ] So sánh float
- [ ] Tràn số thực
- [ ] Format/parse round-trip
- [ ] String intern / symbol table
- [ ] Có dedupe symbol không
- [ ] Có leak symbol không
- [ ] Symbol scope có bị chồng sai không
- [ ] Hashing / map behavior
- [ ] Collision handling
- [ ] Resize map
- [ ] Stable lookup
- [ ] Key lifetime
- [ ] Hash of strings/case sensitivity
- [ ] Case sensitivity
- [ ] Keyword có phân biệt hoa thường không
- [ ] Identifier có giữ nguyên case không
- [ ] Module name / file path có bị lệch trên Windows không
- [ ] Unicode handling
- [ ] UTF-8 byte sequence
- [ ] Emoji / combining marks
- [ ] Identifier Unicode nếu có
- [ ] Column count có lệch với ký tự đa byte không
- [ ] Whitespace semantics
- [ ] Tab vs space
- [ ] Newline có ý nghĩa cú pháp không
- [ ] Indentation-based block nếu có
- [ ] Trailing whitespace có làm hỏng lexer không
- [ ] Comments in tricky positions
- [ ] Comment giữa biểu thức
- [ ] Comment sau \ line continuation
- [ ] Comment trong macro/directive nếu có
- [ ] Comment trước / sau decorator
- [ ] Macro / preprocessor expansion
- [ ] Có macro lồng nhau không
- [ ] Có expansion vô hạn không
- [ ] Có giữ vị trí lỗi sau expand không
- [ ] Source map / debug mapping
- [ ] Code sinh ra có map về source không
- [ ] Stack trace có truy ngược được không
- [ ] Error ở runtime có chỉ đúng vị trí gốc không
- [ ] IR / intermediate representation
- [ ] Có IR riêng không
- [ ] IR có sạch và nhất quán không
- [ ] Có optimize trên IR trước codegen không
- [ ] Optimization passes
- [ ] Dead code elimination
- [ ] Constant propagation
- [ ] Common subexpression elimination
- [ ] Loop optimization
- [ ] Có pass nào phá semantics không
- [ ] Backend selection
- [ ] Có nhiều backend không
- [ ] Chọn backend có đúng không
- [ ] Fallback backend có hoạt động không
- [ ] Serialization of compiled artifacts
- [ ] Có cache compile không
- [ ] Cache có invalidation đúng không
- [ ] Version cũ có đọc được artifact cũ không

## 27. End-to-end reality checks

- [ ] Tổng kiểm end-to-end bằng case thực
- [ ] File đơn giản
- [ ] File có lỗi cú pháp
- [ ] File có lỗi type
- [ ] File có import
- [ ] File có class
- [ ] File có async/match/generic
- [ ] File lớn
- [ ] File nhiều module
- [ ] File chạy REPL
- [ ] File build trong CI
- [ ] Runtime behavior
- [ ] Có chạy end-to-end không
- [ ] Có crash khi input hợp lệ nhưng phức tạp không
- [ ] Có khác hành vi giữa build/debug/release không
- [ ] Có phụ thuộc môi trường không
- [ ] Integration path
- [ ] Từ source → lexer → parser → semantic → codegen → runtime
- [ ] Đứt ở bước nào
- [ ] Feature nào chỉ chạy được nửa đường
- [ ] examples/ hoặc code mẫu đi kèm
- [ ] Ví dụ có chạy thật không
- [ ] Cú pháp trong ví dụ có đúng với compiler hiện tại không
- [ ] Có ví dụ nào viết theo README nhưng thực tế fail không
- [ ] README có ví dụ lỗi thời
- [ ] Grammar trong docs có khớp lexer không
- [ ] Command line trong docs có khớp main không
- [ ] Feature list có trùng implementation không
- [ ] Package/release readiness
- [ ] Có release note không
- [ ] Có version tag rõ không
- [ ] Có binary release cho platform khác nhau không
- [ ] Có hướng dẫn cài đặt từ đầu đến cuối không

## 28. Audit theo file và thư mục

- [ ] README.md
- [ ] Mô tả tính năng có khớp code không
- [ ] Ví dụ trong README có chạy được không
- [ ] Tên lệnh, cú pháp, keyword có bị lệch không
- [ ] compiler/src/lexer.c
- [ ] Có nhận đúng token không
- [ ] Token nào được khai báo nhưng chưa dùng
- [ ] Từ khóa có xung đột hoặc thiếu xử lý không
- [ ] compiler/src/parser.c
- [ ] Cú pháp nào đã parse thật
- [ ] Cú pháp nào chỉ mới ghi nhận nhưng chưa parse đủ
- [ ] Nhánh lỗi, TODO, case thiếu, ưu tiên toán tử
- [ ] class, async, await, match, generic, lambda, import, decorator
- [ ] compiler/src/codegen.c
- [ ] Feature nào được sinh mã thật
- [ ] Feature nào chỉ parse được nhưng chưa codegen
- [ ] Có chỗ sinh mã sai kiểu, sai biến, sai scope không
- [ ] compiler/src/semantic.c hoặc phần check kiểu nếu có
- [ ] Kiểm tra kiểu dữ liệu
- [ ] Biến chưa khai báo
- [ ] Scope, shadowing, return type, function call
- [ ] compiler/src/main.c
- [ ] Argument line có xử lý đủ không
- [ ] Có giới hạn cứng, crash, hoặc xử lý thiếu flag không
- [ ] compiler/tests/
- [ ] Test có đủ không
- [ ] Có test hành vi thực hay chỉ test init
- [ ] Feature nào chưa có test
- [ ] runtime/
- [ ] Module nào có header nhưng chưa có implementation
- [ ] API nào khai báo rồi nhưng dùng không được
- [ ] http, json, sqlite, thread, gpu, security, parallel
- [ ] vscode-lp/
- [ ] Syntax highlight có khớp ngôn ngữ thật không
- [ ] Snippet, command, build task có chạy được không
- [ ] Extension có trỏ sai file hay sai grammar không
- [ ] File build và cấu hình
- [ ] Makefile
- [ ] compile.sh
- [ ] build.bat
- [ ] CI/workflow nếu có
- [ ] Có file nào gọi sai đường dẫn hoặc thiếu dependency không

## 29. Mức đồng bộ giữa các tầng

- [ ] Lexer có token nhưng parser không dùng
- [ ] Parser có node nhưng codegen chưa xử lý
- [ ] README có feature nhưng runtime chưa có
- [ ] VS Code hiển thị feature nhưng compiler chưa chạy được
- [ ] Độ khớp giữa AST và codegen
- [ ] Node nào tạo ra được nhưng codegen bỏ qua
- [ ] Node nào codegen xử lý nhưng parser chưa tạo ra
- [ ] Có trường hợp AST hợp lệ nhưng codegen sinh mã sai không
- [ ] Độ khớp tổng thể giữa ý tưởng và hiện thực
- [ ] Cái gì nằm trong README nhưng chưa có code
- [ ] Cái gì có code nhưng chưa có test
- [ ] Cái gì có test nhưng chưa cover feature thật
- [ ] Cái gì chỉ là nhãn/dự định chứ chưa thành tính năng
- [ ] Mức hoàn thiện tổng thể
- [ ] Feature nào chỉ là syntax highlight
- [ ] Feature nào đã parse
- [ ] Feature nào đã codegen
- [ ] Feature nào đã chạy end-to-end
- [ ] Feature nào chỉ mới là kế hoạch

## 30. Lỗi tổng quát cần soi

- [ ] Crash khi input rỗng
- [ ] Null pointer
- [ ] Tràn chỉ số mảng
- [ ] Memory leak
- [ ] Sai precedence
- [ ] Sai xử lý dấu ;, xuống dòng, ngoặc
- [ ] Case lỗi không báo rõ
- [ ] Naming / spelling mistakes
- [ ] scurity / security
- [ ] paralell / parallel
- [ ] genrate / generate
- [ ] intial / initial
- [ ] Keyword bị typo
- [ ] Cross-file consistency
- [ ] Enum trong header khớp implementation
- [ ] Prototype khớp định nghĩa
- [ ] Tên hàm khớp giữa .h và .c
- [ ] Struct field có bị lệch không
- [ ] Memory lifecycle
- [ ] AST được free chưa
- [ ] Token stream được free chưa
- [ ] Symbol table được free chưa
- [ ] Module cache có thoát sạch không
- [ ] Quản lý lỗi và thông báo lỗi
- [ ] Lỗi có đúng vị trí dòng/cột không
- [ ] Message có dễ hiểu không
- [ ] Có case nào crash thay vì báo lỗi không
- [ ] Quản lý bộ nhớ
- [ ] malloc/free có cân bằng không
- [ ] Có leak ở lexer/parser/codegen không
- [ ] Có giữ con trỏ treo sau khi free không
- [ ] Độ an toàn của parse tree / AST
- [ ] Node nào có thể null
- [ ] Có cast sai kiểu node không
- [ ] Có mất dữ liệu khi qua từng bước xử lý không
