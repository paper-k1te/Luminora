import os
import re

files = ['localization.h', 'menu_creative.h', 'main.c']

for fname in files:
    path = os.path.join('standalone_src', fname)
    if not os.path.exists(path):
        continue
    with open(path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    for idx, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue
        # Extract quoted strings
        matches = re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', stripped)
        for s in matches:
            has_chinese = bool(re.search(r'[\u4e00-\u9fff]', s))
            has_english = bool(re.search(r'[a-zA-Z]', s))
            # Mixed bilingual clutter pattern: English text with (Chinese translation) or vice-versa
            if has_chinese and has_english and ('(' in s or ')' in s):
                print(f"[MIXED STRING DETECTED] {fname}:{idx+1} -> {s}")

print("[VERIFICATION COMPLETED]")
