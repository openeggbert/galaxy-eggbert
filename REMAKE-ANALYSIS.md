# REMAKE-ANALYSIS.md — Why Galaxy Eggbert keeps generating "piles of problems", and what to change

_Author: analysis pass, 2026-07-20. Scope: `GalaxyEggbertCNA` (the sole actively-developed
target) vs. its reference, mobile-eggbert. This is a **root-cause / structural analysis document**,
not a change to game code. It proposes what to change; it does not change it. Every recommendation
respects the existing hard constraints (faithful remake, `../mobile-eggbert` never modified, no
copied code/data, Direct-CNA+Easy3D lock)._

---

## 0. TL;DR

The project is not failing at any one feature — it is losing to a **steady stream of the same
kinds of bug**: wrong sprite icons, wrong animation frames, wrong sound channels, wrong tile
render-orientation, physics that "passes tests" but feels/behaves wrong in the live game. These are
not unrelated incidents. They are the predictable output of **five structural conditions** baked
into how the remake is built:

1. **Correctness is verified by a human looking at a screenshot** — there is no automated parity
   check against the original game. Wrong data sails through CI and is only caught by eye, one
   screenshot at a time, then re-corrected. This is the deepest cause.
2. **Load-bearing data is hand-transcribed** from a decompiled C++ source that the rules forbid
   copying or linking. Every icon = two hand-typed magic values (a frame array + an animation
   divisor) with no cross-check. `GEObjectIcons.cpp` already carries **34 recorded "Fixed" notes**
   for exactly this.
3. **There is no shared collision primitive.** The original's single swept resolver (`TestPath`) is
   replaced by fragmented per-mechanic grid probes, so fixing one mechanic (fall-lockup) *created*
   another bug (airborne wall clip). Collision is a stub, not a port.
4. **Behavior for ~70 object types is dispatched by open-coded `if (type == N)` chains** inside two
   multi-thousand-line `Update()` god-methods — no handler table, no polymorphism. Every object
   touches many scattered sites, so every edit risks a regression somewhere else.
5. **The 2D→3D tile render-mapping is an open-ended guessing problem** over ~440 icons, already
   reversed at scale three times, with ~137 icons still resting on unverified first-pass guesses.

The recommendation is **not** "rewrite everything." It is: **build the missing verification and
data-integrity infrastructure first**, then **retire the two worst structural amplifiers** (the
god-method type-dispatch and the fragmented collision), in that order. Without #1, every other fix
is another guess. See §3 for the prioritized plan.

---

## 1. What "the piles of problems" actually are

Concrete, measured evidence that the recurring bugs cluster in a few areas and are dominated by
rework, not new work.

### 1.1 Where the effort (and churn) concentrates

Task-ID references in `plan.md` by prefix (a heavily-reworked item is referenced many times, which
is itself the signal):

| Prefix | Refs | | Prefix | Refs |
|---|---|---|---|---|
| **BLUPI** (movement + anim-icon wiring) | 212 | | VISUAL | 53 |
| **MENU** | 151 | | CAM | 34 |
| **SOUND** | 146 | | SAVE / EDITOR | 24 / 24 |
| **PICKUP** | 131 | | TEST | 21 |
| E3D-MIG | 93 | | CHEAT / BUILD | 14 / 14 |
| TILE | 70 | | | |

The four dominant buckets — **BLUPI, MENU, SOUND, PICKUP** — are exactly the areas requiring
faithful reproduction of *hand-transcribed lookup data* (animation frames, sprite icon indices,
sound channel numbers, per-object behavior). That is where the churn lives.

### 1.2 The rework is whack-a-mole, and the git log shows it

Of 50 commits: **feat 24 (48%), docs 15 (30%), fix 11 (22%)**. Nearly a third of commits are
`docs:` — but most of those are not documenting features, they are **recording corrected premises**:
`docs: re-scope BLUPI-079`, `docs: TILE-052's lightning premise is wrong`, `docs: correct
SAVE-007`. That is rework overhead wearing a `docs:` label.

The `fix:` commits cluster almost entirely on **which pixels/sound/frame appear**, not on logic:

- `fix: 7 more GetObjIcon() bugs found via systematic audit` → `fix: 12 more GetObjIcon() bugs
  completing the audit sweep` — **19 wrong entries in one lookup function**, across two sweeps.
- **Balloon**: 3 separate `fix:` commits + 2 reverted attempts, because "the whole premise was wrong."
- `fix: HUD's bottom-right animation icon used the wrong sprite sheet` (4 death animations, wrong sheet).
- `fix: Down anim state was missing 2 of its 3 real icon frames`.
- `fix: tentacle real animation, not frozen frame`; `fix: goo particle real icon table`.

Representative wrong-premise cascades from `plan.md`:

- **"Sp0–Sp7" secret-power tiles (icons 158-165)** — *"the premise itself was WRONG… a
  never-confirmed guess based on the decompiled name alone."* They are hub world-select markers,
  not powers. The wrong guess cascaded into re-scoping `TILE-052` and `E3D-MIG-170`.
- **Saw-blade orientation (icon 378)** — 3 autonomous fix attempts, all wrong; still open,
  explicitly waiting on the user to look at a screenshot.
- **Pyramid/teleporter** — pyramid → "fix" to a transparent box → reverted to a pyramid the same
  day (3rd live-feedback round).

### 1.3 Bugs get "proven correct" and are then invalidated

Two standalone bug docs in the repo capture the pattern in miniature:

- `texture-distance-washout-bug.md` — an investigation reached a *"geometry/winding proven correct"*
  conclusion, later invalidated wholesale when the real cause turned out to be a face-winding
  mismatch in a **sibling repo** (`../easy-3d`'s `CubeMesh`), meaning *"everything this
  investigation observed on side faces was observed on the wrong face."*
- `missing.md` — a UV-bleed bug in CNA that `GalaxyEggbertSimple3D` had **already solved** years of
  code earlier via the shared, engine-agnostic `BlockTypes::tileUV()`; CNA re-derived its own tile
  UV lookup and reintroduced the bug. The doc's own conclusion: CNA *"was largely written by
  re-deriving logic… rather than reusing existing galaxy-eggbert code where it already existed and
  was already correct."*

---

## 2. Root causes (why the same bug classes keep coming back)

### RC-1 — No automated parity/regression check; the final arbiter of correctness is a human eye

**This is the deepest cause and the reason all the others are so expensive.** Verification today is
a three-legged stool with only one load-bearing leg:

1. **Unit / "Verify*" tools** (`VerifyInteractionSystem`, `VerifyBlupiMovement`, `VerifyGEInputPad`,
   `VerifyGESaveData`, `VerifyGEWorldEditor`, …) run on **synthetic inputs against hand-authored
   expectations**. They confirm the engine does what the author *thought* was right — they cannot
   detect that a transcribed icon index or sound channel disagrees with the original game, because
   the "expected" value was typed by the same hand that typed the code.
2. **Hand-transcribed reference docs** (`mobile-eggbert-reference/*.md`). When the transcription or a
   guessed decompiled name is wrong, every downstream fix built on it is wrong too (Sp0–Sp7,
   lightning icons).
3. **A human looking at an `xvfb-run` screenshot.** In practice this is the *only* check that
   catches the visual/behavioral half — and it does not run in CI, does not run on every change, and
   requires the user personally for anything subtle. `plan.md` states this outright: gravity/jump
   magnitude *"needs the user's own live-feel judgment"*; Saw/Fox/Balloon were only settled *"with
   an actual attached screenshot this time"* from the user.

Net effect: **a wrong datum is invisible to the whole automated pipeline** and surfaces only when a
person looks. That is a bug *factory*, and it is why BLUPI/SOUND/PICKUP/VISUAL churn regenerates no
matter how many individual fixes land.

### RC-2 — Load-bearing data is hand-transcribed with no possible cross-check

The faithful-remake rules (correctly) forbid copying or linking mobile-eggbert's `Tables.cpp` /
`Decor.cpp`. The consequence is that every animation, icon, and sound datum is **manually
re-transcribed** into Galaxy Eggbert. `GEObjectIcons.cpp` (854 lines) is essentially one function
with **154 `case` labels** and ~30 hand-typed `k___[]` frame arrays. Each object needs **two
independent hand-entered values that are both easy to get wrong**:

- the **frame array** — the real tables are *not* monotonic ranges; they oscillate, embed `-1`
  blanks, and jump (`kExplo4 = {12,13,14,15,7,8,9,10,11}`). Guessing "ascending range" was wrong
  repeatedly.
- the **animation divisor** (`p / N`) — must independently equal the object's real
  `Config::ScaleDiv(...)`. A whole cluster of 2026-07-20 fixes were "array right, divisor wrong."

The file already carries **34 `Fixed 2026-…` annotations** documenting this exact failure mode.
There is no validation, so each of ~70 types is an independent, permanent opportunity for a silent
visual bug.

### RC-3 — No shared collision primitive; collision is a stub, and per-mechanic patches fight each other

The original resolves movement with **one swept check** (`Decor::TestPath`) applied to the final
merged position every frame, regardless of which mode produced the move. Galaxy Eggbert has **no
such primitive**. Instead:

- `GEBlupiController` implements *simplified column-based* collision (topmost solid block = floor) +
  a `kStepLimit` step-up rule — explicitly **not** a port of the pixel-rectangle `BlupiRect` /
  `BlupiAdjust` / `BlupiBloque` system. Per-tile-independent behavior (walking under a floating
  pillar) is faked by **excluding specific icons** from ground resolution.
- Collision is **fragmented into separate probes** (`GroundHeightAt`, `CeilingHeightAt`,
  `HasJumpHeadroom`, `IsSolidAt`, `GetBarreCellType`, the `TryMoveAxis` step-up gate), each bolted
  on for one mechanic.
- **There is no horizontal wall collision for any airborne movement at all.** `TryMoveAxis`
  short-circuits on `!m_onGround`, so a jumping/floating/riding Blupi passes straight through side
  walls. This gap was *created* by a patch: fixing a fall-lockup bug by bypassing the gate while
  airborne removed the only thing that could have blocked airborne movement.

This is the signature of a missing foundation: fixes to one mechanic keep manufacturing bugs in
another, because there is no single place that owns "can this move happen."

### RC-4 — ~70 object types dispatched by open-coded type-checks inside god-methods

There is no per-object class, no virtual `Update()`, no handler table. Behavior is open-coded as
scattered `if (obj.type == ObjectTypeN)` branches (and a few `switch` blocks) **inside single
enormous methods**:

| Method | Approx. size |
|---|---|
| `GalaxyEggbertCnaGame::Update` | **~2,073 lines** |
| `GEInteractionSystem::Update` | **~1,677 lines** |
| `GalaxyEggbertCnaGame::Draw` | **~817 lines** |
| `GEBlupiController::Step` | **~715 lines** |

`ObjectType` is referenced **214 times** in `GEInteractionSystem.cpp` alone. The same type constant
is re-tested in many independent `if` blocks per frame. **Fixing or adding one object means finding
every place its type is mentioned in a 1,677-line function** — which is precisely how "fix object X"
regresses object Y.

### RC-5 — The 2D→3D render-mapping is an unbounded guessing problem, handled by iterative human ID

A flat side-on 64×64 sprite carries **no depth, no facing, and no "bulk material vs. thin object"
signal.** Mapping ~440 tile icons to a 3D render mode (`UniformCube` / `Billboard` /
`DirectionalCube` / `ThinMechanical` / thin-bar / water surface) is therefore a judgment call *per
icon*. The design doc (`mobile-eggbert-reference/15-3d-render-mapping-design.md`) has already been
reversed three times (1 exception → ~100 exceptions → a whole new `DirectionalCube` mode), and by
its own admission **~137 icon identities still rest on unverified first-pass agent guesses.** The
current handling is direct human identification via 190 KB of per-icon questionnaires — faithful,
but slow, unverified, and a standing well of future render bugs. (RC-1 is why these can only be
caught by eye.)

### RC-6 (compounding) — The `*ThisFrame()` signal bus makes every new feature expensive to add correctly

`GEInteractionSystem` is deliberately blind to `GEBlupiController` and graphics, so results flow
back via **17 one-frame boolean flags** consumed by the orchestrator. Adding one interaction touches
**three classes across ≥3 files** (a flag member, a line in a 15-entry reset list, a `= true` at the
trigger site, a getter, and a consumer `if` in `GalaxyEggbertCnaGame::Update`). Correctness depends
on **prose-documented ordering** ("reads *last* frame's value — a one-frame lag"; "must be consumed
before the flag resets"), not on an enforced contract. This does not by itself cause the visual
bugs, but it multiplies the cost and risk of every fix that must touch it — so it belongs on the
list.

> **Note on the decoupling:** `NEXT.md`/§6 defends the `GEInteractionSystem` ↔ `GEBlupiController`
> decoupling as a deliberate, repeatedly-reaffirmed choice. This analysis does **not** propose
> merging those classes or adding a direct dependency. It proposes replacing the *hand-rolled
> parallel-boolean transport* with a single typed event/queue (RC-6 fix below) — the decoupling
> stays; only the boilerplate-heavy, ordering-fragile transport changes. Treat any such change as a
> proposal to discuss with the user, not a settled directive.

### RC-7 (aggravating) — Re-derivation instead of reuse, and sibling-repo coupling

Two independent bug docs (`missing.md`, `texture-distance-washout-bug.md`) show the same thing: CNA
re-derives logic that already existed and was correct elsewhere (reintroducing solved bugs), and a
real class of render bugs actually originates in **sibling repos under independent development**
(`../easy-3d`, `../easy-gl`, `../cna`, `../sharp-runtime`). The one persistent CI failure
(`easy-gl-resource-smoke-tests`) and the Vulkan-only `BasicEffect`/`SkinnedEffect` bugs live in
those repos, not here — so some "Galaxy Eggbert problems" cannot even be fixed from this repo, and
"fixed" visuals must be re-verified on both graphics backends.

---

## 3. What to change — prioritized

Ordered so that each tier makes the next tier cheaper. **Do P0 before spending more on individual
feature fixes** — otherwise every fix is another un-checkable guess.

### P0 — Build the missing correctness infrastructure (attacks RC-1, RC-2, RC-5)

**P0-1. An automated golden-image / behavioral parity harness.**
The single highest-leverage change. Concretely:

- A headless-capture test target that loads a fixed world + fixed camera + fixed input script,
  steps N deterministic ticks, and captures screenshots at known frames (the `xvfb-run` +
  env-var-gated instrumentation already used ad hoc, promoted to a *permanent, committed* harness
  instead of throwaway scaffold reverted every time).
- Store **golden images** and diff against them in CI, so any change that alters rendering is caught
  automatically instead of by eye. Where a true golden from the original game is available (a
  recorded frame or a crop from the real sprite sheet), diff against *that*; otherwise diff against
  a previously-approved Galaxy Eggbert frame to catch *regressions* even before parity is perfect.
- Extend the same idea to behavior: a deterministic-tick trace (position/velocity/anim-state per
  tick) captured to a text log and diffed, so "passes unit tests but feels wrong live" becomes a
  checkable artifact rather than a live-feel judgment.

_Why first:_ it converts the entire visual/behavioral half of the project from "human catches it
eventually" to "CI catches it immediately," which is the only thing that stops RC-1's bug factory.

**P0-2. A data-integrity check for the hand-transcribed tables (attacks RC-2).**
Without copying mobile-eggbert data, you can still **validate** what has been transcribed:

- Add assertions/tests that catch the *known* transcription failure modes: no unintended monotonic
  assumptions, `-1` sentinels preserved, array lengths match the documented frame counts in
  `mobile-eggbert-reference/08-animations.md`, and each object's divisor matches the value the
  reference records for its `ScaleDiv`.
- Cross-check every `GetObjIcon` array/divisor against the reference doc *programmatically* (parse
  the reference tables, compare) so the 34-and-counting "Fixed" notes stop being discovered by hand.
- If the user is ever willing to approve a **read-only extraction tool** (run once against
  `../mobile-eggbert`, emitting a comparison report — not copying code into the tree), that would
  make the transcription self-validating. This needs explicit user approval per the reuse rules;
  flag it, don't do it unilaterally.

**P0-3. Freeze the render-mapping guessing surface (attacks RC-5).**
Mark the ~137 unverified icon identities as an explicit, queryable "unverified" set so they cannot be
silently treated as confirmed, and gate them behind the P0-1 golden harness so any render change to
them is at least regression-checked. Do **not** keep taking autonomous "best guesses" at geometry
(the standing directive already says this for the visual-judgment items — make it structural).

### P1 — Remove the two worst structural amplifiers (attacks RC-3, RC-4)

**P1-1. Introduce one shared collision/movement resolver (attacks RC-3).**
Replace the fragmented per-mechanic probes with a single swept-AABB (or swept-point) resolver that
*every* mode routes its final move through, mirroring the original's `TestPath` shape: compute the
merged intended end position, then resolve it once against terrain for **both** axes and **both**
grounded and airborne states. This closes the airborne-wall gap as a side effect and stops
per-mechanic patches from fighting each other. Guard the change with P0-1's behavioral trace so
already-verified grounded behavior can't silently regress. This is a real, bounded piece of work —
scope it as its own task and get user sign-off before starting, since it touches
already-"verified" movement.

**P1-2. Replace open-coded type-dispatch with a handler table (attacks RC-4).**
Give each `ObjectType` (or each behavior family) a single home — a per-type handler struct/function
registered in a table, replacing the scattered `if (type == N)` chains inside the god-methods. This
does not require class-per-object OOP; a table of `{ObjectType → update fn, icon fn, hitbox}` is
enough and keeps the existing data-oriented style. Result: "fix object X" touches exactly one place.
Do this incrementally, one family at a time, behind P0-1's golden harness.

### P2 — Reduce friction and drift (attacks RC-6, RC-7, and doc sprawl)

- **P2-1. Replace the 17 parallel `*ThisFrame()` booleans with one typed per-frame event queue**
  (RC-6). Keep the decoupling; drop the boilerplate and the prose-documented ordering hazards.
  Propose to the user first (see the note under RC-6).
- **P2-2. Reuse before re-deriving** (RC-7). When a CNA render/math bug has a plausible 2D/pixel
  root cause, first check the engine-agnostic `include/GalaxyEggbert/` tree, the researched
  `mobile-eggbert-reference/` notes, and current verification tests (the `missing.md` lesson).
  Consider promoting more proven math into the shared engine-agnostic layer.
- **P2-3. Isolate sibling-repo risk** (RC-7). Pin/record the sibling-repo commits Galaxy Eggbert is
  verified against, and clearly quarantine known-upstream failures (the `easy-gl` smoke test, the
  Vulkan `BasicEffect`/`SkinnedEffect` issues) so they stop reading as Galaxy Eggbert regressions.
- **P2-4. Cut the documentation-state tax.** `plan.md` is ~505 KB / 5,541 lines and `NEXT.md`
  ~115 KB; a large share of every session is spent reading and re-reconciling state (and ~30% of
  commits are premise-correction bookkeeping). Consider a compact, authoritative "current truth"
  index separate from the historical log, so contributors (human or agent) stop re-deriving state.

---

## 4. Sequenced plan (smallest coherent steps)

1. **P0-1a** — promote the existing throwaway screenshot instrumentation into a permanent,
   committed, deterministic headless-capture target (no golden diffing yet). _Verifies:_ it runs in
   CI and produces stable frames.
2. **P0-1b** — add golden-image diffing for a handful of representative worlds/objects; wire into
   `ctest`. _Verifies:_ an intentional 1-pixel change fails CI.
3. **P0-2** — add the `GetObjIcon` data-integrity tests (length/sentinel/divisor checks vs. the
   reference docs). _Verifies:_ re-introducing one of the 34 historical bugs fails CI.
4. **P0-3** — mark the unverified render-mapping icons as a queryable set, gated behind P0-1.
5. **P1-1** — the shared collision resolver (own task, user sign-off, behavioral-trace guarded).
6. **P1-2** — the object-type handler table, migrated one family at a time behind the golden harness.
7. **P2-x** — event queue, reuse discipline, sibling-repo pinning, doc-state index — opportunistic.

Each step ends with the project's existing sign-off ritual (build → `Verify*` → full `ctest` → live
headless check → commit → push) **plus** the new golden/trace checks once they exist.

---

## 5. Constraints this analysis explicitly respects

- **Faithful remake only.** Nothing here proposes a new gameplay mechanic. The parity harness and
  data checks exist precisely to make the remake *more* faithful, not less.
- **`../mobile-eggbert` is never modified.** The one place that would touch it (P0-2's optional
  read-only extraction tool) is flagged as requiring explicit user approval and is not assumed.
- **No copied code/data.** P0-2 *validates* transcription; it does not license copying tables in.
- **Direct-CNA + Easy3D lock, no alternate engine work.** P1/P2 are all inside
  `GalaxyEggbertCNA` and the shared engine-agnostic tree. The retired pre-CNA source is available
  only through git history at `4afd53e`.
- **The `GEInteractionSystem` decoupling stays.** RC-6's fix changes the *transport*, not the
  boundary, and is offered as a proposal to confirm with the user.
- **Items needing live human judgment stay human-gated** — this analysis does not authorize new
  autonomous "best guesses" at Saw orientation, `AscenseurVertigo`, water surface, etc. It proposes
  making that gating structural (P0-3) rather than a per-item reminder.

---

## 6. Evidence index

| Claim | Source |
|---|---|
| BLUPI/MENU/SOUND/PICKUP dominate task churn | `plan.md` (ID-prefix counts) |
| 19 `GetObjIcon` bugs across two audit sweeps; Balloon 3+ fixes; Sp0–Sp7 wrong premise | `plan.md`, `git log` |
| 34 recorded "Fixed" transcription notes; 154 `case` labels | `src/GalaxyEggbertCNA/Game/GEObjectIcons.cpp` |
| `Update` ~2073 lines, `GEInteractionSystem::Update` ~1677, `ObjectType` ×214 | `GalaxyEggbertCnaGame.cpp`, `GEInteractionSystem.cpp` |
| Airborne no-wall-collision; `!m_onGround` bypass; no `TestPath` equivalent | `GEBlupiController.cpp` (`TryMoveAxis`), `GEBlupiController.hpp:136-170` |
| Collision is a column-stub, not a `BlupiRect`/`BlupiAdjust`/`BlupiBloque` port | `GEBlupiController.cpp:263-417`, `mobile-eggbert-reference/10-blupi-mechanics.md` |
| 40 `AnimState` values vs. 87 real `BlupiAction` states | `grep AnimState src/GalaxyEggbertCNA`, `mobile-eggbert-reference/08/10-*.md` |
| Render-mapping reversed 3×; ~137 unverified icons | `mobile-eggbert-reference/15-3d-render-mapping-design.md` |
| 17 `*ThisFrame()` flags, 3-file boilerplate per feature, prose-ordering hazards | `GEInteractionSystem.hpp`, `GalaxyEggbertCnaGame.{hpp,cpp}` |
| Verification = synthetic unit tests + transcribed docs + user screenshot | `plan.md`, `NEXT.md` §4/§5/§7.5 |
| Re-derivation reintroduces solved bugs; sibling-repo-origin bugs | `missing.md`, `texture-distance-washout-bug.md`, `NEXT.md` §5 |
