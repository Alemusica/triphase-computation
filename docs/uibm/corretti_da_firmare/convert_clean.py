#!/usr/bin/env python3
"""
Convert markdown patent files to clean plain text format.
Removes all markdown syntax while preserving content structure.
"""

import re
from pathlib import Path

def convert_table(match):
    """Convert a markdown table to plain text list format."""
    lines = match.group(0).strip().split('\n')
    # Parse header row
    headers = [h.strip() for h in lines[0].strip('|').split('|')]
    # Skip separator row (line 1), process data rows
    rows = []
    for line in lines[2:]:
        cells = [c.strip() for c in line.strip('|').split('|')]
        rows.append(cells)
    # Format as list entries
    result = []
    for row in rows:
        parts = []
        for h, c in zip(headers, row):
            if c:
                parts.append(f"{h}: {c}")
        result.append(' — '.join(parts))
    return '\n'.join(result)


def clean_markdown(text):
    """Remove all markdown formatting from text."""

    # Convert markdown tables to plain text before other processing
    text = re.sub(
        r'(?:^\|.+\|$\n){2,}',
        convert_table,
        text,
        flags=re.MULTILINE
    )

    # Remove markdown headers (###, ####, etc.) - keep just the text
    text = re.sub(r'^#{1,6}\s+', '', text, flags=re.MULTILINE)

    # Remove bold/italic markers (but NOT multiplication asterisks like f_1*t)
    text = re.sub(r'\*\*(.*?)\*\*', r'\1', text)
    text = re.sub(r'(?<!\w)\*(.*?)\*(?!\w)', r'\1', text)

    # Remove code blocks
    text = re.sub(r'```.*?```', '', text, flags=re.DOTALL)
    text = re.sub(r'`(.*?)`', r'\1', text)

    # Remove horizontal rules
    text = re.sub(r'^---+\s*$', '', text, flags=re.MULTILINE)

    # Convert bullet lists - keep the content but clean up
    text = re.sub(r'^\s*[-\*]\s+', '', text, flags=re.MULTILINE)

    # Remove multiple blank lines
    text = re.sub(r'\n{3,}', '\n\n', text)

    # Trim whitespace
    text = text.strip()

    return text

def main():
    source_dir = Path('../source')
    output_dir = Path('.')

    files_to_convert = [
        ('descrizione_it.md', 'descrizione_clean.txt'),
        ('riassunto_it.md', 'riassunto_clean.txt'),
        ('rivendicazioni_it.md', 'rivendicazioni_clean.txt'),
        ('rivendicazioni.md', 'rivendicazioni_en_clean.txt'),
    ]

    for source_name, output_name in files_to_convert:
        source_file = source_dir / source_name
        if not source_file.exists():
            print(f"Skipping {source_name} - not found")
            continue

        # Read source
        with open(source_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Clean markdown
        clean_content = clean_markdown(content)

        # Write output
        output_file = output_dir / output_name
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(clean_content)

        print(f"  {source_name} -> {output_name}")

if __name__ == '__main__':
    main()
