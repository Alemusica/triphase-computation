#!/usr/bin/env python3
"""Create PDFs from text files using reportlab."""

from reportlab.lib.pagesizes import A4
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import cm
from reportlab.lib.enums import TA_JUSTIFY
from pathlib import Path

def create_pdf(text_file, pdf_file):
    with open(text_file, 'r', encoding='utf-8') as f:
        content = f.read()

    doc = SimpleDocTemplate(str(pdf_file), pagesize=A4,
                           rightMargin=2*cm, leftMargin=2*cm,
                           topMargin=2*cm, bottomMargin=2*cm)

    styles = getSampleStyleSheet()
    style = ParagraphStyle('Custom', parent=styles['Normal'],
                          fontSize=10, leading=14, alignment=TA_JUSTIFY)

    story = []
    for line in content.split('\n'):
        if line.strip():
            line = line.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
            story.append(Paragraph(line, style))
        else:
            story.append(Spacer(1, 0.3*cm))

    doc.build(story)
    print(f"  {pdf_file}")

files = [
    ('descrizione_clean.txt', 'descrizione_FINAL.pdf'),
    ('rivendicazioni_clean.txt', 'rivendicazioni_FINAL.pdf'),
    ('riassunto_clean.txt', 'riassunto_FINAL.pdf'),
    ('rivendicazioni_en_clean.txt', 'rivendicazioni_en_FINAL.pdf'),
]

for txt, pdf in files:
    create_pdf(txt, pdf)
