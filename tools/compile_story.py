#!/usr/bin/env python3
"""Compile the project's plain-text/link Twee subset; no third-party dependencies.
Header syntax: https://github.com/iftechfoundation/twine-specs/blob/master/twee-3-specification.md
"""
import argparse
import json
from pathlib import Path
import re
import struct
import sys


class StoryError(ValueError):
    pass


def header(text):
    # Strip only unescaped separator whitespace, preserving e.g. 'Gate\\ '.
    match = re.match(r'((?:\\.|[^\[\{\\])*)(.*)$', text.lstrip())
    if not match:
        raise StoryError('invalid passage header')
    encoded, rest = match.groups()
    while encoded.endswith((' ', '\t')):
        slashes = len(encoded[:-1]) - len(encoded[:-1].rstrip('\\'))
        if slashes % 2:
            break
        encoded = encoded[:-1]
    name = re.sub(r'\\(.)', r'\1', encoded)
    tags = []
    if rest.startswith('['):
        tag = re.match(r'\[((?:\\.|[^\]\\])*)\]\s*', rest)
        if not tag:
            raise StoryError('invalid tags')
        tags = re.sub(r'\\(.)', r'\1', tag[1]).split()
        rest = rest[tag.end():]
    if rest:
        if not isinstance(json.loads(rest), dict):
            raise StoryError('header metadata must be an object')
    if not name:
        raise StoryError('empty passage name')
    return name, tags


def compile_text(source):
    passages = {}
    current = None
    for line_no, line in enumerate(source.splitlines(), 1):
        if line.startswith('::'):
            try:
                name, tags = header(line[2:])
            except ValueError as e:
                raise StoryError(f'line {line_no}: {e}') from e
            if name in passages:
                raise StoryError(f'line {line_no}: duplicate passage {name!r}')
            if set(tags) & {'script', 'stylesheet', 'startup', 'header', 'footer', 'debug-header', 'debug-footer'}:
                raise StoryError(f'{name!r}: executable/style tags are unsupported')
            current = []
            passages[name] = (tags, current)
        elif current is not None:
            current.append(line)
        elif line.strip():
            raise StoryError(f'line {line_no}: content before first passage')
    def body(name):
        return '\n'.join(passages[name][1]).rstrip('\n')
    if 'StoryData' not in passages or 'StoryTitle' not in passages:
        raise StoryError('StoryTitle and StoryData are required')
    meta = json.loads(body('StoryData'))
    if not isinstance(meta, dict) or meta.get('format') != 'Harlowe':
        raise StoryError('expected Harlowe StoryData')
    names = sorted(set(passages) - {'StoryTitle', 'StoryData'})
    ids = {name: i for i, name in enumerate(names)}
    start = meta.get('start', 'Start')
    if start not in ids:
        raise StoryError(f'missing start passage {start!r}')
    nodes = []
    for name in names:
        content = body(name)
        choices = []
        prose = []
        in_choices = False
        for line in content.splitlines():
            match = re.fullmatch(r'\s*\[\[(.*?)\]\]\s*', line)
            if match:
                in_choices = True
                link = match[1]
                if '->' in link:
                    label, target = link.rsplit('->', 1)
                elif '<-' in link:
                    target, label = link.split('<-', 1)
                elif '|' in link:
                    label, target = link.split('|', 1)
                else:
                    label = target = link
                if not label.strip() or target not in ids:
                    raise StoryError(f'{name!r}: empty label or missing target {target!r}')
                choices.append({'label': label, 'target': ids[target]})
            elif line.strip() and in_choices:
                raise StoryError(f'{name!r}: put all choices after the prose, one per line')
            elif not in_choices:
                prose.append(line)
        text = '\n'.join(prose).rstrip('\n')
        # Fail explicitly rather than silently executing/printing unsupported Harlowe.
        unsupported = r'\[|\]|\([\w-]+\s*:|\$\w|\b_\w|<[/!A-Za-z]|\*\*|//|~~|\^\^|\{\}|(?m:^\s*(?:\* |#{1,6} ))'
        if re.search(unsupported, text) or any(re.search(r'\[|\]|\$\w|\([\w-]+\s*:', c['label']) for c in choices):
            raise StoryError(f'{name!r}: unsupported markup; use plain text and standalone links')
        nodes.append({'id': ids[name], 'name': name, 'text': text,
                      'tags': passages[name][0], 'choices': choices})
    seen, pending = set(), [ids[start]]
    while pending:
        node = pending.pop()
        if node in seen:
            continue
        seen.add(node)
        pending.extend(c['target'] for c in nodes[node]['choices'])
    warnings = [f'unreachable passage: {n["name"]}' for n in nodes if n['id'] not in seen]
    return {'version': 1, 'title': body('StoryTitle'), 'start': ids[start], 'nodes': nodes}, warnings


def encode(story):
    out = bytearray(b'RKST')
    def number(n):
        out.extend(struct.pack('<I', n))
    def string(s):
        data = s.encode('utf-8')
        number(len(data))
        out.extend(data)
    number(1)
    number(story['start'])
    number(len(story['nodes']))
    string(story['title'])
    for node in story['nodes']:
        string(node['name'])
        string(node['text'])
        number(len(node['choices']))
        for choice in node['choices']:
            string(choice['label'])
            number(choice['target'])
    return bytes(out)


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', nargs='?', type=Path, default=root / 'assets/story/righteous-knight.twee')
    parser.add_argument('--output', type=Path, default=root / 'dist/story.bin')
    parser.add_argument('--inspect', type=Path, help='optional human-readable JSON output')
    args = parser.parse_args()
    try:
        story, warnings = compile_text(args.source.read_text(encoding='utf-8-sig'))
        data = encode(story)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        # Failed validation never overwrites a previously successful build.
        temporary = args.output.with_suffix(args.output.suffix + '.tmp')
        temporary.write_bytes(data)
        temporary.replace(args.output)
        if args.inspect:
            args.inspect.parent.mkdir(parents=True, exist_ok=True)
            args.inspect.write_text(json.dumps(story, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        for warning in warnings:
            print('warning:', warning, file=sys.stderr)
        print(f'{len(story["nodes"])} nodes, {sum(len(n["choices"]) for n in story["nodes"])} choices -> {args.output} ({len(data)} bytes)')
    except (OSError, ValueError) as e:
        parser.exit(1, f'story compile failed: {e}\n')


if __name__ == '__main__':
    main()
