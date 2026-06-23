# Simple3D API Gaps — Galaxy Eggbert

Features required by galaxy-eggbert that are missing from the Simple3D API as of 2026-06-23.

These are documented here instead of being hacked with direct Urho3D calls in the Simple3D port code.

| Feature needed by Galaxy Eggbert | Currently in Simple3D? | Temporary workaround | Proposed Simple3D API |
|---|---|---|---|
| Tile atlas material with per-tile UV offset | No | Grey Box.mdl cubes (S3D-1) | `Entity::SetMaterialUVOffset(uOff, vOff, uScale, vScale)` or `Entity::SetAtlasMaterial(texPath, col, row, cols, rows)` |
| Billboard sprite with UV-region crop (sprite sheet) | Partial — `AddBillboard(path, size)` exists but no UV crop | Box.mdl placeholder cube (S3D-1) | `Entity::AddBillboard(texPath, uvRect, size)` |
| Sky dome / sky sphere | No | `SetClearColor()` per-world approximation | `Game::SetSkyDome(texPath)` or `Game::SetSkyColor(top, horizon, bottom)` |
| Fog colour + fog start/end range | No | `SetClearColor()` approximation | `Game::SetFogColor(Color)`, `Game::SetFogRange(start, end)` |
| Ambient zone (world-space ambient light colour) | No | Sun directional light approximation | `Game::SetAmbientColor(Color)` |
| UI Image with UV-region crop (sprite sheet slice) | `UI::Image` exists but no UV crop | Text-only HUD (S3D-1) | `UI::Image::SetImageRect(IntRect)` |
| UI gauge / life-icon sprite strip | No | Text-only HUD (S3D-1) | `UI::ProgressBar` + `UI::Image` composition, or dedicated `UI::SpriteStrip` |
| Pixel-positioned hit-flash full-screen overlay | No | Skip in S3D-1 | `UI::Panel::SetFullscreen()` with alpha fade |
| Particle explosion from sprite sheet frames | Partial — `AddParticleEmitter(xmlPath)` exists, but requires `explo.png` strip frames | Skip in S3D-1 | `AddParticleEmitter` with custom sprite-sheet XML, or `Entity::AddBillboardAnimation(texPath, cols, rows, fps)` |
| Per-channel audio volume (93 channels) | No — only `Game::PlaySound(path, volume)` | Map 5 critical sounds only (S3D-1) | `AudioSource::SetVolume()` per entity source, or `Game::PlaySound(path, volume, channel)` with channel priority |
| Looped sound effects (e.g. lava hiss, engine rumble) | Partial — `Entity::AddAudioSource` can loop | Not wired yet in S3D-1 | Wire `Entity::AddAudioSource()` per world hazard entity |
| Camera shake | No | `GECameraRig::StartShake` is a no-op stub | `Camera::Shake(intensity, duration)` |
| Input actions (keyboard + gamepad unified) | **Yes** — `BindAction / BindAxis2D` available | Already used in S3D-1 | ✅ Already available |
| Save data (binary GameData blob) | Partial — `Simple3D::SaveData` exists (JSON) | Not wired in S3D-1 | Port GameData to `SaveData` JSON, or `SaveData::SaveRaw()` for binary |
| 3D audio listener (positional sound) | Yes — `Game::SetAudioListener(entity)` | Not wired in S3D-1 | Wire to Blupi entity in Start() |
| Trigger volumes (collect pickup on contact) | **Yes** — `AddTriggerSphere` / `SetOnTriggerEnter` | Already used in S3D-1 | ✅ Already available |
| Character controller (platformer) | **Yes** — `AddCharacterController` | Already used in S3D-1 | ✅ Already available |

## What IS available in Simple3D (and used in S3D-1)

- `Game` subclass with `Start / Update / Stop`
- `CreateEntity`, `CreateCamera`, `CreateLabel`
- `Entity::AddModel`, `SetPosition`, `SetScale`, `SetRotation`
- `Entity::AddRigidBody`, `AddBoxCollider`, `AddCapsuleCollider`
- `Entity::AddTriggerSphere`, `SetOnTriggerEnter`
- `Entity::AddCharacterController` → `CharacterController::Move / TryJump / IsOnGround`
- `Entity::CreateChild`
- `Entity::SetCollisionLayer(CollisionLayer)`, `SetCollisionMask`
- `CollisionLayer::Actor / StaticGeometry / Trigger`
- `MakeCollisionMask({...})`
- `Camera::SetOrbitMode`, `SetOrbitAngles`, `SetOrbitPitchLimits`, `SetOrbitSensitivity`
- `Camera::SetCollisionEnabled`, `SetFOV`, `SetFarClip`
- `Label::SetText / SetPosition / SetFontSize / SetColor / SetVisible`
- `Game::PlaySound`, `PlayMusic`, `SetMasterVolume`
- `Game::IsKeyDown`, `IsKeyPressed`
- `Game::IsGamepadButtonDown`, `IsGamepadButtonPressed`, `GetGamepadAxis`
- `Game::BindAction`, `BindAxis2D`, `IsActionDown`, `IsActionPressed`, `GetAxis2D`
- `Game::SetClearColor`, `SetWindowTitle`, `SetWindowSize`, `SetAppName`
- `Game::SetResourcePrefixPaths`
- `Game::DestroyEntity`, `FindEntity`, `IsPendingDestroy`
- `Entity::AddDirectionalLight`
