#!/usr/bin/env python3
"""Verify that all ICON_FA_* UTF-8 hex sequences match their claimed Unicode codepoints."""
import re

with open('BlueprintEditor/IconsFontAwesome6.h', 'r') as f:
    content = f.read()

# Match: #define ICON_FA_xxx  "\xef\x80\x82"  // U+f002
pattern = r'#define\s+(ICON_FA_\w+)\s+"((?:\\x[0-9a-fA-F]{2})+)"\s+//\s*U\+([0-9a-fA-F]+)'
matches = re.findall(pattern, content)

print(f"Found {len(matches)} icon definitions to verify.\n")

errors = []
for name, utf8_str, codepoint_hex in matches:
    cp = int(codepoint_hex, 16)
    expected_utf8 = chr(cp).encode('utf-8')
    
    # Parse the actual hex bytes from the string like \xef\x80\x82
    actual_bytes_strs = re.findall(r'\\x([0-9a-fA-F]{2})', utf8_str)
    actual_bytes = bytes(int(b, 16) for b in actual_bytes_strs)
    
    if actual_bytes != expected_utf8:
        expected_hex = ''.join(f'\\x{b:02x}' for b in expected_utf8)
        actual_hex = ''.join(f'\\x{b:02x}' for b in actual_bytes)
        errors.append((name, codepoint_hex, actual_hex, expected_hex))

if errors:
    print(f"ERRORS: {len(errors)} encoding mismatches found:\n")
    for name, cp, actual, expected in errors:
        print(f"  {name} (U+{cp}):")
        print(f"    Current:  \"{actual}\"")
        print(f"    Expected: \"{expected}\"")
        print()
else:
    print("All encodings are CORRECT!")
