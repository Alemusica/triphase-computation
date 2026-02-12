#!/usr/bin/env python3
"""
Convert markdown patent files to clean plain text format.
Removes all markdown syntax while preserving content structure.
"""

import re
from pathlib import Path

def clean_markdown(text):
    """Remove all markdown formatting from text."""

    # Remove markdown headers (###, ####, etc.) - keep just the text
    text = re.sub(r'^#{1,6}\s+', '', text, flags=re.MULTILINE)

    # Remove bold/italic markers
    text = re.sub(r'\*\*(.*?)\*\*', r'\1', text)
    text = re.sub(r'\*(.*?)\*', r'\1', text)

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
        'descrizione.md',
        'riassunto.md',
        'rivendicazioni.md'
    ]

    for filename in files_to_convert:
        source_file = source_dir / filename
        if not source_file.exists():
            print(f"Skipping {filename} - not found")
            continue

        # Read source
        with open(source_file, 'r', encoding='utf-8') as f:
            content = f.read()

        # Clean markdown
        clean_content = clean_markdown(content)

        # Write output
        output_file = output_dir / filename.replace('.md', '_clean.txt')
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(clean_content)

        print(f"  {filename} -> {output_file.name}")

if __name__ == '__main__':
    main()
