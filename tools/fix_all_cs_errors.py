#!/usr/bin/env python3
"""一次性修复所有 MiniGame C# 编译错误"""
import os, glob, re

MINIGAME_DIR = os.path.join(os.path.dirname(__file__), '..', 'Unity', 'Samples', 'MiniGame')

USING_AINPC = 'using BlueprintRuntime.Samples.AINpc;'
USING_AINPC_OW = 'using BlueprintRuntime.Samples.AINpc.OpenWorld;'
USING_QUEST = 'using BlueprintRuntime.Samples.MiniGame.Quest;'
USING_CHATAPP = 'using BlueprintRuntime.Samples.MiniGame.ChatApp;'
USING_WECHAT = 'using BlueprintRuntime.Samples.MiniGame.WeChat;'

AINPC_TYPES = ['AINpcController', 'AINpcStreamingController', 'EmotionalNpcController', 'BlueprintService']
OW_TYPES = ['NpcProximityTrigger', 'WorldSpeechBubble', 'InteractionPrompt', 'OpenWorldDialogPanel']
QUEST_TYPES = ['QuestSystem']
WECHAT_TYPES = ['WeChatSDK', 'ShareViralSystem']

def add_using(content, using_line):
    """在最后一个 using 行后面插入"""
    if using_line in content:
        return content, False
    lines = content.split('\n')
    last_using = -1
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('using ') and stripped.endswith(';'):
            last_using = i
    if last_using >= 0:
        lines.insert(last_using + 1, using_line)
        return '\n'.join(lines), True
    return content, False

total_changes = 0

for fpath in sorted(glob.glob(os.path.join(MINIGAME_DIR, '**', '*.cs'), recursive=True)):
    with open(fpath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    fname = os.path.basename(fpath)
    changed = False
    
    # 1. Add missing using directives
    needs_ainpc = any(t in content for t in AINPC_TYPES)
    needs_ow = any(t in content for t in OW_TYPES)
    needs_quest = 'QuestSystem' in content and 'namespace BlueprintRuntime.Samples.MiniGame.Quest' not in content
    needs_wechat = any(t in content for t in WECHAT_TYPES) and 'namespace BlueprintRuntime.Samples.MiniGame.WeChat' not in content
    
    if needs_ainpc:
        content, c = add_using(content, USING_AINPC)
        if c: changed = True; print(f"  {fname}: +{USING_AINPC}")
    if needs_ow:
        content, c = add_using(content, USING_AINPC_OW)
        if c: changed = True; print(f"  {fname}: +{USING_AINPC_OW}")
    if needs_quest:
        content, c = add_using(content, USING_QUEST)
        if c: changed = True; print(f"  {fname}: +{USING_QUEST}")
    if needs_wechat:
        content, c = add_using(content, USING_WECHAT)
        if c: changed = True; print(f"  {fname}: +{USING_WECHAT}")
    
    # 2. Fix long -> int: ctx.GetInputInt returns long, cast to int
    # Pattern: int xxx = ctx.GetInputInt("...") → int xxx = (int)ctx.GetInputInt("...")
    old_content = content
    content = re.sub(
        r'(int\s+\w+\s*=\s*)ctx\.GetInputInt\(',
        r'\1(int)ctx.GetInputInt(',
        content
    )
    if content != old_content:
        changed = True
        print(f"  {fname}: fixed long->int cast for GetInputInt")
    
    # 3. Fix PlayerStats.Gold -> PlayerStats.Instance properties
    # PlayerStats stores data in Data.gold/Data.level/Data.exp/Data.playerName
    # But we added public properties Gold/Level/Exp/PlayerName in the class
    # Check if the properties exist
    
    # 4. Fix QuestDefinition -> Quest.QuestSystem.QuestData  
    if 'QuestDefinition' in content:
        content = content.replace('QuestDefinition', 'QuestSystem.QuestData')
        changed = True
        print(f"  {fname}: QuestDefinition -> QuestSystem.QuestData")
    
    if changed:
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(content)
        total_changes += 1

print(f"\nTotal files modified: {total_changes}")
