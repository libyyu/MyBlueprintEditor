#!/usr/bin/env python3
"""转换 from/to 格式的 links 为 startPinId/endPinId 格式"""
import json, os, glob

BJSON_DIR = os.path.join(os.path.dirname(__file__), '..', 'data', 'examples', 'wxgame')

for fpath in glob.glob(os.path.join(BJSON_DIR, '*.bjson')):
    with open(fpath, 'r', encoding='utf-8') as f:
        data = json.load(f)

    rt = data['runtime']
    nodes = rt['nodes']
    links = rt['links']
    fname = os.path.basename(fpath)

    # Check if any link uses from/to format
    has_new_format = any('from' in l for l in links)
    if not has_new_format:
        continue

    # Build pin lookup: (nodeId, pinName, kind) -> pinId
    pin_map = {}
    for n in nodes:
        for p in n.get('pins', []):
            name = p.get('name', '')
            is_output = p.get('kind', 0) == 1
            pin_map[(n['id'], name, is_output)] = p['id']

    new_links = []
    for i, link in enumerate(links):
        if 'startPinId' in link:
            new_links.append(link)
            continue

        fr = link.get('from', {})
        to = link.get('to', {})
        from_node = fr.get('nodeId')
        from_pin = fr.get('pin', '')
        to_node = to.get('nodeId')
        to_pin = to.get('pin', '')

        start_id = pin_map.get((from_node, from_pin, True))
        end_id = pin_map.get((to_node, to_pin, False))

        if start_id and end_id:
            new_links.append({
                'id': len(new_links) + 1,
                'startPinId': start_id,
                'endPinId': end_id,
                'isEnabled': True
            })
        else:
            print(f"  {fname} SKIP: ({from_node}).{from_pin} -> ({to_node}).{to_pin}")

    rt['links'] = new_links
    with open(fpath, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
    print(f"  {fname}: converted {len(new_links)} links to pinId format")
