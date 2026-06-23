#pragma once

// This file documents Simple3D API gaps encountered during the galaxy-eggbert
// Simple3D migration (S3D-1 pass). All missing features are stubbed here as
// no-op macros or empty inline functions so the port compiles without them.
//
// When Simple3D adds a feature, find the corresponding TODO here and replace
// the stub with a real call. Document each gap in docs/SIMPLE3D_GAPS.md too.

// ─── Tile atlas / UV material ────────────────────────────────────────────────
// Simple3D has no API to set a UV-offset on a model's material at runtime.
// The old Urho3D code used SetShaderParameter("UOffset"/"VOffset").
// In S3D-1 we just use grey Box.mdl blocks; tile textures come in S3D-2.
// TODO(S3D-2): Replace GETerrainRenderer with atlas-UV material when available.

// ─── Billboard sprite with UV region ─────────────────────────────────────────
// Simple3D has Entity::AddBillboard(texturePath, size) but no UV-region crop.
// Blupi and object sprites come from large sprite sheets (blupi.png, element.png).
// In S3D-1, characters are Box.mdl cubes; billboard sprites come in S3D-3/4.
// TODO(S3D-3): Use Entity::AddBillboard + UV-crop API when available.

// ─── Sky dome / fog / zone ───────────────────────────────────────────────────
// Simple3D exposes no sky dome, fog colour, or ambient zone API.
// In S3D-1 we use SetClearColor() as a sky approximation.
// TODO: Expose SetFogColor / ambient zone / sky mesh in Simple3D.

// ─── UI Image / gauge / icon ─────────────────────────────────────────────────
// Simple3D has UI::Image but the HUD uses BorderImage (sprite sheet slice).
// In S3D-1 the HUD is text-only via Simple3D::Label.
// TODO(S3D-5): Port gauge, life icons, key icons once Simple3D UI panel API stabilises.

// ─── Particle / explosion sprite ─────────────────────────────────────────────
// Simple3D has Entity::AddParticleEmitter(path) using Urho3D ParticleEffect XML.
// The old code used BillboardSet + animation frames from explo.png.
// In S3D-1, explosions are skipped (no visual). A future pass (S3D-4) will use
// AddParticleEmitter("Particles/Explosion.xml") as an interim.
// TODO(S3D-4): Implement explosion via AddParticleEmitter.

// ─── Audio channels ──────────────────────────────────────────────────────────
// Simple3D's Game::PlaySound plays a one-shot sound with a single volume param.
// The old SoundManager mapped 93 indexed channels with per-channel volume/pitch
// from a tableVolumePitch table. In S3D-1 we map only the 5 most critical sounds.
// TODO(S3D-7): Expose per-channel volume + loop control in Simple3D::Audio.

// ─── Input Actions ───────────────────────────────────────────────────────────
// Simple3D DOES have BindAction/BindAxis2D (InputActions.h) — using them in S3D-1.

// ─── Save / load ─────────────────────────────────────────────────────────────
// Simple3D has <Simple3D/SaveData/SaveData.h> but the galaxy-eggbert GameData
// is a 640-byte binary blob. Integration deferred to a dedicated pass.
// TODO: Port GameData to Simple3D::SaveData JSON or raw binary wrapper.
