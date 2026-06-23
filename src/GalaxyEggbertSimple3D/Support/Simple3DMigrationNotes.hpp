#pragma once

// Migration notes for the galaxy-eggbert Simple3D port.
//
// Old Urho3D source tree: src/GalaxyEggbert/
// New Simple3D source tree: src/GalaxyEggbertSimple3D/
//
// Correspondence map:
//
//  Old (Urho3D)                  New (Simple3D)
//  ─────────────────────────────────────────────────────────────────
//  GalaxyEggbertApp              GalaxyEggbertSimpleGame (Simple3D::Game subclass)
//  GalaxyEggbertGame             GalaxyEggbertSimpleGame (merged)
//  Blupi                         GEBlupiController
//  Decor                         GEDecorSystem
//  CameraController              GECameraRig
//  HUD                           GEHud
//  SoundManager                  GESound
//  GalaxyEggbertGame terrain     GETerrainRenderer
//  GalaxyEggbertGame world load  GEWorldRuntime
//
// Engine types that MUST NOT appear in the new Simple3D source files:
//  Urho3D::Context               — removed; Simple3D::Game owns the engine
//  Urho3D::Scene / Node          — replaced by Simple3D::Entity
//  Urho3D::Material / Texture2D  — replaced by entity model paths (S3D-2)
//  Urho3D::BillboardSet          — replaced by Entity::AddBillboard (S3D-3)
//  Urho3D::Text                  — replaced by Simple3D::Label
//  Urho3D::SoundSource           — replaced by Game::PlaySound
//
// Engine types that ARE IDENTICAL between Urho3D and Simple3D via alias:
//  Simple3D::Vector3  ==  Urho3D::Vector3   (Types.h re-exports)
//  Simple3D::Color    ==  Urho3D::Color
//  Simple3D::Quaternion == Urho3D::Quaternion
//  → Always use the Simple3D:: namespace in new code.
//
// Engine-independent shared code (reused verbatim in both targets):
//  include/GalaxyEggbert/Worlds/   — pure C++ voxel data model
//  include/GalaxyEggbert/def/      — game enums (ObjectType, GamePhase, …)
//  include/GalaxyEggbert/BlockTypes.hpp — tile ID logic
//  src/GalaxyEggbert/Game/Tables.cpp   — animation frame tables
