# Enemy/Object Behavior Model

**Status:** architectural overview only — describes the general movement/collision *system*, not a
per-type exhaustive behavior catalog (that level of detail belongs in `03-objects.md` per-type as
it gets filled in under `DOC-003`).

From `Decor.cpp`'s own architectural documentation (top-of-file comment, paraphrased): most
enemies/effects are **table-driven** — a `MoveObject` moves linearly between `posStart` and
`posEnd` at `stepAdvance`/`stepRecede` speed with `timeStopStart`/`timeStopEnd` dwell timers
(`MoveObjectStepLine`), while its visual animation/lifetime is a phase counter indexed into a
`Tables.cpp` animation array (`MoveObjectStepIcon`). A **minority** of object types are
**hand-coded** with bespoke per-type logic in `MoveObjectStepIcon` and its helpers — named examples
in the source comment: dynamite, "charging" enemies, followers (`ObjectType96`/`97`, which track
Blupi's position directly rather than patrolling a fixed line), cloud-nets, and crates.

Patrol-style enemies with `posStart == posEnd` in the raw file data (a common authoring pattern —
confirmed by galaxy-eggbert's own Simple3D loader, which detects this case and synthesizes a small
patrol range: `spec.posStart.x_ -= 2.0f; spec.posEnd.x_ += 2.0f`) patrol back and forth over a
short fixed range around their placed position rather than staying stationary.

Collision for both Blupi and objects is tile-based (not swept): an axis-aligned rectangle is
tested against the tile cells it overlaps via `IsBlocIcon()`/`IsPassIcon()`; a single blocking cell
makes the whole rectangle "occupied". This is the source-of-truth collision model mobile-eggbert
uses — distinct from, and not necessarily reusable for, galaxy-eggbert's own 3D grid-collision code
(e.g. `GEBlupiController` in the CNA target implements its own simplified 3D grid collision with
step-up traversal, not a transcription of this 2D system).

## The follower pattern (`ObjectType96`/`97`), as implemented 2026-07-03

mobile-eggbert's real follower behavior (`Decor::MoveObjectFollow`) promotes a dormant `96` to an
awake `97` once Blupi is near, then homes toward Blupi at roughly 1 px/tick.
`GEDecorSystem`'s port (Simple3D) approximates this with a distance-triggered wake (≈2.5 world
units) followed by continuous speed-based motion toward Blupi's live position every frame, using an
idle animation (`table_follow1`) while dormant and a chase animation (`table_follow2`) while awake
— a reasonable approximation of the real tick-stepped homing, not a byte-exact port.
