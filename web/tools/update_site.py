#!/usr/bin/env python3
"""Keep the static site's shared parts in step with one site map.

Run from anywhere:  python3 tools/update_site.py [--check]

For every page listed in SITE_ORDER and CLASS_GROUPS it rewrites
  * the sidebar  (<nav id="sidebar"> ... </nav>), with the current page marked active,
  * the previous/next links (<div class="page-nav"> ... </div>),
and it regenerates the search index inside assets/script.js (between the
BEGIN/END SEARCH INDEX markers) from each page's <title>, h1, subtitle and headings.

Finally it checks every relative href/src in every page and reports links whose
target file does not exist, and HTML files that are in no list (redirect stubs,
recognised by their meta refresh, are skipped). Exit status 1 when anything is wrong.

With --check nothing is written: it reports pages whose generated parts are out of
date, missing pages and broken links.
"""
from __future__ import annotations

import html
import json
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Sidebar: (section title, [(page, label), ...]).
SIDEBAR = [
    ("Getting Started", [
        ("index.html", "Home"),
        ("overview.html", "Project Overview"),
        ("getting-started.html", "Getting Started"),
        ("build.html", "Build Guide"),
        ("running.html", "Running the Game"),
        ("platform-support.html", "Platform Support"),
    ]),
    ("Playing", [
        ("gameplay.html", "Gameplay"),
        ("controls.html", "Controls"),
        ("levels.html", "Campaign &amp; Worlds"),
        ("cheats.html", "Cheats &amp; Diagnostics"),
        ("demos.html", "Demos &amp; Recordings"),
        ("galaxy.html", "Your Own Galaxy"),
        ("editor.html", "3D World Editor"),
        ("graphics.html", "3D Effects &amp; Graphics"),
    ]),
    ("Architecture", [
        ("architecture.html", "Architecture"),
        ("project-structure.html", "Project Structure"),
        ("engine-integration.html", "Engine Integration"),
        ("game-loop.html", "Game Loop"),
        ("rendering.html", "Rendering"),
        ("audio.html", "Audio &amp; Music"),
        ("input.html", "Input"),
        ("save-system.html", "Save Data &amp; Resume"),
        ("configuration.html", "Configuration"),
        ("assets.html", "Assets &amp; Tools"),
    ]),
    ("Reference", [
        ("modules/index.html", "Modules"),
        ("classes/index.html", "Classes"),
        ("systems/index.html", "Systems"),
    ]),
    ("Guides", [
        ("tutorials/index.html", "Tutorials"),
        ("internals/index.html", "Internals"),
        ("development/index.html", "Development"),
        ("development/coding-style.html", "Coding Style"),
        ("development/testing.html", "Testing &amp; QA"),
        ("development/debugging.html", "Debugging"),
        ("development/extending-the-game.html", "Extending the Game"),
        ("development/known-issues.html", "Known Issues"),
    ]),
]

# Reading order for the previous/next links: (page, label).
SITE_ORDER = [
    ("index.html", "Home"),
    ("overview.html", "Project Overview"),
    ("getting-started.html", "Getting Started"),
    ("build.html", "Build Guide"),
    ("running.html", "Running the Game"),
    ("platform-support.html", "Platform Support"),
    ("gameplay.html", "Gameplay"),
    ("controls.html", "Controls"),
    ("levels.html", "Campaign &amp; Worlds"),
    ("cheats.html", "Cheats &amp; Diagnostics"),
    ("demos.html", "Demos &amp; Recordings"),
    ("galaxy.html", "Your Own Galaxy"),
    ("editor.html", "3D World Editor"),
    ("graphics.html", "3D Effects &amp; Graphics"),
    ("architecture.html", "Architecture"),
    ("project-structure.html", "Project Structure"),
    ("engine-integration.html", "Engine Integration"),
    ("game-loop.html", "Game Loop"),
    ("rendering.html", "Rendering"),
    ("audio.html", "Audio &amp; Music"),
    ("input.html", "Input"),
    ("save-system.html", "Save Data &amp; Resume"),
    ("configuration.html", "Configuration"),
    ("assets.html", "Assets &amp; Tools"),
    ("modules/index.html", "Modules"),
    ("classes/index.html", "Classes"),
    ("systems/index.html", "Systems"),
    ("tutorials/index.html", "Tutorials"),
    ("tutorials/building.html", "Tutorial: Building"),
    ("tutorials/modifying-levels.html", "Tutorial: Authoring a World"),
    ("tutorials/first-object.html", "Tutorial: A Block or Object Behaviour"),
    ("internals/index.html", "Internals"),
    ("internals/startup.html", "Startup Sequence"),
    ("internals/world-loading.html", "World Loading"),
    ("internals/vwr-format.html", "The .vwr Format"),
    ("internals/object-lifecycle.html", "Object Lifecycle"),
    ("internals/collision.html", "Collision"),
    ("development/index.html", "Development"),
    ("development/coding-style.html", "Coding Style"),
    ("development/testing.html", "Testing &amp; QA"),
    ("development/debugging.html", "Debugging"),
    ("development/extending-the-game.html", "Extending the Game"),
    ("development/known-issues.html", "Known Issues"),
]

# Class reference pages, in the order of classes/index.html. They chain from
# classes/index.html through every class to systems/index.html.
CLASS_GROUPS = [
    ("World data model (engine-independent)", [
        "World", "Chunk", "Block", "BlockMetadata", "VoxelConfig",
        "BlockDefinitionRegistry", "MoveObjectRecord", "HubDoors",
    ]),
    ("Game", [
        "GalaxyEggbertGame", "WorldRuntime", "BlupiController", "InteractionSystem",
        "ObjectDefinitionRegistry", "Hud", "InputPad", "Sound", "SaveData",
        "ResumeLog", "DemoReplay", "Autoplay",
    ]),
    ("Rendering", ["TerrainRenderer", "TileAtlas", "WorldRenderer"]),
    ("Editor", ["WorldEditor", "CustomWorldStorage"]),
]
CLASS_PAGES = [f"classes/{name}.html" for _, names in CLASS_GROUPS for name in names]

# A page that is not in the sidebar marks this sidebar entry active instead.
SIDEBAR_PARENT = {
    "tutorials/": "tutorials/index.html",
    "internals/": "internals/index.html",
    "classes/": "classes/index.html",
}


def prefix_for(page: str) -> str:
    return "../" * page.count("/")


def rel(page: str, target: str) -> str:
    return prefix_for(page) + target


def active_entry(page: str) -> str:
    sidebar_pages = {p for _, items in SIDEBAR for p, _ in items}
    if page in sidebar_pages:
        return page
    for folder, parent in SIDEBAR_PARENT.items():
        if page.startswith(folder):
            return parent
    return ""


def render_sidebar(page: str) -> str:
    active = active_entry(page)
    lines = ['<nav id="sidebar">']
    for i, (section, items) in enumerate(SIDEBAR):
        if i:
            lines.append("")
        lines.append(f'  <div class="nav-section">{section}</div>')
        for target, label in items:
            cls = ' class="active"' if target == active else ""
            lines.append(f'  <a href="{rel(page, target)}"{cls}>{label}</a>')
    lines.append("</nav>")
    return "\n".join(lines)


def neighbours() -> dict[str, tuple[tuple[str, str] | None, tuple[str, str] | None]]:
    result: dict[str, tuple] = {}
    order = list(SITE_ORDER)
    for i, (page, _) in enumerate(order):
        prev = order[i - 1] if i > 0 else None
        nxt = order[i + 1] if i + 1 < len(order) else None
        result[page] = (prev, nxt)
    # Classes: index -> first class ... last class -> systems.
    labels = {p: label for p, label in SITE_ORDER}
    chain = ["classes/index.html", *CLASS_PAGES, "systems/index.html"]

    def label_of(p: str) -> str:
        return labels.get(p) or Path(p).stem

    for i in range(1, len(chain) - 1):
        p = chain[i]
        result[p] = ((chain[i - 1], label_of(chain[i - 1])), (chain[i + 1], label_of(chain[i + 1])))
    prev_of_index = result["classes/index.html"][0]
    result["classes/index.html"] = (prev_of_index, (chain[1], label_of(chain[1])))
    next_of_systems = result["systems/index.html"][1]
    result["systems/index.html"] = ((chain[-2], label_of(chain[-2])), next_of_systems)
    return result


def render_page_nav(page: str, nav: dict) -> str:
    prev, nxt = nav[page]
    lines = ['<div class="page-nav">']
    if prev:
        lines.append(f'    <a class="prev" href="{rel(page, prev[0])}">')
        lines.append('      <span class="label">Previous</span>')
        lines.append(f'      <span class="title">&#8592; {prev[1]}</span>')
        lines.append("    </a>")
    else:
        lines.append("    <span></span>")
    if nxt:
        lines.append(f'    <a class="next" href="{rel(page, nxt[0])}">')
        lines.append('      <span class="label">Next</span>')
        lines.append(f'      <span class="title">{nxt[1]} &#8594;</span>')
        lines.append("    </a>")
    else:
        lines.append("    <span></span>")
    lines.append("  </div>")
    return "\n".join(lines)


TAG_RE = re.compile(r"<[^>]+>")


def text_of(fragment: str) -> str:
    return " ".join(html.unescape(TAG_RE.sub(" ", fragment)).split())


def search_entry(page: str, source: str) -> dict:
    title_m = re.search(r"<title>(.*?)</title>", source, re.S)
    title = text_of(title_m.group(1)) if title_m else page
    title = re.sub(r"\s+—\s+Galaxy Eggbert Docs$", "", title)
    h1 = re.search(r"<h1[^>]*>(.*?)</h1>", source, re.S)
    sub = re.search(r'<p class="subtitle">(.*?)</p>', source, re.S)
    heads = re.findall(r"<h[23][^>]*>(.*?)</h[23]>", source, re.S)
    words = " ".join(text_of(x) for x in [h1.group(1) if h1 else "", sub.group(1) if sub else "", *heads])
    return {"t": title, "u": page, "k": words.lower()}


def main() -> int:
    check_only = "--check" in sys.argv[1:]
    problems: list[str] = []
    nav = neighbours()
    pages = [p for p, _ in SITE_ORDER] + CLASS_PAGES
    index: list[dict] = []

    for page in pages:
        path = ROOT / page
        if not path.exists():
            problems.append(f"missing page: {page}")
            continue
        source = path.read_text(encoding="utf-8")
        new, n = re.subn(r'<nav id="sidebar">.*?</nav>', lambda _: render_sidebar(page), source, count=1, flags=re.S)
        if n != 1:
            problems.append(f"{page}: no <nav id=\"sidebar\"> block")
        new, n = re.subn(r'<div class="page-nav">.*?</div>', lambda _: render_page_nav(page, nav), new, count=1, flags=re.S)
        if n != 1:
            problems.append(f"{page}: no <div class=\"page-nav\"> block")
        for needed in (f'href="{prefix_for(page)}assets/style.css"', f'src="{prefix_for(page)}assets/script.js"'):
            if needed not in new:
                problems.append(f"{page}: expected {needed}")
        if new != source:
            if check_only:
                problems.append(f"{page}: sidebar or page-nav out of date")
            else:
                path.write_text(new, encoding="utf-8")
        index.append(search_entry(page, new))

    script = ROOT / "assets" / "script.js"
    js = script.read_text(encoding="utf-8")
    block = "/* BEGIN SEARCH INDEX (generated by tools/update_site.py) */\n"
    block += "  const pages = " + json.dumps(index, ensure_ascii=False, indent=1).replace("\n", "\n  ") + ";\n"
    block += "  /* END SEARCH INDEX */"
    js_new, n = re.subn(r"/\* BEGIN SEARCH INDEX.*?/\* END SEARCH INDEX \*/", lambda _: block, js, count=1, flags=re.S)
    if n != 1:
        problems.append("assets/script.js: search index markers not found")
    elif js_new != js:
        if check_only:
            problems.append("assets/script.js: search index out of date")
        else:
            script.write_text(js_new, encoding="utf-8")

    # Link check over every HTML file, and files the site map does not know.
    known = set(pages)
    for path in sorted(ROOT.rglob("*.html")):
        relpath = path.relative_to(ROOT).as_posix()
        if relpath.startswith(".git/"):
            continue
        source = path.read_text(encoding="utf-8")
        is_redirect = 'http-equiv="refresh"' in source
        if relpath not in known and not is_redirect:
            problems.append(f"not in the site map: {relpath}")
        for link in re.findall(r'(?:href|src)="([^"]+)"', source):
            if re.match(r"^(?:[a-z]+:|#|//)", link):
                continue
            target = link.split("#", 1)[0].split("?", 1)[0]
            if not target:
                continue
            resolved = (path.parent / target).resolve()
            if not resolved.exists():
                problems.append(f"{relpath}: broken link {link}")

    for line in problems:
        print(line)
    print(f"{len(pages)} pages, {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
