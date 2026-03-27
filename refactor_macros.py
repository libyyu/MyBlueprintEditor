#!/usr/bin/env python3
"""
Refactor script: replace #define macro aliases with direct ActiveDoc()->xxx access.
Processes all .cpp/.h files in BlueprintEditor/, then removes the #define block from BlueprintEditor.h
"""

import re
import os

BASE = "/root/.openclaw/workspace/MyBlueprintEditor/BlueprintEditor"

# Files to process for macro replacement
FILES = [
    "EditorUI.cpp",
    "FileOperations.cpp",
    "BlueprintEditor.cpp",
    "ClipboardOps.cpp",
    "SearchOverlay.cpp",
    "ExecutionPanel.cpp",
    "Minimap.cpp",
    "BlueprintEditor.h",
]

# Ordered from longest to shortest to avoid partial matches
# (e.g., m_NeedNavigateToContent before m_NeedSetNodePositions before m_Nodes)
REPLACEMENTS = [
    ("m_NeedNavigateToContent",  "ActiveDoc()->needNavigateToContent"),
    ("m_NeedSetNodePositions",   "ActiveDoc()->needSetNodePositions"),
    ("m_PendingContentBounds",   "ActiveDoc()->pendingContentBounds"),
    ("m_LastExecutionStatus",    "ActiveDoc()->lastExecutionStatus"),
    ("m_PersistentRunner",       "ActiveDoc()->persistentRunner"),
    ("m_ExecutionLogDirty",      "ActiveDoc()->executionLogDirty"),
    ("m_ExecutionLogText",       "ActiveDoc()->executionLogText"),
    ("m_CurrentFilePath",        "ActiveDoc()->filePath"),
    ("m_PendingLoadData",        "ActiveDoc()->pendingLoadData"),
    ("m_NodeTouchTime",          "ActiveDoc()->nodeTouchTime"),
    ("m_ExecutionLog",           "ActiveDoc()->executionLog"),
    ("m_IsExecuting",            "ActiveDoc()->isExecuting"),
    ("m_FlowLinks",              "ActiveDoc()->flowLinks"),
    ("m_IsDirty",                "ActiveDoc()->isDirty"),
    ("m_NextId",                 "ActiveDoc()->nextId"),
    ("m_Links",                  "ActiveDoc()->links"),
    ("m_Nodes",                  "ActiveDoc()->nodes"),
]

def replace_with_word_boundary(content, macro, replacement):
    """Replace macro using word boundary (not followed by word chars)."""
    pattern = re.compile(r'\b' + re.escape(macro) + r'\b')
    return pattern.sub(replacement, content)

total_replacements = {}

for fname in FILES:
    fpath = os.path.join(BASE, fname)
    if not os.path.exists(fpath):
        print(f"  SKIP (not found): {fname}")
        continue

    with open(fpath, 'r', encoding='utf-8') as f:
        original = f.read()

    content = original
    file_counts = {}

    for macro, replacement in REPLACEMENTS:
        pattern = re.compile(r'\b' + re.escape(macro) + r'\b')
        count = len(pattern.findall(content))
        if count > 0:
            content = pattern.sub(replacement, content)
            file_counts[macro] = count

    if content != original:
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"  MODIFIED: {fname}")
        for macro, count in file_counts.items():
            print(f"    {macro} -> replaced {count} occurrence(s)")
        total_replacements[fname] = file_counts
    else:
        print(f"  UNCHANGED: {fname}")

print("\nAll files processed.")

# Now remove the #define block from BlueprintEditor.h
header_path = os.path.join(BASE, "BlueprintEditor.h")
with open(header_path, 'r', encoding='utf-8') as f:
    header = f.read()

# Pattern: from the comment line through the last #define, ending just before `};`
# We want to remove from the comment block to the last #define line (inclusive),
# but keep `};`
define_block_pattern = re.compile(
    r'\n    // ---- 以下为兼容性别名，代理到 ActiveDoc\(\) ----\n'
    r'    // 让旧代码中 m_Nodes / m_Links 等访问透明转发（仅在有活跃文档时有效）\n'
    r'(?:    #define [^\n]*\n)+'
)

match = define_block_pattern.search(header)
if match:
    new_header = header[:match.start()] + "\n" + header[match.end():]
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write(new_header)
    print(f"\nRemoved #define block from BlueprintEditor.h")
    print(f"  Removed span: lines at positions {match.start()}..{match.end()}")
else:
    print("\nWARNING: Could not find #define block pattern in BlueprintEditor.h!")
    print("Please check manually.")
