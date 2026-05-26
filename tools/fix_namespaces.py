#!/usr/bin/env python3
"""给 MiniGame 下引用了 AINpc 类的文件加 using BlueprintRuntime.Samples.AINpc;
   给引用了 OpenWorld 类的文件加 using BlueprintRuntime.Samples.AINpc.OpenWorld;
   修复 QuestDefinition → QuestSystem.QuestData
"""
import os, re, glob

MINIGAME_DIR = os.path.join(os.path.dirname(__file__), '..', 'Unity', 'Samples', 'MiniGame')

AINPC_TYPES = [
    'AINpcController', 'AINpcStreamingController', 'EmotionalNpcController',
    'BlueprintService', 'DialogUI', 'StreamingDialogUI',
]
OPENWORLD_TYPES = [
    'NpcProximityTrigger', 'WorldSpeechBubble', 'InteractionPrompt', 'OpenWorldDialogPanel',
]

USING_AINPC = 'using BlueprintRuntime.Samples.AINpc;'
USING_OPENWORLD = 'using BlueprintRuntime.Samples.AINpc.OpenWorld;'

for fpath in glob.glob(os.path.join(MINIGAME_DIR, '**', '*.cs'), recursive=True):
    with open(fpath, 'r', encoding='utf-8') as f:
        content = f.read()

    fname = os.path.basename(fpath)
    changed = False

    # Check if file references AINpc types
    needs_ainpc = any(t in content for t in AINPC_TYPES)
    needs_openworld = any(t in content for t in OPENWORLD_TYPES)

    if needs_ainpc and USING_AINPC not in content:
        # Insert after last existing 'using' line
        lines = content.split('\n')
        last_using = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('using ') and line.strip().endswith(';'):
                last_using = i
        if last_using >= 0:
            lines.insert(last_using + 1, USING_AINPC)
            content = '\n'.join(lines)
            changed = True
            print(f"  {fname}: added {USING_AINPC}")

    if needs_openworld and USING_OPENWORLD not in content:
        lines = content.split('\n')
        last_using = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('using ') and line.strip().endswith(';'):
                last_using = i
        if last_using >= 0:
            lines.insert(last_using + 1, USING_OPENWORLD)
            content = '\n'.join(lines)
            changed = True
            print(f"  {fname}: added {USING_OPENWORLD}")

    # Fix QuestDefinition -> QuestSystem.QuestData
    if 'QuestDefinition' in content:
        content = content.replace('QuestDefinition', 'Quest.QuestSystem.QuestData')
        changed = True
        print(f"  {fname}: QuestDefinition -> Quest.QuestSystem.QuestData")

    if changed:
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(content)
    # else: no changes needed
