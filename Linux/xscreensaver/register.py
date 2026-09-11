#!/usr/bin/env python3
"""Add/remove Matrix Reflow without replacing other XScreenSaver preferences."""
import argparse
import os
from pathlib import Path
import re
import shlex
import tempfile


def update(text, command, remove=False):
    lines = text.splitlines(keepends=True)
    start = next((i for i, line in enumerate(lines) if re.match(r'^programs\s*:', line)), None)
    if start is None:
        if remove:
            return text
        raise ValueError('No programs resource: first run xscreensaver-settings to create the configuration')
    end = start + 1
    while lines[end - 1].rstrip('\n').endswith('\\') and end < len(lines):
        end += 1
    body = ''.join(lines[start:end]).split(':', 1)[1].replace('\\\n', '')
    entries = [entry.strip() for entry in body.split(r'\n') if entry.strip()]
    def ours(entry):
        return re.search(r'''(?<![\w-])matrix-reflow(?=[\s"']|$)''', entry) is not None
    existing = any(ours(entry) for entry in entries)
    if not remove and existing:
        return text
    if remove:
        if not existing:
            return text
        selected = next((i for i, line in enumerate(lines) if re.match(r'^selected\s*:', line)), None)
        if selected is not None:
            old_index = int(lines[selected].split(':', 1)[1].strip())
            remaining = [i for i, entry in enumerate(entries) if not ours(entry)]
            new_index = remaining.index(old_index) if old_index in remaining else 0
            lines[selected] = re.sub(r'(:\s*)-?\d+', lambda m: m[1] + str(new_index), lines[selected])
        entries = [entry for entry in entries if not ours(entry)]
    else:
        entries.append('GL: "Matrix Reflow" ' + shlex.quote(command) + ' --root')
    replacement = 'programs: \\\n' + ''.join('  ' + entry + r'\n' + '\\\n' for entry in entries) + '\n'
    return ''.join(lines[:start]) + replacement + ''.join(lines[end:])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=Path.home() / '.xscreensaver')
    parser.add_argument('--command', default='/usr/local/libexec/xscreensaver/matrix-reflow')
    parser.add_argument('--remove', action='store_true')
    args = parser.parse_args()
    original = args.config.read_text()
    result = update(original, args.command, args.remove)
    if result == original:
        print('No change needed')
        return
    backup = args.config.with_name(args.config.name + '.before-matrix-reflow')
    if not backup.exists():
        backup.write_text(original)
        backup.chmod(args.config.stat().st_mode & 0o777)
    fd, temporary = tempfile.mkstemp(prefix='.matrix-reflow-', dir=args.config.parent)
    try:
        with os.fdopen(fd, 'w') as out:
            out.write(result)
        os.chmod(temporary, args.config.stat().st_mode & 0o777)
        if args.config.read_text() != original:
            raise RuntimeError('Configuration changed during update; retry')
        os.replace(temporary, args.config)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    print(('Removed' if args.remove else 'Registered') + ' Matrix Reflow; other preferences preserved')


if __name__ == '__main__':
    main()
