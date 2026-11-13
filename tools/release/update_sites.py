#!/usr/bin/env python3
"""Update both release sites without committing. Python 3, standard library only.

HTML edits use explicit data-release markers, preserving all surrounding source.
--changelog allows a copied changelog for release rehearsals. Protocols come from
this script's repository's stable vX.Y.Z tags (the worktree for an untagged rehearsal).
"""
import argparse
import datetime
import difflib
import hashlib
import html
from html.parser import HTMLParser
import json
import math
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile

REPO = Path(__file__).resolve().parents[2]
SUFFIXES = {'linux': 'linux-x86_64.tar.gz', 'windows': 'windows-x86_64.zip',
            'android': 'android-arm64.apk', 'web': 'web.zip'}
PROTOCOL_PATH = 'src/GalaxyEggbert/Net/NetProtocol.hpp'
MONTHS = ('January February March April May June July August September October November December').split()


def git(*args):
    return subprocess.check_output(['git', '-C', str(REPO), *args], text=True)


def version_tuple(value):
    return tuple(map(int, value.split('.')))


def protocol(source):
    match = re.search(r'\bkProtocolVersion\s*=\s*(\d+)\s*;', source)
    if not match:
        raise ValueError('Missing numeric kProtocolVersion')
    return int(match[1])


def join_words(words):
    return ' and '.join(words) if len(words) <= 2 else ', '.join(words[:-1]) + ' and ' + words[-1]


def network_line(version):
    tags = {tag[1:]: tag for tag in git('tag', '--list', 'v*').splitlines()
            if re.fullmatch(r'v\d+\.\d+\.\d+', tag) and version_tuple(tag[1:]) <= version_tuple(version)}
    current = protocol(git('show', f'{tags[version]}:{PROTOCOL_PATH}') if version in tags
                       else (REPO / PROTOCOL_PATH).read_text())
    compatible, incompatible = [], {}
    for old in sorted(tags, key=version_tuple, reverse=True):
        if old == version:
            continue
        same = protocol(git('show', f'{tags[old]}:{PROTOCOL_PATH}')) == current
        if same:
            compatible.append(old)
        incompatible.setdefault(old.rsplit('.', 1)[0], []).append((old, same))
    older = []
    for minor, releases in incompatible.items():
        bad = [old for old, same in releases if not same]
        # A single release reads better as 0.2.0; several wholly incompatible patches as 0.1.x.
        older.extend([minor + '.x'] if len(bad) > 1 and len(bad) == len(releases) else bad)
    result = f'Version {version} plays with '
    result += join_words(compatible) if compatible else 'no earlier release'
    if older:
        result += ', but not with ' + ' or '.join(older)
    return result


def changelog_entry(path, version, date):
    source = path.read_text(encoding='utf-8')
    match = re.search(r'^## \[' + re.escape(version) + r'\] — (\d{4}-\d{2}-\d{2})\s*\n(.*?)(?=^## |\Z)',
                      source, re.M | re.S)
    if not match or match[1] != date:
        raise ValueError(f'Changelog must contain ## [{version}] — {date}')
    body = match[2]
    headlines = re.findall(r'<!-- site-headline:\s*(.*?)\s*-->', body, re.S)
    if len(headlines) > 1:
        raise ValueError('Use at most one site-headline comment per entry')
    body = re.sub(r'<!--.*?-->', '', body, flags=re.S).strip()
    summary = body.split('\n\n', 1)[0]
    if summary.startswith(('#', '- ')):
        raise ValueError('Changelog entry needs a first summary paragraph')
    summary = ' '.join(summary.split())
    groups, bullets = [], None
    for line in body.splitlines():
        if line.startswith('### '):
            bullets = []
            groups.append((line[4:].strip(), bullets))
        elif line.startswith('- '):
            if bullets is None:
                raise ValueError('Changelog bullet without a ### category')
            bullets.append(line[2:])
        elif line[:1].isspace() and line.strip() and bullets:
            bullets[-1] += ' ' + line.strip()
    return summary, groups, ' '.join(headlines[0].split()) if headlines else None


def inline_markdown(text):
    """The changelog's inline subset; raw HTML is escaped, never injected."""
    tokens = []

    def protect(value):
        tokens.append(value)
        return f'\x00{len(tokens)-1}\x00'

    text = re.sub(r'`([^`]+)`', lambda m: protect('<code>' + html.escape(m[1]) + '</code>'), text)
    text = re.sub(r'\[([^\]]+)\]\((https?://[^\s)]+)\)',
                  lambda m: protect('<a href="' + html.escape(m[2], quote=True) + '">' + html.escape(m[1]) + '</a>'), text)
    text = html.escape(text)
    text = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', text)
    text = re.sub(r'\*([^*]+)\*', r'<em>\1</em>', text)
    return re.sub(r'\x00(\d+)\x00', lambda m: tokens[int(m[1])], text)


class MarkedHTML(HTMLParser):
    """Record source offsets; do not serialize HTML or normalize its whitespace."""
    def __init__(self, source, values):
        super().__init__(convert_charrefs=False)
        self.source, self.values, self.edits, self.stack = source, values, [], []
        self.lines = [0] + [m.end() for m in re.finditer('\n', source)]
        self.feed(source)
        self.close()
        if self.stack:
            raise ValueError('Unclosed release marker')

    def source_offset(self):
        row, col = self.getpos()
        return self.lines[row-1] + col

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        start = self.source_offset()
        raw = self.get_starttag_text()
        for marker, target in [('data-release-href', 'href'), ('data-release-content', 'content')]:
            if marker not in attrs:
                continue
            if target == 'content' and (tag != 'meta' or not (
                    attrs.get('name') == 'description' or attrs.get('property') == 'og:description')):
                raise ValueError('Content templates are only for exact description meta properties')
            matches = list(re.finditer(r'(?<![\w-])' + target + r'="([^"]*)"', raw))
            if len(matches) != 1:
                raise ValueError(f'Missing or duplicate {target} on marked {tag}')
            match = matches[0]
            value = attrs[marker].format(version=self.values['version'])
            self.edits.append((start + match.start(1), start + match.end(1), html.escape(value, quote=True)))
        kind = attrs.get('data-release')
        if 'data-release-size' in attrs:
            kind = 'size-' + attrs['data-release-size']
        # Only marked spans have replaceable contents. Nested markup is not guessed at.
        if kind:
            if tag != 'span':
                raise ValueError('Use a span for data-release contents')
            self.stack.append((tag, start + len(raw), kind))
        elif self.stack and tag == 'span':
            raise ValueError('Do not nest spans inside release markers')

    def handle_endtag(self, tag):
        if tag == 'span' and self.stack:
            _, start, kind = self.stack.pop()
            old = self.source[start:self.source_offset()]
            if kind == 'headline':
                value = ('Version ' + self.values['version'] + ' is out: ' + self.values['headline']
                         if self.values['headline'] is not None else
                         re.sub(r'^Version \d+\.\d+\.\d+ is out:', 'Version ' + self.values['version'] + ' is out:', old))
                if self.values['headline'] is not None:
                    value = html.escape(value)
            else:
                if kind not in self.values:
                    raise ValueError(f'Unknown release marker: {kind}')
                value = self.values[kind]
            # Keep the FAQ's hand-wrapped network wording when its text already agrees.
            if kind == 'network' and ' '.join(old.split()) == value:
                value = old
            self.edits.append((start, self.source_offset(), value))

    def result(self):
        result = self.source
        for start, end, value in sorted(self.edits, reverse=True):
            result = result[:start] + value + result[end:]
        return result


def replace_once(source, needle, value):
    if source.count(needle) != 1:
        raise ValueError(f'Expected exactly one insertion marker: {needle}')
    return source.replace(needle, value)


def update_json_ld(source, version):
    def script(match):
        body = match[2]
        data = json.loads(body)
        if data.get('@type') != 'VideoGame':
            return match[0]
        for key, value in [('softwareVersion', version),
                           ('downloadUrl', f'https://github.com/openeggbert/galaxy-eggbert/releases/tag/v{version}')]:
            pattern = r'("' + key + r'"\s*:\s*)"[^"\n]*"'
            body, count = re.subn(pattern, lambda m: m[1] + json.dumps(value), body)
            if count != 1:
                raise ValueError(f'Expected exact JSON-LD key {key}')
        return match[1] + body + match[3]
    return re.sub(r'(<script type="application/ld\+json">)(.*?)(</script>)', script, source, flags=re.S)


def search_index(source, version, summary):
    def entry(raw):
        obj = json.loads(raw)
        old = obj['releaseCurrent']
        keywords = obj['k']
        prefix = f'releases galaxy eggbert {old}:'
        if not keywords.startswith(prefix):
            raise ValueError('Unexpected Releases search keyword prefix')
        keywords = f'releases galaxy eggbert {version}:' + keywords[len(prefix):]
        if version not in obj['releaseVersions']:
            keywords += ' new in ' + version + ' ' + summary.lower()
            obj['releaseVersions'].append(version)
        result = raw
        for key, value in [('k', keywords), ('releaseCurrent', version), ('releaseVersions', obj['releaseVersions'])]:
            result = re.sub(r'("' + key + r'"\s*:\s*)("(?:\\.|[^"\\])*"|\[[^\]]*\])',
                            lambda m: m[1] + json.dumps(value, ensure_ascii=False), result)
        return result
    # JSON objects inside the explicitly delimited generated index, not arbitrary JavaScript.
    start = source.index('/* BEGIN SEARCH INDEX')
    end = source.index('/* END SEARCH INDEX', start)
    index = source[start:end]
    position = index.index('[', index.index('const pages')) + 1
    decoder = json.JSONDecoder()
    entries = []
    while True:
        position += len(index[position:]) - len(index[position:].lstrip())
        if index[position] == ']':
            break
        obj, stop = decoder.raw_decode(index, position)
        if obj.get('u') == 'releases.html':
            entries.append((position, stop))
        position = stop
        position += len(index[position:]) - len(index[position:].lstrip())
        if index[position] == ',':
            position += 1
        elif index[position] != ']':
            raise ValueError('Expected comma or end of search index')
    if len(entries) != 1:
        raise ValueError('Expected exactly one Releases search entry')
    first, last = entries[0]
    index = index[:first] + entry(index[first:last]) + index[last:]
    return source[:start] + index + source[end:]


def text_updates(args, values, summary, groups):
    updates = {}
    for root in [args.web_dir, args.site]:
        paths = sorted(root.rglob('*.html')) if root == args.web_dir else [root/'index.html', root/'play/index.html']
        for path in paths:
            old = path.read_text(encoding='utf-8')
            new = MarkedHTML(old, values).result()
            if path == args.site/'index.html':
                new = update_json_ld(new, args.version)
            updates[path] = (old, new)
    path = args.web_dir/'releases.html'
    old, new = updates[path]
    if f'data-release-entry="{args.version}"' not in new:
        section = f'  <h2 data-release-entry="{args.version}">New in {args.version}</h2>\n'
        section += '  <p>' + inline_markdown(summary) + '</p>\n  <ul>\n'
        for category, bullets in groups:
            for bullet in bullets:
                section += '    <li><strong>' + html.escape(category) + ':</strong> ' + inline_markdown(bullet) + '</li>\n'
        section += '  </ul>\n\n'
        new = replace_once(new, '<!-- release-sections -->\n', '<!-- release-sections -->\n' + section)
    updates[path] = old, new
    path = args.web_dir/'overview.html'
    old, new = updates[path]
    if f'data-release-history="{args.version}"' not in new:
        short = summary
        if len(short) > 180:
            short = re.split(r'(?<=[.!?])\s+', short, maxsplit=1)[0]
        row = f'    <tr data-release-history="{args.version}"><td>{args.date}</td><td>Release {args.version}: {inline_markdown(short)}</td></tr>\n'
        new = replace_once(new, '    <!-- release-history -->', row + '    <!-- release-history -->')
    updates[path] = old, new
    path = args.web_dir/'assets/script.js'
    old = path.read_text(encoding='utf-8')
    updates[path] = old, search_index(old, args.version, summary)
    return updates


def digest(stream):
    result = hashlib.sha256()
    for chunk in iter(lambda: stream.read(1024 * 1024), b''):
        result.update(chunk)
    return result.digest()


def web_members(archive, version):
    prefix = f'GalaxyEggbert-{version}-web/'
    members = {}
    for item in archive.infolist():
        path = PurePosixPath(item.filename)
        if path.is_absolute() or '..' in path.parts or '\\' in item.filename:
            raise ValueError('Unsafe web zip member: ' + item.filename)
        if item.is_dir():
            continue
        if not item.filename.startswith(prefix):
            raise ValueError('Unexpected web zip root: ' + item.filename)
        relative = item.filename[len(prefix):]
        if relative in ['GalaxyEggbert.' + ext for ext in ('html', 'js', 'wasm', 'data')] or relative.startswith('Content/music/'):
            if relative in members or not item.file_size or (item.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError('Duplicate, empty or symlink web member: ' + relative)
            members[relative] = item
    required = {'GalaxyEggbert.' + ext for ext in ('html', 'js', 'wasm', 'data')}
    if not required <= members.keys() or not any(p.startswith('Content/music/') for p in members):
        raise ValueError('Web zip needs HTML, JS, WASM, data and Content/music')
    return members


def update_web_build(args, package):
    dest = args.site/'play/game'
    with zipfile.ZipFile(package) as archive:
        members = web_members(archive, args.version)
        changed = []
        for relative, item in members.items():
            target = dest/relative
            with archive.open(item) as incoming:
                new_hash = digest(incoming)
            if target.is_file():
                with target.open('rb') as existing:
                    if digest(existing) == new_hash:
                        continue
            changed.append(relative)
        removed = [p.relative_to(dest).as_posix() for p in (dest/'Content/music').rglob('*')
                   if p.is_file() and p.relative_to(dest).as_posix() not in members]
        if not changed and not removed:
            return False
        for relative in changed:
            print(f'Web build: {dest/relative} (content changed)')
        for relative in removed:
            print(f'Web build: {dest/relative} (removed)')
        if not args.dry_run:
            # Keep rehearsal temporary files within its package directory, then remove them.
            with tempfile.TemporaryDirectory(prefix='.site-web-', dir=args.packages) as temp:
                for relative, item in members.items():
                    target = Path(temp)/relative
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with archive.open(item) as incoming, target.open('wb') as output:
                        shutil.copyfileobj(incoming, output)
                subprocess.run(['bash', str(args.site/'tools/update-web-build.sh'), temp], check=True)
        return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--date', required=True)
    parser.add_argument('--packages', type=Path, required=True)
    parser.add_argument('--site', type=Path, required=True)
    parser.add_argument('--web-dir', type=Path, required=True)
    parser.add_argument('--changelog', type=Path, default=REPO/'CHANGELOG.md')
    parser.add_argument('--dry-run', action='store_true', help='Print diffs; write nothing')
    args = parser.parse_args()
    if not re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', args.version):
        parser.error('--version must be a final X.Y.Z release')
    date = datetime.date.fromisoformat(args.date)
    if date.isoformat() != args.date:
        parser.error('--date must be YYYY-MM-DD')
    args.site, args.web_dir, args.packages = (p.resolve() for p in (args.site, args.web_dir, args.packages))
    summary, groups, headline = changelog_entry(args.changelog, args.version, args.date)
    values = {'version': args.version, 'date': args.date,
              'date-long': f'{date.day}&nbsp;{MONTHS[date.month-1]} {date.year}',
              'headline': headline, 'network': network_line(args.version)}
    packages = {}
    for kind, suffix in SUFFIXES.items():
        package = args.packages/f'GalaxyEggbert-{args.version}-{suffix}'
        size = package.stat().st_size
        if not size:
            raise ValueError('Empty package: ' + str(package))
        packages[kind] = package
        values['size-' + kind] = f'{math.ceil(size / 1048576)}&nbsp;MB'
    updates = text_updates(args, values, summary, groups)
    # Validate the complete zip before touching either site.
    with zipfile.ZipFile(packages['web']) as archive:
        web_members(archive, args.version)
    changed = False
    for path, (old, new) in updates.items():
        if old == new:
            continue
        changed = True
        print(''.join(difflib.unified_diff(old.splitlines(True), new.splitlines(True),
                                        fromfile=str(path), tofile=str(path))), end='')
    game_changed = update_web_build(args, packages['web'])
    if not args.dry_run:
        for path, (old, new) in updates.items():
            if old != new:
                path.write_text(new, encoding='utf-8')
    if not changed and not game_changed:
        print('No changes.')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError, zipfile.BadZipFile) as error:
        sys.exit(f'update_sites: {error}')
