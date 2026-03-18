from reportlab.lib.pagesizes import A4
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_CENTER, TA_LEFT, TA_JUSTIFY
from reportlab.lib import colors
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfbase.pdfmetrics import registerFontFamily

# Register Vietnamese fonts
pdfmetrics.registerFont(TTFont('SimHei', '/usr/share/fonts/truetype/chinese/SimHei.ttf'))
pdfmetrics.registerFont(TTFont('Times New Roman', '/usr/share/fonts/truetype/english/Times-New-Roman.ttf'))
registerFontFamily('SimHei', normal='SimHei', bold='SimHei')
registerFontFamily('Times New Roman', normal='Times New Roman', bold='Times New Roman')

doc = SimpleDocTemplate(
    "/home/z/my-project/download/LP_CP_Proposal.pdf",
    pagesize=A4,
    title="LP CP Proposal",
    author="Z.ai",
    creator="Z.ai",
    subject="LP Language Competitive Programming Improvements Proposal"
)

styles = getSampleStyleSheet()

# Custom styles
title_style = ParagraphStyle(
    name='TitleStyle',
    fontName='Times New Roman',
    fontSize=24,
    alignment=TA_CENTER,
    spaceAfter=12,
    textColor=colors.HexColor('#1F4E79')
)

subtitle_style = ParagraphStyle(
    name='SubtitleStyle',
    fontName='Times New Roman',
    fontSize=14,
    alignment=TA_CENTER,
    spaceAfter=24,
    textColor=colors.HexColor('#666666')
)

h1_style = ParagraphStyle(
    name='H1Style',
    fontName='Times New Roman',
    fontSize=16,
    alignment=TA_LEFT,
    spaceBefore=18,
    spaceAfter=12,
    textColor=colors.HexColor('#1F4E79')
)

h2_style = ParagraphStyle(
    name='H2Style',
    fontName='Times New Roman',
    fontSize=13,
    alignment=TA_LEFT,
    spaceBefore=12,
    spaceAfter=8,
    textColor=colors.HexColor('#2E75B6')
)

body_style = ParagraphStyle(
    name='BodyStyle',
    fontName='Times New Roman',
    fontSize=11,
    alignment=TA_JUSTIFY,
    spaceAfter=8,
    leading=16
)

code_style = ParagraphStyle(
    name='CodeStyle',
    fontName='Times New Roman',
    fontSize=10,
    alignment=TA_LEFT,
    spaceAfter=8,
    leftIndent=20,
    backColor=colors.HexColor('#F5F5F5')
)

# Table styles
header_style = ParagraphStyle(
    name='TableHeader',
    fontName='Times New Roman',
    fontSize=10,
    textColor=colors.white,
    alignment=TA_CENTER
)

cell_style = ParagraphStyle(
    name='TableCell',
    fontName='Times New Roman',
    fontSize=9,
    textColor=colors.black,
    alignment=TA_CENTER
)

cell_left_style = ParagraphStyle(
    name='TableCellLeft',
    fontName='Times New Roman',
    fontSize=9,
    textColor=colors.black,
    alignment=TA_LEFT
)

story = []

# Cover page
story.append(Spacer(1, 100))
story.append(Paragraph("<b>LP Language</b>", title_style))
story.append(Paragraph("<b>Competitive Programming Improvements Proposal</b>", title_style))
story.append(Spacer(1, 24))
story.append(Paragraph("De xuat cai thien kha nang lap trinh thi dau", subtitle_style))
story.append(Spacer(1, 48))
story.append(Paragraph("Version 1.0", subtitle_style))
story.append(Paragraph("March 2025", subtitle_style))
story.append(PageBreak())

# Section 1: Executive Summary
story.append(Paragraph("<b>1. Tong quan</b>", h1_style))
story.append(Paragraph(
    "Tai lieu nay de xuat cac cai thien cho ngon ngu LP de ho tro tot hon cho lap trinh thi dau (Competitive Programming - CP). "
    "LP la mot ngon ngu lap trinh nhe voi cu phap giong Python, bien dich truc tiep sang native code khong can GCC/LLVM. "
    "Hien tai, LP da co mot so tinh nang co ban nhung con thieu nhieu tinh nang quan trong cho CP.",
    body_style
))
story.append(Spacer(1, 12))

# Section 2: Current Capabilities
story.append(Paragraph("<b>2. Kha nang hien tai</b>", h1_style))
story.append(Paragraph("<b>2.1 Tinh nang da co</b>", h2_style))

# Table of current features
current_data = [
    [Paragraph('<b>Category</b>', header_style), Paragraph('<b>Features</b>', header_style), Paragraph('<b>Status</b>', header_style)],
    [Paragraph('Math Module', cell_left_style), Paragraph('sqrt, sin, cos, tan, exp, log, pow, factorial, gcd, lcm, ceil, floor, round, fabs', cell_left_style), Paragraph('OK', cell_style)],
    [Paragraph('Random Module', cell_left_style), Paragraph('random, randint, uniform, seed', cell_left_style), Paragraph('OK', cell_style)],
    [Paragraph('Time Module', cell_left_style), Paragraph('time, sleep', cell_left_style), Paragraph('OK', cell_style)],
    [Paragraph('NumPy-like Arrays', cell_left_style), Paragraph('zeros, ones, arange, sum, mean, min, max, sort, dot, matmul, argmax, argmin', cell_left_style), Paragraph('OK', cell_style)],
    [Paragraph('Basic Data Structures', cell_left_style), Paragraph('list, dict, string operations', cell_left_style), Paragraph('OK', cell_style)],
    [Paragraph('Bitwise Operators', cell_left_style), Paragraph('and, or, xor, not, shift left, shift right', cell_left_style), Paragraph('OK', cell_style)],
]

t = Table(current_data, colWidths=[100, 280, 60])
t.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('BACKGROUND', (0, 1), (-1, 1), colors.white),
    ('BACKGROUND', (0, 2), (-1, 2), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 3), (-1, 3), colors.white),
    ('BACKGROUND', (0, 4), (-1, 4), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 5), (-1, 5), colors.white),
    ('BACKGROUND', (0, 6), (-1, 6), colors.HexColor('#F5F5F5')),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 8),
    ('RIGHTPADDING', (0, 0), (-1, -1), 8),
]))
story.append(t)
story.append(Spacer(1, 18))

# Section 3: Missing Features
story.append(Paragraph("<b>3. Tinh nang con thieu (Priority Ranking)</b>", h1_style))

missing_data = [
    [Paragraph('<b>Priority</b>', header_style), Paragraph('<b>Feature</b>', header_style), Paragraph('<b>Description</b>', header_style), Paragraph('<b>Impact</b>', header_style)],
    [Paragraph('P0', cell_style), Paragraph('Fast I/O', cell_left_style), Paragraph('scanf/printf style fast input/output', cell_left_style), Paragraph('Critical', cell_style)],
    [Paragraph('P0', cell_style), Paragraph('Number Theory', cell_left_style), Paragraph('sieve, prime_factor, mod_pow, mod_inverse, extended_gcd', cell_left_style), Paragraph('Critical', cell_style)],
    [Paragraph('P1', cell_style), Paragraph('Data Structures', cell_left_style), Paragraph('Stack, Queue, Deque, Heap, PriorityQueue', cell_left_style), Paragraph('High', cell_style)],
    [Paragraph('P1', cell_style), Paragraph('DSU (Union-Find)', cell_left_style), Paragraph('Disjoint Set Union with path compression', cell_left_style), Paragraph('High', cell_style)],
    [Paragraph('P1', cell_style), Paragraph('Graph Algorithms', cell_left_style), Paragraph('BFS, DFS, Dijkstra, Bellman-Ford, Floyd-Warshall', cell_left_style), Paragraph('High', cell_style)],
    [Paragraph('P2', cell_style), Paragraph('Segment Tree', cell_left_style), Paragraph('Range queries with lazy propagation', cell_left_style), Paragraph('Medium', cell_style)],
    [Paragraph('P2', cell_style), Paragraph('Fenwick Tree (BIT)', cell_left_style), Paragraph('Binary Indexed Tree for prefix sums', cell_left_style), Paragraph('Medium', cell_style)],
    [Paragraph('P2', cell_style), Paragraph('String Algorithms', cell_left_style), Paragraph('KMP, Z-algorithm, rolling hash', cell_left_style), Paragraph('Medium', cell_style)],
    [Paragraph('P3', cell_style), Paragraph('Advanced Graph', cell_left_style), Paragraph('SCC, Topological Sort, LCA, HLD', cell_left_style), Paragraph('Low', cell_style)],
    [Paragraph('P3', cell_style), Paragraph('Geometry', cell_left_style), Paragraph('Point, Line, Polygon, Convex Hull', cell_left_style), Paragraph('Low', cell_style)],
]

t2 = Table(missing_data, colWidths=[50, 100, 200, 60])
t2.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('BACKGROUND', (0, 1), (-1, 1), colors.white),
    ('BACKGROUND', (0, 2), (-1, 2), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 3), (-1, 3), colors.white),
    ('BACKGROUND', (0, 4), (-1, 4), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 5), (-1, 5), colors.white),
    ('BACKGROUND', (0, 6), (-1, 6), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 7), (-1, 7), colors.white),
    ('BACKGROUND', (0, 8), (-1, 8), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 9), (-1, 9), colors.white),
    ('BACKGROUND', (0, 10), (-1, 10), colors.HexColor('#F5F5F5')),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 8),
    ('RIGHTPADDING', (0, 0), (-1, -1), 8),
]))
story.append(t2)
story.append(Spacer(1, 18))

# Section 4: Detailed Proposals
story.append(Paragraph("<b>4. De xuan chi tiet</b>", h1_style))

# 4.1 Fast I/O
story.append(Paragraph("<b>4.1 Fast I/O Module (P0)</b>", h2_style))
story.append(Paragraph(
    "Trong lap trinh thi dau, toc do doc/ghi du lieu la yeu to quyet dinh. Python bi cham vi su dung input()/print(). "
    "LP can module io voi cac ham fast_read(), fast_write() su dung scanf/printf hoac fread/fwrite truc tiep.",
    body_style
))
story.append(Paragraph("<b>API de xuan:</b>", body_style))
story.append(Paragraph(
    "io.read_int() - doc mot so nguyen<br/>"
    "io.read_str() - doc mot chuoi<br/>"
    "io.read_line() - doc mot dong<br/>"
    "io.write(x) - ghi xuat<br/>"
    "io.writeln(x) - ghi xuat voi xuong dong<br/>"
    "io.flush() - day buffer",
    code_style
))

# 4.2 Number Theory
story.append(Paragraph("<b>4.2 Number Theory Module (P0)</b>", h2_style))
story.append(Paragraph(
    "Ly thuyet so la chuyen muc rat thuong gap trong CP. LP can module nt hoac mo rong math voi cac ham:",
    body_style
))
story.append(Paragraph(
    "nt.sieve(n) - sang so nguyen to Eratosthenes, tra ve list bool<br/>"
    "nt.is_prime(n) - kiem tra so nguyen to<br/>"
    "nt.prime_factors(n) - phan tich thanh thua so nguyen to<br/>"
    "nt.mod_pow(base, exp, mod) - luy thua modulo (binary exponentiation)<br/>"
    "nt.mod_inverse(a, mod) - nghich dao modulo<br/>"
    "nt.extended_gcd(a, b) - GCD mo rong, tra ve (gcd, x, y)<br/>"
    "nt.euler_phi(n) - ham Euler totient<br/>"
    "nt.chinese_remainder(remainders, moduli) - Dinh ly Trung Hoa",
    code_style
))

# 4.3 Data Structures
story.append(Paragraph("<b>4.3 Data Structures Module (P1)</b>", h2_style))
story.append(Paragraph(
    "Cau truc du lieu la nen tang cua giai thuat. LP can module ds voi:",
    body_style
))
story.append(Paragraph(
    "<b>Stack:</b> ds.Stack() - push, pop, top, is_empty<br/>"
    "<b>Queue:</b> ds.Queue() - enqueue, dequeue, front, is_empty<br/>"
    "<b>Deque:</b> ds.Deque() - push_front, push_back, pop_front, pop_back<br/>"
    "<b>Heap:</b> ds.Heap() - push, pop, top - min-heap mac dinh<br/>"
    "<b>PriorityQueue:</b> ds.PriorityQueue() - push, pop, top voi priority",
    code_style
))

# 4.4 DSU
story.append(Paragraph("<b>4.4 Disjoint Set Union (P1)</b>", h2_style))
story.append(Paragraph(
    "DSU la cau truc du lieu quan trong cho bai toan doi thong nhat, Kruskal MST, v.v.",
    body_style
))
story.append(Paragraph(
    "dsu = ds.DSU(n) - khoi tao voi n phan tu<br/>"
    "dsu.find(x) - tim root cua x (voi path compression)<br/>"
    "dsu.union(x, y) - hop nhat 2 tap hop<br/>"
    "dsu.same(x, y) - kiem tra cung tap hop<br/>"
    "dsu.size(x) - kich thuoc tap hop chua x",
    code_style
))

# 4.5 Graph Algorithms
story.append(Paragraph("<b>4.5 Graph Algorithms Module (P1)</b>", h2_style))
story.append(Paragraph(
    "Bai toan do thi la chuyen muc rat pho bien. De xuan module graph:",
    body_style
))
story.append(Paragraph(
    "<b>Representation:</b> graph.Graph(n, directed=False)<br/>"
    "g.add_edge(u, v, weight=1) - them canh<br/>"
    "<b>Traversal:</b><br/>"
    "graph.bfs(g, start) - BFS tra ve distances, parents<br/>"
    "graph.dfs(g, start) - DFS tra ve order, parents<br/>"
    "<b>Shortest Path:</b><br/>"
    "graph.dijkstra(g, start) - Dijkstra tra ve distances<br/>"
    "graph.bellman_ford(g, start) - Bellman-Ford hoac am<br/>"
    "graph.floyd_warshall(g) - Floyd-Warshall all-pairs<br/>"
    "<b>Tree/Forest:</b><br/>"
    "graph.is_bipartite(g) - kiem tra do thi 2 phia<br/>"
    "graph.connected_components(g) - cac thanh phan lien thong",
    code_style
))

# 4.6 Segment Tree
story.append(Paragraph("<b>4.6 Segment Tree (P2)</b>", h2_style))
story.append(Paragraph(
    "Segment Tree la cau truc du lieu quan trong cho bai toan range query/update:",
    body_style
))
story.append(Paragraph(
    "seg = ds.SegmentTree(arr, op='sum') - op: sum, min, max, gcd<br/>"
    "seg.query(l, r) - truy van khoang [l, r]<br/>"
    "seg.update(idx, value) - cap nhat phan tu<br/>"
    "seg.range_update(l, r, value) - cap nhat khoang (lazy propagation)",
    code_style
))

# 4.7 Fenwick Tree
story.append(Paragraph("<b>4.7 Fenwick Tree / BIT (P2)</b>", h2_style))
story.append(Paragraph(
    "Binary Indexed Tree don gian hon Segment Tree cho prefix sum:",
    body_style
))
story.append(Paragraph(
    "bit = ds.FenwickTree(n)<br/>"
    "bit.add(idx, delta) - them delta vao vi tri<br/>"
    "bit.prefix_sum(idx) - tong tu 0 den idx<br/>"
    "bit.range_sum(l, r) - tong khoang [l, r]",
    code_style
))

story.append(PageBreak())

# Section 5: Implementation Plan
story.append(Paragraph("<b>5. Ke hoach trien khai</b>", h1_style))

impl_data = [
    [Paragraph('<b>Phase</b>', header_style), Paragraph('<b>Features</b>', header_style), Paragraph('<b>Time</b>', header_style), Paragraph('<b>Files</b>', header_style)],
    [Paragraph('Phase 1', cell_style), Paragraph('Fast I/O, Number Theory', cell_left_style), Paragraph('1-2 days', cell_style), Paragraph('runtime/lp_io.h, lp_nt.h', cell_left_style)],
    [Paragraph('Phase 2', cell_style), Paragraph('Stack, Queue, Deque, Heap', cell_left_style), Paragraph('2-3 days', cell_style), Paragraph('runtime/lp_ds.h', cell_left_style)],
    [Paragraph('Phase 3', cell_style), Paragraph('DSU, Graph algorithms', cell_left_style), Paragraph('3-4 days', cell_style), Paragraph('runtime/lp_graph.h', cell_left_style)],
    [Paragraph('Phase 4', cell_style), Paragraph('Segment Tree, Fenwick Tree', cell_left_style), Paragraph('2-3 days', cell_style), Paragraph('runtime/lp_ds.h', cell_left_style)],
    [Paragraph('Phase 5', cell_style), Paragraph('String algorithms, Geometry', cell_left_style), Paragraph('3-4 days', cell_style), Paragraph('runtime/lp_string.h, lp_geo.h', cell_left_style)],
]

t3 = Table(impl_data, colWidths=[70, 180, 70, 150])
t3.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('BACKGROUND', (0, 1), (-1, 1), colors.white),
    ('BACKGROUND', (0, 2), (-1, 2), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 3), (-1, 3), colors.white),
    ('BACKGROUND', (0, 4), (-1, 4), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 5), (-1, 5), colors.white),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 8),
    ('RIGHTPADDING', (0, 0), (-1, -1), 8),
]))
story.append(t3)
story.append(Spacer(1, 18))

# Section 6: Example Code
story.append(Paragraph("<b>6. Vi du ma nguon LP cho CP</b>", h1_style))

story.append(Paragraph("<b>6.1 Bai toan: Tim so nguyen to (Sieve)</b>", h2_style))
story.append(Paragraph(
    "# Using proposed nt module<br/>"
    "import nt<br/><br/>"
    "n = io.read_int()<br/>"
    "is_prime = nt.sieve(n)<br/>"
    "for i in range(2, n + 1):<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;if is_prime[i]:<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;io.writeln(i)",
    code_style
))

story.append(Paragraph("<b>6.2 Bai toan: Dijkstra shortest path</b>", h2_style))
story.append(Paragraph(
    "import graph<br/>"
    "import io<br/><br/>"
    "n, m = io.read_int(), io.read_int()<br/>"
    "g = graph.Graph(n, directed=True)<br/>"
    "for _ in range(m):<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;u, v, w = io.read_int(), io.read_int(), io.read_int()<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;g.add_edge(u, v, w)<br/><br/>"
    "dist = graph.dijkstra(g, 0)<br/>"
    "for i in range(n):<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;io.writeln(dist[i])",
    code_style
))

story.append(Paragraph("<b>6.3 Bai toan: DSU - Ket noi thanh pho</b>", h2_style))
story.append(Paragraph(
    "import ds<br/>"
    "import io<br/><br/>"
    "n, q = io.read_int(), io.read_int()<br/>"
    "dsu = ds.DSU(n)<br/><br/>"
    "for _ in range(q):<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;typ, x, y = io.read_int(), io.read_int(), io.read_int()<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;if typ == 1:<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;dsu.union(x, y)<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;else:<br/>"
    "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;io.writeln(1 if dsu.same(x, y) else 0)",
    code_style
))

# Section 7: Conclusion
story.append(Paragraph("<b>7. Ket luan</b>", h1_style))
story.append(Paragraph(
    "Voi cac cai thien de xuan o tren, LP se tro thanh mot ngon ngu phu hop cho lap trinh thi dau voi cac loi the:",
    body_style
))
story.append(Paragraph(
    "1. <b>Hieu nang cao:</b> Native compilation, khong can interpreter<br/>"
    "2. <b>Dependencies nhe:</b> Chi can binutils (~5MB)<br/>"
    "3. <b>Cu phap de hoc:</b> Giong Python, de cho nguoi moi bat dau<br/>"
    "4. <b>Thu vien CP day du:</b> Math, Number Theory, Graph, Data Structures",
    body_style
))
story.append(Spacer(1, 12))
story.append(Paragraph(
    "De xuat uu tien trien khai Phase 1 va Phase 2 truoc de dap ung cac bai toan CP co ban, "
    "roi tiep tuc voi Phase 3-5 cho cac tinh nang nang cao.",
    body_style
))

doc.build(story)
print("PDF created successfully!")
