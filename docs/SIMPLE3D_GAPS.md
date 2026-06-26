# Simple3D API Gaps — Galaxy Eggbert

**Single source of truth for missing Simple3D APIs** required by the galaxy-eggbert
Simple3D port (`GalaxyEggbertSimple3D`). Update this file when simple-3d adds a feature.

Code stubs in `src/GalaxyEggbertSimple3D/` and references in `NEXT.md` point here
rather than repeating the list.

---

## Still missing

None — all previously identified gaps are now implemented in simple-3d.

---

## Available in Simple3D

| Feature | Simple3D API |
|---|---|
| Tile atlas material with per-tile UV offset | `Entity::SetTileTexture(texPath, uOff, vOff, uScale, vScale)` |
| Flat unlit solid-colour material | `Entity::SetMaterialColor(Color)` |
| Billboard UV-region crop (sprite sheet) | `Entity::SetBillboardUVRect(x, y, w, h)` |
| Fog colour + range | `Game::SetFogEnabled(bool)`, `SetFogColor(Color)`, `SetFogRange(start, end)` |
| Ambient scene light colour | `Game::SetAmbientColor(Color)` |
| Camera shake | `Camera::Shake(intensity, duration)` |
| UI Image UV crop (sprite sheet slice) | `UI::Image::SetImageRect(x, y, w, h)` |
| UI gauge / progress bar | `UI::ProgressBar` via `CreateProgressBar()` |
| Full-screen hit-flash overlay | `UI::Panel` with `SetColor` + `SetOpacity` + `FadeIn/FadeOut` |
| Particle explosion | `Entity::AddParticleEmitter(xmlPath)` |
| Looped 3D audio source | `Entity::AddAudioSource()` |
| 3D audio listener | `Game::SetAudioListener(entity)` |
| Save data | `Game::GetSaveData(slot)` → `SaveData::Get/Set/Load/Save` |
| Input actions (keyboard + gamepad unified) | `BindAction / BindAxis2D` |
| Trigger volumes | `Entity::AddTriggerSphere/Box/Capsule` + `SetOnTriggerEnter` |
| Character controller | `Entity::AddCharacterController` → `CharacterController` |
| Orbit camera | `Camera::SetOrbitMode`, `SetOrbitAngles`, `SetOrbitPitchLimits` |
| Camera collision avoidance | `Camera::SetCollisionEnabled(true)` |
| Scene fade transitions | `Game::FadeOut(duration, cb)`, `FadeIn(duration, cb)` |
| Sky dome / sky sphere | `Game::SetSkyDome(texPath)`, `SetSkyGradient(zenith, horizon)`, `SetSkyDomeEnabled(bool)` |
| Per-channel audio (93 indexed channels) | `Game::PlaySound(path, volume, channel)`, `StopSound(channel)`, `SetChannelVolume(channel, volume)`, `IsChannelPlaying(channel)` |
