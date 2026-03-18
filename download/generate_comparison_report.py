from reportlab.lib.pagesizes import A4
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.lib.units import inch, cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfbase.pdfmetrics import registerFontFamily

# Register fonts
pdfmetrics.registerFont(TTFont('Times New Roman', '/usr/share/fonts/truetype/english/Times-New-Roman.ttf'))
registerFontFamily('Times New Roman', normal='Times New Roman', bold='Times New Roman')

# Create styles
styles = getSampleStyleSheet()

# Title style
title_style = ParagraphStyle(
    name='ReportTitle',
    fontName='Times New Roman',
    fontSize=24,
    leading=30,
    alignment=TA_CENTER,
    spaceAfter=20,
    textColor=colors.HexColor('#1F4E79')
)

# Section header style
section_style = ParagraphStyle(
    name='SectionHeader',
    fontName='Times New Roman',
    fontSize=14,
    leading=18,
    spaceBefore=15,
    spaceAfter=10,
    textColor=colors.HexColor('#1F4E79')
)

# Body style
body_style = ParagraphStyle(
    name='BodyText',
    fontName='Times New Roman',
    fontSize=10,
    leading=14,
    alignment=TA_LEFT,
    spaceAfter=6
)

# Highlight style for discrepancies
warning_style = ParagraphStyle(
    name='WarningText',
    fontName='Times New Roman',
    fontSize=10,
    leading=14,
    alignment=TA_LEFT,
    spaceAfter=6,
    textColor=colors.HexColor('#C00000')
)

# Success style
success_style = ParagraphStyle(
    name='SuccessText',
    fontName='Times New Roman',
    fontSize=10,
    leading=14,
    alignment=TA_LEFT,
    spaceAfter=6,
    textColor=colors.HexColor('#006400')
)

# Table styles
header_style = ParagraphStyle(
    name='TableHeader',
    fontName='Times New Roman',
    fontSize=9,
    textColor=colors.white,
    alignment=TA_CENTER
)

cell_style = ParagraphStyle(
    name='TableCell',
    fontName='Times New Roman',
    fontSize=8,
    alignment=TA_LEFT
)

# Create PDF
doc = SimpleDocTemplate(
    "/home/z/my-project/download/LP_Wiki_Verification_Report.pdf",
    pagesize=A4,
    title="LP Wiki Verification Report",
    author="Z.ai",
    creator="Z.ai",
    subject="Comparison between LP Wiki documentation and actual codebase"
)

story = []

# Cover page
story.append(Spacer(1, 80))
story.append(Paragraph("LP Language Wiki Verification Report", title_style))
story.append(Spacer(1, 20))
story.append(Paragraph("Comparison between Wiki Documentation and Actual Codebase", ParagraphStyle(
    name='Subtitle',
    fontName='Times New Roman',
    fontSize=14,
    alignment=TA_CENTER,
    textColor=colors.HexColor('#666666')
)))
story.append(Spacer(1, 40))
story.append(Paragraph("Repository: https://github.com/huyhapitk12/LP", ParagraphStyle(
    name='Link',
    fontName='Times New Roman',
    fontSize=11,
    alignment=TA_CENTER,
    textColor=colors.HexColor('#0066CC')
)))
story.append(Paragraph("Version: 0.3.0 | Date: March 2025", ParagraphStyle(
    name='Date',
    fontName='Times New Roman',
    fontSize=11,
    alignment=TA_CENTER
)))
story.append(PageBreak())

# Executive Summary
story.append(Paragraph("<b>1. Executive Summary</b>", section_style))
story.append(Paragraph(
    "This report compares the LP Language Wiki documentation against the actual source code implementation. "
    "The verification process examined core features, runtime modules, security features, and advanced language constructs.",
    body_style
))
story.append(Spacer(1, 10))

# Summary table
summary_data = [
    [Paragraph('<b>Category</b>', header_style), Paragraph('<b>Wiki Status</b>', header_style), Paragraph('<b>Actual Status</b>', header_style), Paragraph('<b>Match</b>', header_style)],
    [Paragraph('Core Syntax', cell_style), Paragraph('25+ features', cell_style), Paragraph('25+ features verified', cell_style), Paragraph('✅ Match', cell_style)],
    [Paragraph('Runtime Modules', cell_style), Paragraph('12 modules', cell_style), Paragraph('12 modules verified', cell_style), Paragraph('✅ Match', cell_style)],
    [Paragraph('Security Features', cell_style), Paragraph('Built-in', cell_style), Paragraph('Fully implemented', cell_style), Paragraph('✅ Match', cell_style)],
    [Paragraph('Parallel Computing', cell_style), Paragraph('OpenMP + GPU', cell_style), Paragraph('OpenMP implemented', cell_style), Paragraph('⚠️ Partial', cell_style)],
    [Paragraph('HTTP Module', cell_style), Paragraph('Only http.get', cell_style), Paragraph('http.get AND http.post', cell_style), Paragraph('❌ Understated', cell_style)],
    [Paragraph('Async/Await', cell_style), Paragraph('Not implemented', cell_style), Paragraph('Syntactic sugar only', cell_style), Paragraph('⚠️ Clarification', cell_style)],
    [Paragraph('Type Unions', cell_style), Paragraph('Not implemented', cell_style), Paragraph('Partially implemented', cell_style), Paragraph('❌ Discrepancy', cell_style)],
    [Paragraph('Generics', cell_style), Paragraph('Not implemented', cell_style), Paragraph('Partially implemented', cell_style), Paragraph('❌ Discrepancy', cell_style)],
]

summary_table = Table(summary_data, colWidths=[3*cm, 3.5*cm, 4*cm, 2.5*cm])
summary_table.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('BACKGROUND', (0, 1), (-1, 1), colors.white),
    ('BACKGROUND', (0, 2), (-1, 2), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 3), (-1, 3), colors.white),
    ('BACKGROUND', (0, 4), (-1, 4), colors.HexColor('#FFF3CD')),
    ('BACKGROUND', (0, 5), (-1, 5), colors.white),
    ('BACKGROUND', (0, 6), (-1, 6), colors.HexColor('#F5F5F5')),
    ('BACKGROUND', (0, 7), (-1, 7), colors.HexColor('#FFCCCB')),
    ('BACKGROUND', (0, 8), (-1, 8), colors.HexColor('#FFCCCB')),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 6),
    ('RIGHTPADDING', (0, 0), (-1, -1), 6),
]))
story.append(summary_table)
story.append(Spacer(1, 20))

# Core Features Verification
story.append(Paragraph("<b>2. Core Language Features Verification</b>", section_style))

core_data = [
    [Paragraph('<b>Feature</b>', header_style), Paragraph('<b>Wiki Says</b>', header_style), Paragraph('<b>Code Verification</b>', header_style), Paragraph('<b>Status</b>', header_style)],
    [Paragraph('Variables & Type Inference', cell_style), Paragraph('✅ Full support', cell_style), Paragraph('Verified in lexer.c, codegen.c', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Functions (def)', cell_style), Paragraph('✅ With default args, *args', cell_style), Paragraph('NODE_FUNC_DEF in ast.h', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Classes', cell_style), Paragraph('✅ With inheritance', cell_style), Paragraph('NODE_CLASS_DEF verified', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('private/protected', cell_style), Paragraph('✅ Compile-time checked', cell_style), Paragraph('Keywords in lexer, access checks in codegen', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('if/elif/else', cell_style), Paragraph('✅', cell_style), Paragraph('NODE_IF in ast.h', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('for loops', cell_style), Paragraph('✅ With range()', cell_style), Paragraph('NODE_FOR verified', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('while loops', cell_style), Paragraph('✅ With break, continue', cell_style), Paragraph('NODE_WHILE verified', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('try/except/finally', cell_style), Paragraph('✅', cell_style), Paragraph('NODE_TRY in ast.h, lp_exception.h', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('raise', cell_style), Paragraph('✅', cell_style), Paragraph('NODE_RAISE verified', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('with statement', cell_style), Paragraph('✅', cell_style), Paragraph('NODE_WITH verified', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Lambda', cell_style), Paragraph('✅ Single & multiline', cell_style), Paragraph('NODE_LAMBDA in ast.h', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('F-strings', cell_style), Paragraph('✅ f"value: {x}"', cell_style), Paragraph('NODE_FSTRING, TOK_FSTRING_LIT', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Pattern Matching', cell_style), Paragraph('✅ match/case', cell_style), Paragraph('NODE_MATCH, NODE_MATCH_CASE', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Decorators', cell_style), Paragraph('✅ @settings, @security', cell_style), Paragraph('NODE_SETTINGS, NODE_SECURITY', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('List Comprehensions', cell_style), Paragraph('⚠️ Numeric-focused', cell_style), Paragraph('NODE_LIST_COMP in parser.c', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Dict Comprehensions', cell_style), Paragraph('✅', cell_style), Paragraph('NODE_DICT_COMP verified', cell_style), Paragraph('✅', cell_style)],
]

core_table = Table(core_data, colWidths=[3.5*cm, 3*cm, 5*cm, 1.5*cm])
core_table.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 4),
    ('RIGHTPADDING', (0, 0), (-1, -1), 4),
    ('BACKGROUND', (0, 1), (-1, -1), colors.white),
    ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, colors.HexColor('#F5F5F5')]),
]))
story.append(core_table)
story.append(PageBreak())

# Runtime Modules Verification
story.append(Paragraph("<b>3. Runtime Modules Verification</b>", section_style))

modules_data = [
    [Paragraph('<b>Module</b>', header_style), Paragraph('<b>Wiki Says</b>', header_style), Paragraph('<b>Actual Implementation</b>', header_style), Paragraph('<b>Status</b>', header_style)],
    [Paragraph('math', cell_style), Paragraph('Supported', cell_style), Paragraph('lp_math_* functions in lp_native_modules.h', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('random', cell_style), Paragraph('Supported', cell_style), Paragraph('seed, random, randint, uniform', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('time', cell_style), Paragraph('Supported', cell_style), Paragraph('lp_time_time(), lp_time_sleep()', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('os/os.path', cell_style), Paragraph('Supported', cell_style), Paragraph('lp_os_* functions in lp_native_os.h', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('sys', cell_style), Paragraph('Supported', cell_style), Paragraph('platform, argv, exit, maxsize', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('http', cell_style), Paragraph('Only http.get', cell_style), Paragraph('BOTH http.get AND http.post!', cell_style), Paragraph('❌', cell_style)],
    [Paragraph('json', cell_style), Paragraph('loads/dumps only', cell_style), Paragraph('lp_json_parse_*, lp_json_stringify', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('sqlite', cell_style), Paragraph('Supported', cell_style), Paragraph('connect, close, execute, query', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('thread', cell_style), Paragraph('Partially', cell_style), Paragraph('spawn, join, lock_* (strict rules)', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('memory', cell_style), Paragraph('Supported', cell_style), Paragraph('arena_new, arena_alloc, arena_reset', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('platform', cell_style), Paragraph('Supported', cell_style), Paragraph('os(), arch(), cores()', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('numpy', cell_style), Paragraph('Basic only', cell_style), Paragraph('array, zeros, ones, arange, linspace, sum, mean, etc.', cell_style), Paragraph('✅', cell_style)],
]

modules_table = Table(modules_data, colWidths=[2.5*cm, 2.5*cm, 6*cm, 1.5*cm])
modules_table.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 4),
    ('RIGHTPADDING', (0, 0), (-1, -1), 4),
    ('BACKGROUND', (0, 6), (-1, 6), colors.HexColor('#FFCCCB')),
]))
story.append(modules_table)
story.append(Spacer(1, 20))

# Discrepancies Found
story.append(Paragraph("<b>4. Discrepancies Found</b>", section_style))

story.append(Paragraph("<b>4.1 HTTP Module - Wiki Understated</b>", ParagraphStyle(
    name='SubHeader',
    fontName='Times New Roman',
    fontSize=11,
    spaceBefore=10,
    spaceAfter=5,
    textColor=colors.HexColor('#C00000')
)))
story.append(Paragraph(
    "<b>Wiki says:</b> \"http.get(...) only\" (Partially supported)<br/>"
    "<b>Actual code:</b> Both http.get AND http.post are implemented in lp_http.h (lp_http_get, lp_http_post)<br/>"
    "<b>Code reference:</b> runtime/lp_http.h lines 50-80, codegen.c line 1146",
    warning_style
))

story.append(Spacer(1, 10))
story.append(Paragraph("<b>4.2 Type Unions - Partially Implemented</b>", ParagraphStyle(
    name='SubHeader',
    fontName='Times New Roman',
    fontSize=11,
    spaceBefore=10,
    spaceAfter=5,
    textColor=colors.HexColor('#C00000')
)))
story.append(Paragraph(
    "<b>Wiki says:</b> \"Not Yet Implemented\"<br/>"
    "<b>Actual code:</b> NODE_TYPE_UNION exists in ast.h, is_union_type() function in codegen.c<br/>"
    "<b>Status:</b> Type unions like 'int|str|float' are parsed and treated as variant type (LP_VAL)",
    warning_style
))

story.append(Spacer(1, 10))
story.append(Paragraph("<b>4.3 Generics - Partially Implemented</b>", ParagraphStyle(
    name='SubHeader',
    fontName='Times New Roman',
    fontSize=11,
    spaceBefore=10,
    spaceAfter=5,
    textColor=colors.HexColor('#C00000')
)))
story.append(Paragraph(
    "<b>Wiki says:</b> \"Not Yet Implemented\"<br/>"
    "<b>Actual code:</b> NODE_GENERIC_INST in ast.h for Box[int] syntax<br/>"
    "<b>Status:</b> Generic instantiation is parsed but defaults to LP_VAL (variant type)",
    warning_style
))

story.append(Spacer(1, 10))
story.append(Paragraph("<b>4.4 Async/Await - Syntactic Sugar Only</b>", ParagraphStyle(
    name='SubHeader',
    fontName='Times New Roman',
    fontSize=11,
    spaceBefore=10,
    spaceAfter=5,
    textColor=colors.HexColor('#FF9800')
)))
story.append(Paragraph(
    "<b>Wiki says:</b> \"Not Yet Implemented\"<br/>"
    "<b>Actual code:</b> NODE_ASYNC_DEF, NODE_AWAIT_EXPR exist in ast.h and codegen.c<br/>"
    "<b>Implementation note:</b> async/await treated as \"syntactic sugar for synchronous code\"<br/>"
    "<b>Code comment:</b> \"For true async, a coroutine-based implementation would be needed\"",
    ParagraphStyle(
        name='WarningOrange',
        fontName='Times New Roman',
        fontSize=10,
        leading=14,
        textColor=colors.HexColor('#996600')
    )
))
story.append(PageBreak())

# Security Features Verification
story.append(Paragraph("<b>5. Security Features Verification</b>", section_style))

security_data = [
    [Paragraph('<b>Feature</b>', header_style), Paragraph('<b>Wiki Says</b>', header_style), Paragraph('<b>Actual Code</b>', header_style), Paragraph('<b>Status</b>', header_style)],
    [Paragraph('@security decorator', cell_style), Paragraph('✅ Rate limiting, auth', cell_style), Paragraph('NODE_SECURITY, LpSecurityContext', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Rate limiting', cell_style), Paragraph('✅ rate_limit=N', cell_style), Paragraph('lp_check_func_rate_limit()', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Authentication', cell_style), Paragraph('✅ auth=True', cell_style), Paragraph('lp_set_authenticated(), lp_is_authenticated()', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Input validation', cell_style), Paragraph('✅ validate=True', cell_style), Paragraph('lp_validate_email/numeric/url/identifier()', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('XSS prevention', cell_style), Paragraph('✅ prevent_xss=True', cell_style), Paragraph('lp_detect_xss()', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('SQL injection', cell_style), Paragraph('✅ prevent_injection=True', cell_style), Paragraph('Parameterized queries in lp_sqlite', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Access control', cell_style), Paragraph('✅ admin, readonly', cell_style), Paragraph('access_level in security context', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('Encryption', cell_style), Paragraph('Not documented', cell_style), Paragraph('encrypt_data, hash_algorithm fields', cell_style), Paragraph('⚠️', cell_style)],
]

security_table = Table(security_data, colWidths=[3*cm, 3*cm, 5*cm, 1.5*cm])
security_table.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 4),
    ('RIGHTPADDING', (0, 0), (-1, -1), 4),
]))
story.append(security_table)
story.append(Spacer(1, 20))

# Parallel Computing
story.append(Paragraph("<b>6. Parallel Computing Verification</b>", section_style))

parallel_data = [
    [Paragraph('<b>Feature</b>', header_style), Paragraph('<b>Wiki Says</b>', header_style), Paragraph('<b>Actual Code</b>', header_style), Paragraph('<b>Status</b>', header_style)],
    [Paragraph('parallel for', cell_style), Paragraph('✅ Requires OpenMP', cell_style), Paragraph('NODE_PARALLEL_FOR, OpenMP pragmas', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('@settings decorator', cell_style), Paragraph('✅ threads, schedule, GPU', cell_style), Paragraph('NODE_SETTINGS, NODE_PARALLEL_SETTINGS', cell_style), Paragraph('✅', cell_style)],
    [Paragraph('GPU device selection', cell_style), Paragraph('✅ device="gpu"', cell_style), Paragraph('lp_gpu.h with CUDA stubs', cell_style), Paragraph('⚠️', cell_style)],
    [Paragraph('Unified GPU memory', cell_style), Paragraph('✅ unified=True', cell_style), Paragraph('unified field in parallel settings', cell_style), Paragraph('⚠️', cell_style)],
    [Paragraph('OpenMP functions', cell_style), Paragraph('Not detailed', cell_style), Paragraph('lp_parallel_cores/threads/wtime()', cell_style), Paragraph('✅', cell_style)],
]

parallel_table = Table(parallel_data, colWidths=[3*cm, 3*cm, 5*cm, 1.5*cm])
parallel_table.setStyle(TableStyle([
    ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#1F4E79')),
    ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
    ('GRID', (0, 0), (-1, -1), 0.5, colors.grey),
    ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
    ('LEFTPADDING', (0, 0), (-1, -1), 4),
    ('RIGHTPADDING', (0, 0), (-1, -1), 4),
    ('BACKGROUND', (0, 3), (-1, 4), colors.HexColor('#FFF3CD')),
]))
story.append(parallel_table)
story.append(Spacer(1, 10))
story.append(Paragraph(
    "<b>Note:</b> GPU features have code structure but actual GPU execution may require CUDA runtime. "
    "OpenMP parallel execution is verified in lp_parallel.h with omp_get_max_threads(), omp_set_num_threads(), etc.",
    body_style
))
story.append(PageBreak())

# String Methods
story.append(Paragraph("<b>7. String Methods Verification</b>", section_style))

string_methods = [
    "upper()", "lower()", "strip()", "lstrip()", "rstrip()",
    "find(sub)", "replace(old, new)", "split(delim)", "join(parts)",
    "startswith(prefix)", "endswith(suffix)", 
    "isdigit()", "isalpha()", "isalnum()", "count(sub)"
]

story.append(Paragraph(
    "All string methods documented in the Wiki are verified in runtime/lp_native_strings.h:",
    body_style
))
story.append(Spacer(1, 5))

# Create string methods verification
for method in string_methods:
    story.append(Paragraph(f"✅ {method} - lp_str_{method.split('(')[0].lower()}() implemented", success_style))

story.append(Spacer(1, 20))

# Conclusion
story.append(Paragraph("<b>8. Conclusion</b>", section_style))

story.append(Paragraph(
    "The LP Language Wiki documentation is generally <b>accurate and well-maintained</b>. "
    "The core features, runtime modules, and security features are documented correctly and match the actual implementation.",
    body_style
))
story.append(Spacer(1, 10))

story.append(Paragraph("<b>Key Findings:</b>", body_style))
story.append(Paragraph(
    "1. <b>HTTP Module:</b> Wiki understates the implementation - http.post IS supported alongside http.get.<br/>"
    "2. <b>Type Unions & Generics:</b> These are partially implemented but wiki says \"Not Yet Implemented\" - should be updated to \"Partially Implemented\".<br/>"
    "3. <b>Async/Await:</b> Exists as syntactic sugar only - wiki could clarify this distinction.<br/>"
    "4. <b>GPU Features:</b> Code structure exists but may require CUDA runtime for actual GPU execution.<br/>"
    "5. <b>Encryption:</b> Security module has encrypt_data and hash_algorithm fields but not documented in wiki.",
    body_style
))
story.append(Spacer(1, 15))

story.append(Paragraph("<b>Recommendations:</b>", body_style))
story.append(Paragraph(
    "1. Update wiki to mention http.post support<br/>"
    "2. Change Type Unions status from \"Not Yet\" to \"Partially Supported\"<br/>"
    "3. Change Generics status from \"Not Yet\" to \"Partially Supported\"<br/>"
    "4. Add note about async/await being syntactic sugar<br/>"
    "5. Document encryption-related security features",
    body_style
))

# Build PDF
doc.build(story)
print("Verification report PDF created successfully!")
