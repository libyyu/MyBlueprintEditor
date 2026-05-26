#!/usr/bin/env python3
"""修复 MakeMessage.Message -> FormatString 的连线
原来错误连到了 Result(output), 应该连到 0(input)
"""
import json, os, glob

BJSON_DIR = os.path.join(os.path.dirname(__file__), '..', 'data', 'examples', 'wxgame')

for fpath in glob.glob(os.path.join(BJSON_DIR, '*.bjson')):
    with open(fpath, 'r', encoding='utf-8') as f:
        data = json.load(f)

    rt = data['runtime']
    nodes = {n['id']: n for n in rt['nodes']}
    links = rt['links']
    fname = os.path.basename(fpath)
    changed = False

    # Find the FormatString node (Wrap Messages Array)
    fmt_node = None
    for n in rt['nodes']:
        if n['definitionId'] == 'FormatString' and 'Messages' in n.get('name', ''):
            fmt_node = n
            break

    if fmt_node is None:
        print(f"  {fname}: no FormatString 'Wrap Messages Array' found, skip")
        continue

    # Find pin ids
    fmt_pins = {p.get('name', ''): p['id'] for p in fmt_node.get('pins', [])}
    input_0_id = fmt_pins.get('0')
    result_id = fmt_pins.get('Result')

    if input_0_id is None or result_id is None:
        print(f"  {fname}: FormatString pins incomplete, skip")
        continue

    # Find MakeMessage output pin id
    make_msg_node = None
    for n in rt['nodes']:
        if n['definitionId'] == 'JSON.MakeMessage':
            make_msg_node = n
            break

    if make_msg_node is None:
        print(f"  {fname}: no JSON.MakeMessage found, skip")
        continue

    msg_output_id = None
    for p in make_msg_node.get('pins', []):
        if p.get('name') == 'Message' and p.get('kind') == 1:
            msg_output_id = p['id']
            break

    if msg_output_id is None:
        print(f"  {fname}: MakeMessage.Message pin not found, skip")
        continue

    # Fix: any link from msg_output_id to result_id should go to input_0_id
    for link in links:
        if link.get('startPinId') == msg_output_id and link.get('endPinId') == result_id:
            link['endPinId'] = input_0_id
            changed = True
            print(f"  {fname}: fixed link {link['id']}: MakeMessage.Message({msg_output_id}) -> FormatString.0({input_0_id})")

    # Also check: is there already a correct link from result_id -> LLM.Chat.Messages?
    has_result_link = any(l.get('startPinId') == result_id for l in links)
    if not has_result_link:
        # Find LLM.Chat Messages input pin
        for n in rt['nodes']:
            if n['definitionId'] in ('LLM.Chat', 'LLM.StreamChat'):
                for p in n.get('pins', []):
                    if p.get('name') == 'Messages' and p.get('kind') == 0:
                        new_link_id = max(l['id'] for l in links) + 1
                        links.append({
                            'id': new_link_id,
                            'startPinId': result_id,
                            'endPinId': p['id'],
                            'isEnabled': True
                        })
                        changed = True
                        print(f"  {fname}: added missing link: FormatString.Result({result_id}) -> LLM.Messages({p['id']})")
                        break

    if changed:
        with open(fpath, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    else:
        print(f"  {fname}: links already correct")
