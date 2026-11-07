#!/usr/bin/env bash
# Read-only inventory helper for docs/provenance/.
#
# Prints, to stdout only (nothing is written anywhere, nothing is built, no other repository is
# read): file and line counts per top-level area from `git ls-files`, the AI-related records in the
# git history and in the tracked documents, the declared dependencies, and the places where the
# repository's own text names an original-game source. Run from anywhere inside the repository:
#
#     bash docs/provenance/inventory.sh            # everything
#     bash docs/provenance/inventory.sh areas      # one section: areas | history | firstadded |
#                                                  #   records | deps | original | licences
#
# Counts are of tracked files only. "Lines" are counted for text files (by extension), "bytes" for
# every file. Numbers are facts about the tree at the time of the run, not judgements.
set -u
cd "$(git rev-parse --show-toplevel)" || exit 1
section="${1:-all}"
want() { [ "$section" = all ] || [ "$section" = "$1" ]; }

TEXT_RE='\.(c|cc|cpp|cxx|h|hpp|hxx|py|sh|in|cmake|txt|md|csv|json|xml|yml|yaml|html|css|js|java|gradle|properties|mgcb|pro|template|vwr_txt)$|(^|/)(CMakeLists\.txt|Doxyfile|LICENSE|\.gitignore)$'

# area name -> git pathspec(s). Order matters only for display. An area is a set of pathspecs.
areas() {
cat <<'EOF'
src/GalaxyEggbert (shared game, editor, worlds)|src/GalaxyEggbert
src/GalaxyEggbertCNA (application host)|src/GalaxyEggbertCNA
include/|include
tests/ (unit tests, golden frames)|tests
tools/*.cpp (Verify* test executables, generators)|tools/*.cpp
tools/worlds/ (world generator scripts)|tools/worlds
tools/autoplay/ (scripted playthroughs)|tools/autoplay
tools/qa_steps/|tools/qa_steps
tools/rd4_to_gltf/ (Ray Dream converter)|tools/rd4_to_gltf
tools/audit/ (ledger generators)|tools/audit
tools/a3d tools/terrain_relief tools/music tools/glue tools/bolt tools/data|tools/a3d tools/terrain_relief tools/music tools/glue tools/bolt tools/data
tools/*.py *.sh (other scripts)|tools/*.py tools/*.sh
worlds3d/ (generated .vwr worlds)|worlds3d
worlds/ (original-format .txt levels)|worlds
Content/ (sprites, sounds, music, backgrounds)|Content
avatars3d/ (converted 3D models)|avatars3d
textures3d/|textures3d
mobile-eggbert-reference/ (notes, cropped images)|mobile-eggbert-reference
plans/ (plans, briefs, audit ledgers, evidence)|plans
web/ (project website)|web
android/|android
cmake/ packaging/ .github/|cmake packaging .github
root documents (*.md in the root, LICENSE, Doxyfile)|*.md LICENSE Doxyfile CMakeLists.txt .gitignore .bitbackupignore
EOF
}

if want areas; then
  echo "== files, lines of text, bytes per area (git ls-files) =="
  printf '%-62s %7s %9s %12s\n' area files lines bytes
  total_f=0
  areas | while IFS='|' read -r name specs; do
    # shellcheck disable=SC2086
    files=$(git ls-files -- $specs 2>/dev/null)
    n=$(printf '%s' "$files" | grep -c .)
    # symlinks (android/app/src/main/assets/*) are counted as files of size 0 here
    bytes=0; lines=0
    if [ "$n" -gt 0 ]; then
      bytes=$(printf '%s\n' "$files" | while IFS= read -r f; do [ -f "$f" ] && [ ! -L "$f" ] && stat -c %s "$f"; done | awk '{s+=$1} END{print s+0}')
      lines=$(printf '%s\n' "$files" | grep -E "$TEXT_RE" | while IFS= read -r f; do [ -f "$f" ] && [ ! -L "$f" ] && cat "$f"; done | wc -l)
    fi
    printf '%-62s %7s %9s %12s\n' "$name" "$n" "$lines" "$bytes"
  done
  echo
  echo "tracked files in total: $(git ls-files | wc -l)"
  echo
  echo "== tracked files by extension (top 15) =="
  git ls-files | sed -E 's/.*\.([A-Za-z0-9_]+)$/\1/;t;s/.*/(none)/' | sort | uniq -c | sort -rn | head -15
  echo
  echo "== counts the provenance documents quote =="
  echo "world generator scripts (tools/worlds/world*.py): $(git ls-files 'tools/worlds/world*.py' | wc -l)"
  echo "shipped .vwr worlds: $(git ls-files 'worlds3d/*.vwr' | wc -l)"
  echo ".vwr worlds without a generator of the same name: $(for v in $(git ls-files 'worlds3d/*.vwr' | sed 's|worlds3d/||;s|\.vwr$||'); do [ -f "tools/worlds/$v.py" ] || printf '%s ' "$v"; done)"
  echo "autoplay scripts (tools/autoplay/*.txt, excluding spare/): $(git ls-files 'tools/autoplay/*.txt' | grep -vc '/spare/')"
  echo "converted model directories (avatars3d/objects/*/avatar.skinnedmodel.json): $(git ls-files 'avatars3d/objects/*/avatar.skinnedmodel.json' | wc -l)"
  echo "terrain reliefs (avatars3d/terrain/*.gtr): $(git ls-files 'avatars3d/terrain/*.gtr' | wc -l)"
  echo "sound effects (Content/sounds/*.wav): $(git ls-files 'Content/sounds/*.wav' | wc -l)"
  echo "music tracks (Content/music/*.ogg): $(git ls-files 'Content/music/*.ogg' | wc -l)"
  echo "sprite sheets and UI images (Content/icons/*.png): $(git ls-files 'Content/icons/*.png' | wc -l)"
  echo "backgrounds (Content/backgrounds/*.png): $(git ls-files 'Content/backgrounds/*.png' | wc -l)"
  echo
fi

if want history; then
  echo "== git history =="
  echo "commits: $(git rev-list --count HEAD)"
  echo "first commit: $(git log --reverse --format='%ad' --date=short | head -1)   last: $(git log -1 --format='%ad' --date=short)"
  echo "authors:"; git log --format='  %an <%ae>' | sort | uniq -c
  echo "committers:"; git log --format='  %cn <%ce>' | sort | uniq -c
  echo "commits per month:"; git log --format='%ad' --date=format:%Y-%m | sort | uniq -c | sed 's/^/  /'
  echo "commit subjects by conventional prefix:"
  git log --format='%s' | sed -E 's/^([a-z]+)(\(.*\))?!?:.*/\1/;t;s/.*/(no prefix)/' | sort | uniq -c | sort -rn | head -12 | sed 's/^/  /'
  echo "commits with a Co-Authored-By trailer: $(git log --format='%(trailers:key=Co-Authored-By,valueonly)' | grep -c .)"
  echo "commits whose message contains 'Generated with': $(git log --format='%B' | grep -c 'Generated with')"
  echo "commits whose subject or body names Claude or Codex:"
  git log -i -E --grep='claude|codex' --format='  %h %ad %s' --date=short
  echo "merges:"; git log --merges --format='  %h %ad %s' --date=short
  echo "branches: $(git branch -a --format='%(refname:short)' | tr '\n' ' ')"
  echo "when the instruction files first appeared:"
  for f in CLAUDE.md AGENTS.md plan.md handoff.md; do
    printf '  %-10s %s\n' "$f" "$(git log --diff-filter=A --format='%ad %h' --date=short -- "$f" | tail -1)"
  done
  echo
fi

if want firstadded; then
  echo "== text files in src/ include/ tests/ tools/ by the period in which git first added them =="
  echo "(first-add date is not authorship; it only shows how much of the tree already existed when"
  echo " CLAUDE.md first appeared, 2026-06-11, and when the completion campaign started, 2026-10-25)"
  git log --diff-filter=A --no-renames --format='C %ad' --date=format:%Y-%m-%d --name-only -- src include tests tools |
  python3 -I -c '
import collections, subprocess, sys
first, cur = {}, None
for line in sys.stdin:
    line = line.rstrip("\n")
    if line.startswith("C "): cur = line[2:]
    elif line: first[line] = cur          # newest first, so the last assignment is the oldest add
text = (".cpp", ".hpp", ".h", ".py", ".sh", ".txt", ".csv", ".md", ".json", ".cmake")
tracked = subprocess.check_output(["git", "ls-files", "--", "src", "include", "tests", "tools"], text=True).split("\n")
def period(d):
    if d < "2026-06-11": return "1 before 2026-06-11"
    if d < "2026-08-01": return "2 2026-06-11 .. 2026-07-31"
    if d < "2026-10-01": return "3 2026-08 .. 2026-09"
    if d < "2026-10-25": return "4 2026-10-01 .. 2026-10-24"
    return "5 2026-10-25 and later"
agg = collections.defaultdict(lambda: [0, 0])
for f in tracked:
    if f in first and f.endswith(text):
        try: n = sum(1 for _ in open(f, "rb"))
        except OSError: continue
        a = agg[period(first[f])]; a[0] += 1; a[1] += n
print("  %-32s %6s %9s" % ("period", "files", "lines"))
for k in sorted(agg): print("  %-32s %6d %9d" % (k[2:], agg[k][0], agg[k][1]))
'
  echo
fi

if want records; then
  echo "== the repository's own records of AI use =="
  echo "tracked files (outside avatars3d/) that mention Claude or Codex:"
  git grep -liE 'claude|codex' -- . ':!avatars3d' | sed 's/^/  /'
  echo
  echo "briefs written for the Codex CLI (plans/codex/):"
  git ls-files plans/codex | sed 's/^/  /'
  echo
  echo "ledger lines saying 'Codex' (file: count):"
  git grep -ciE 'codex' -- handoff.md plan.md plans | grep -v ':0$' | sed 's/^/  /'
  echo
  echo "the session summary that calls itself AI-assisted:"
  git grep -nE 'AI-assisted' -- plans/SESSION_SUMMARY_2026-10.md | sed 's/^/  /'
  echo
fi

if want deps; then
  echo "== declared dependencies =="
  echo "-- CMakeLists.txt: add_subdirectory / find_package / FetchContent / URL --"
  grep -nE 'add_subdirectory|find_package|FetchContent_Declare|^[[:space:]]*(URL|GIT_REPOSITORY) ' CMakeLists.txt | sed 's/^/  /'
  echo "-- .gitmodules --"
  if [ -f .gitmodules ]; then sed 's/^/  /' .gitmodules; else echo "  (none)"; fi
  echo "-- CI: checked-out repositories and pinned refs --"
  grep -nE 'repository:|ref:' .github/workflows/*.yml | sed 's/^/  /'
  echo "-- CI: actions used --"
  grep -hE 'uses:' .github/workflows/*.yml | sed 's/^ *//' | sort -u | sed 's/^/  /'
  echo "-- CI: system packages --"
  sed -n '/apt-get install/,/nlohmann/p' .github/workflows/cna-ci.yml | tr -s ' \\' '\n' | grep -E '^lib|^nlohmann|^ninja|^ccache|^build|^pkg|^wayland' | sort -u | tr '\n' ' '; echo
  echo "-- Android: Gradle plugins, repositories, wrapper --"
  grep -nE "classpath|id '|repositories|mavenCentral|google\(\)|implementation|distributionUrl" android/build.gradle android/app/build.gradle android/gradle/wrapper/gradle-wrapper.properties | sed 's/^/  /'
  echo "-- Python modules imported by tools/ and mobile-eggbert-reference/tools (non-stdlib candidates) --"
  git ls-files '*.py' | xargs grep -hE '^[[:space:]]*(import|from) (numpy|PIL|bpy|scipy|cv2|trimesh|pygltflib|requests)' 2>/dev/null | sed -E 's/^[[:space:]]*(import|from) ([A-Za-z_0-9]+).*/\2/' | sort | uniq -c | sed 's/^/  /'
  echo "-- external programs called by scripts --"
  git grep -ohE '\b(ffmpeg|blender|xvfb-run|ffprobe|emcc|emcmake|fluidsynth|xdotool|ccache|ninja|gradlew)\b' -- tools '*.sh' '*.py' CMakeLists.txt | sort | uniq -c | sed 's/^/  /'
  echo "-- Python requirements files, package manifests --"
  git ls-files | grep -iE 'requirements|pyproject|Pipfile|package(-lock)?\.json' | sed 's/^/  /'; echo "  (end of list)"
  echo "-- symlinks tracked in git (not real copies) --"
  git ls-files -s | awk '$1=="120000"{print $4}' | sed 's/^/  /'
  echo
fi

if want original; then
  echo "== where the repository's own text names the original game =="
  echo "-- sources that cite a file:line of the original C++ (count of lines mentioning Tables.cpp/Decor.cpp/...) --"
  git grep -cE '(Decor|Tables|Blupi|InputPad|Sound|Text|Pixmap|Event|Def|Misc|Menu|Jauge|Action)\.cpp:?[0-9]*' -- src include | awk -F: '{s+=$2; n++} END{print "  " n " files, " s " lines"}'
  echo "-- largest files that transcribe or cite original tables --"
  git grep -lE 'Tables\.cpp|table_[a-z_]+' -- src include | while read -r f; do printf '  %6s %s\n' "$(wc -l < "$f")" "$f"; done | sort -rn | head -12
  echo "-- places that say 'transcribed' --"
  git grep -nE 'ranscri' -- src include | cut -c1-170 | sed 's/^/  /' | head -30
  echo "-- places that quote the original authors' licence --"
  git grep -nE 'GPLv3, ' -- . ':!avatars3d' | cut -c1-200 | sed 's/^/  /'
  git grep -nE 'GPLv3, ' -- avatars3d | cut -c1-200 | sed 's/^/  /'
  echo "-- the asset ledgers --"
  for f in plans/audit/asset_sources.csv plans/audit/render_representation.csv plans/inventory/assets.md plans/audit/model_sources.md; do
    [ -f "$f" ] && printf '  %-48s %s lines\n' "$f" "$(wc -l < "$f")"
  done
  echo "  asset_sources.csv statuses:"
  python3 -I -c 'import csv,collections,sys; c=collections.Counter(r["status"] for r in csv.DictReader(open(sys.argv[1], newline="", encoding="utf-8"))); [print("   ", n, k) for k, n in sorted(c.items())]' plans/audit/asset_sources.csv 2>/dev/null
  echo
fi

if want licences; then
  echo "== licence files and headers in this repository =="
  git ls-files | grep -E '(^|/)(LICENSE|COPYING|NOTICE)[^/]*$' | while read -r f; do
    printf '  %-28s %s | %s\n' "$f" "$(sed -n '1p;3p' "$f" | tr -s ' \n' ' ' | cut -c1-90)" "$(wc -l < "$f") lines"
  done
  echo "source files carrying a licence/copyright header (src include tests tools cmake):"
  echo "  $(git grep -lE 'SPDX-License-Identifier|GNU General Public License|Copyright \(c\)|Copyright ©' -- src include tests tools cmake | wc -l) files"
  echo "licence-looking lines in the root README: $(grep -ciE 'licen[cs]e' README.md)"
  echo "the first three lines of the licence texts at the sibling repositories' roots (read only):"
  base="$(cd .. && pwd)"
  for r in "$base/../libcna/cna" "$base/../libcna/cna-engine" "$base/../libcna/easy-gl" "$base/../libcna/sharp-runtime" "$base/../libcna/meta-gl" "$base/mobile-eggbert"; do
    if [ -f "$r/LICENSE" ]; then printf '  %-22s %s\n' "$(basename "$r")" "$(sed -n '1,4p' "$r/LICENSE" | grep -v '^$' | head -2 | tr '\n' ' ')"; fi
  done
  echo
fi
