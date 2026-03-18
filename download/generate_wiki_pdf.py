from reportlab.lib.pagesizes import A4
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.lib.units import inch, cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfbase.pdfmetrics import registerFontFamily
import json
import re
import os

# Register fonts
pdfmetrics.registerFont(TTFont('Times New Roman', '/usr/share/fonts/truetype/english/Times-New-Roman.ttf'))
pdfmetrics.registerFont(TTFont('SimHei', '/usr/share/fonts/truetype/chinese/SimHei.ttf'))
registerFontFamily('Times New Roman', normal='Times New Roman', bold='Times New Roman')

# Create styles
styles = getSampleStyleSheet()

# Title style
title_style = ParagraphStyle(
    name='WikiTitle',
    fontName='Times New Roman',
    fontSize=28,
    leading=34,
    alignment=TA_CENTER,
    spaceAfter=20,
    textColor=colors.HexColor('#1F4E79')
)

# Subtitle style
subtitle_style = ParagraphStyle(
    name='WikiSubtitle',
    fontName='Times New Roman',
    fontSize=14,
    leading=18,
    alignment=TA_CENTER,
    spaceAfter=30,
    textColor=colors.HexColor('#666666')
)

# Section header style
section_style = ParagraphStyle(
    name='SectionHeader',
    fontName='Times New Roman',
    fontSize=16,
    leading=22,
    spaceBefore=18,
    spaceAfter=12,
    textColor=colors.HexColor('#1F4E79')
)

# Body style
body_style = ParagraphStyle(
    name='BodyText',
    fontName='Times New Roman',
    fontSize=10,
    leading=14,
    alignment=TA_LEFT,
    spaceAfter=8
)

# Code style
code_style = ParagraphStyle(
    name='CodeText',
    fontName='Times New Roman',
    fontSize=9,
    leading=12,
    leftIndent=20,
    backColor=colors.HexColor('#F5F5F5'),
    spaceAfter=10
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
    alignment=TA_LEFT
)

def html_to_text(html):
    """Convert HTML to clean text"""
    html = re.sub(r'<script[^>]*>.*?</script>', '', html, flags=re.DOTALL)
    html = re.sub(r'<style[^>]*>.*?</style>', '', html, flags=re.DOTALL)
    html = re.sub(r'<br\s*/?>', '\n', html)
    html = re.sub(r'</p>', '\n\n', html)
    html = re.sub(r'</h[1-6]>', '\n\n', html)
    html = re.sub(r'<li[^>]*>', '• ', html)
    html = re.sub(r'</li>', '\n', html)
    html = re.sub(r'<[^>]+>', '', html)
    html = html.replace('&amp;', '&')
    html = html.replace('&lt;', '<')
    html = html.replace('&gt;', '>')
    html = html.replace('&quot;', '"')
    html = html.replace('&#39;', "'")
    html = html.replace('&nbsp;', ' ')
    html = re.sub(r'\n\s*\n', '\n\n', html)
    html = re.sub(r' +', ' ', html)
    return html.strip()

def extract_wiki_content(html):
    """Extract wiki content from HTML"""
    pattern = r'<div[^>]*class="[^"]*markdown-body[^"]*"[^>]*>(.*?)</div>\s*</div>'
    matches = re.findall(pattern, html, re.DOTALL)
    if matches:
        return html_to_text(matches[0])
    return ""

# Create PDF
doc = SimpleDocTemplate(
    "/home/z/my-project/download/LP_Wiki_Documentation.pdf",
    pagesize=A4,
    title="LP Language Wiki Documentation",
    author="Z.ai",
    creator="Z.ai",
    subject="Complete documentation for LP programming language"
)

story = []

# Cover page
story.append(Spacer(1, 120))
story.append(Paragraph("LP Language Wiki", title_style))
story.append(Paragraph("Complete Documentation", subtitle_style))
story.append(Spacer(1, 40))
story.append(Paragraph("Repository: https://github.com/huyhapitk12/LP", subtitle_style))
story.append(Spacer(1, 20))
story.append(Paragraph("Version: 0.3.0 | Last Updated: March 2025", subtitle_style))
story.append(PageBreak())

# Table of Contents
story.append(Paragraph("Table of Contents", section_style))
story.append(Spacer(1, 10))

toc_items = [
    "1. Home - Wiki Overview",
    "2. Features - Language Features Overview",
    "3. Feature Status - Implementation Status",
    "4. Installation and Setup",
    "5. Language Basics",
    "6. First Programs",
    "7. Expressions and Collections",
    "8. Language Reference",
    "9. Object-Oriented Programming",
    "10. Error Handling",
    "11. Concurrency and Parallelism",
    "12. Parallel Computing (GPU)",
    "13. Security Explained",
    "14. Security Module Reference",
    "15. Calling Security Functions",
    "16. Runtime Modules",
    "17. CLI and Tooling",
    "18. Build and Distribution",
    "19. C API Export",
    "20. Examples Cookbook",
    "21. Quick Reference",
    "22. Troubleshooting",
    "23. Vietnamese Guide"
]

for item in toc_items:
    story.append(Paragraph(item, body_style))

story.append(PageBreak())

# Process all wiki pages
pages_info = [
    ("Home", "Wiki overview and navigation"),
    ("Features", "Complete features overview"),
    ("Feature-Status", "Implementation status"),
    ("Installation-and-Setup", "Installation guide"),
    ("Language-Basics", "Core language concepts"),
    ("First-Programs", "Getting started"),
    ("Expressions-and-Collections", "Data structures"),
    ("Language-Reference", "Syntax reference"),
    ("Object-Oriented-Programming", "Classes and inheritance"),
    ("Error-Handling", "Exception handling"),
    ("Concurrency-and-Parallelism", "Threading"),
    ("Parallel-Computing", "GPU computing"),
    ("Security-Explained", "Security concepts"),
    ("Security-Module-Reference", "Security API"),
    ("Calling-Security-Functions", "Security examples"),
    ("Runtime-Modules", "Built-in modules"),
    ("CLI-and-Tooling", "Command line tools"),
    ("Build-and-Distribution", "Build system"),
    ("C-API-Export", "C interoperability"),
    ("Examples-Cookbook", "Code examples"),
    ("Quick-Reference", "Cheat sheet"),
    ("Troubleshooting", "Common issues"),
    ("Vietnamese-Guide", "Vietnamese documentation")
]

for page_name, description in pages_info:
    filename = f"wiki_pages/{page_name}.json"
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            data = json.load(f)
        
        html = data.get('data', {}).get('html', '')
        title = data.get('data', {}).get('title', page_name)
        
        content = extract_wiki_content(html)
        
        # Add section header
        story.append(Paragraph(f"<b>{title}</b>", section_style))
        story.append(Paragraph(f"<i>{description}</i>", body_style))
        story.append(Spacer(1, 10))
        
        # Process content
        lines = content.split('\n')
        for line in lines:
            line = line.strip()
            if line:
                # Handle different line types
                if line.startswith('•'):
                    story.append(Paragraph(line, body_style))
                elif line.startswith('#') or line.startswith('='):
                    continue  # Skip markdown headers and separators
                elif '```' in line or line.startswith('def ') or line.startswith('class '):
                    story.append(Paragraph(line, code_style))
                elif len(line) > 200:
                    # Long lines - split into smaller paragraphs
                    chunks = [line[i:i+200] for i in range(0, len(line), 200)]
                    for chunk in chunks:
                        story.append(Paragraph(chunk, body_style))
                else:
                    story.append(Paragraph(line, body_style))
        
        story.append(Spacer(1, 20))

# Build PDF
doc.build(story)
print("PDF created successfully!")
