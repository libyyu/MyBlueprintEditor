#!/usr/bin/env python3
"""批量修复小游戏蓝图：
1. OnExecute -> OnBeginPlay
2. JSON.MakeMessageArray -> FormatString('[{0}]')
"""
import json, os, glob

BJSON_DIR = os.path.join(os.path.dirname(__file__), '..', 'data', 'examples', 'wxgame')

for fpath in glob.glob(os.path.join(BJSON_DIR, '*.bjson')):
    with open(fpath, 'r', encoding='utf-8') as f:
        data = json.load(f)

    runtime = data.get('runtime', data)
    nodes = runtime.get('nodes', [])
    links = runtime.get('links', [])
    changed = False
    fname = os.path.basename(fpath)

    # Fix 1: OnExecute -> OnBeginPlay
    for node in nodes:
        if node.get('definitionId') == 'OnExecute':
            node['definitionId'] = 'OnBeginPlay'
            node['name'] = 'On Begin Play'
            changed = True
            print(f"  {fname}: OnExecute -> OnBeginPlay (node {node['id']})")

    # Fix 2: JSON.MakeMessageArray -> FormatString('[{0}]')
    for node in nodes:
        if node.get('definitionId') == 'JSON.MakeMessageArray':
            old_id = node['id']
            node['definitionId'] = 'FormatString'
            node['name'] = 'Wrap Messages Array'
            changed = True
            print(f"  {fname}: JSON.MakeMessageArray -> FormatString (node {old_id})")

            # Fix pins (old pinId-based format)
            if 'pins' in node:
                old_pins = node['pins']
                # Keep first input pin id and last output pin id
                in_id = old_pins[0]['id'] if old_pins else old_id * 100 + 1
                out_id = old_pins[-1]['id'] if len(old_pins) > 1 else old_id * 100 + 2
                node['pins'] = [
                    {'id': in_id, 'kind': 0, 'dataType': 4, 'name': 'Format', 'defaultValue': '[{0}]'},
                    {'id': in_id + 1, 'kind': 0, 'dataType': 4, 'name': '0'},
                    {'id': out_id, 'kind': 1, 'dataType': 4, 'name': 'Result'},
                ]
                # Fix links referencing old pin ids
                # Old: input pin "Messages" or "Message 0" -> now pin "0" (in_id+1)
                # Old: output pin "Array" -> now pin "Result" (out_id)
                for link in links:
                    # Old format: startPinId/endPinId
                    if 'startPinId' in link or 'endPinId' in link:
                        # Find which old pins were input/output
                        old_input_ids = {p['id'] for p in old_pins if p.get('kind') == 0}
                        old_output_ids = {p['id'] for p in old_pins if p.get('kind') == 1}
                        
                        if link.get('endPinId') in old_input_ids:
                            link['endPinId'] = in_id + 1  # connect to "0" input
                        if link.get('startPinId') in old_output_ids:
                            link['startPinId'] = out_id  # connect from "Result" output

            # Fix pinValues format
            if 'pinValues' in node or not node.get('pins'):
                node['pinValues'] = {'Format': '[{0}]'}

            # Fix new-format links (from/to with pin names)
            for link in links:
                fr = link.get('from', {})
                to = link.get('to', {})
                if fr.get('nodeId') == old_id and fr.get('pin') in ('Array', 'Messages'):
                    fr['pin'] = 'Result'
                    print(f"    link fix: output -> Result")
                if to.get('nodeId') == old_id and to.get('pin') in ('Message 0', 'Messages', 'Message'):
                    to['pin'] = '0'
                    print(f"    link fix: input -> 0")

    if changed:
        with open(fpath, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    else:
        print(f"  {fname}: no changes needed")
