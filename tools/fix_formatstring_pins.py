#!/usr/bin/env python3
"""修复 FormatString 节点的动态输入引脚 dataType: 9(Any) → 4(String)
同时规范引脚名: arg0~argN → 0~N (与 Format 占位符 {0}{1} 对应)
"""
import json, os, glob

BJSON_DIR = os.path.join(os.path.dirname(__file__), '..', 'data', 'examples', 'wxgame')

for fpath in glob.glob(os.path.join(BJSON_DIR, '*.bjson')):
    with open(fpath, 'r', encoding='utf-8') as f:
        data = json.load(f)

    rt = data['runtime']
    fname = os.path.basename(fpath)
    changed = False

    for node in rt['nodes']:
        if node.get('definitionId') != 'FormatString':
            continue
        for pin in node.get('pins', []):
            if pin.get('kind') == 0 and pin.get('name', '') != 'Format':
                # 动态输入引脚
                if pin.get('dataType') == 9:  # Any
                    pin['dataType'] = 4  # String
                    changed = True
                # 也修正 argN → N
                name = pin.get('name', '')
                if name.startswith('arg'):
                    pin['name'] = name[3:]  # arg0 → 0, arg1 → 1
                    changed = True

    if changed:
        with open(fpath, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        print(f"  {fname}: fixed FormatString pin types")
    else:
        print(f"  {fname}: OK")
