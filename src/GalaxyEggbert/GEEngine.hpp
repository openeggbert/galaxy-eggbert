#pragma once
// Complete Bullet types before Urho3DAll.h so Ptr.h sizeof checks pass on
// Clang 19 (Emscripten). These headers are on URHO3D_INCLUDE_DIRS ThirdParty path.
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btTriangleInfoMap.h>
#include <Urho3D/Urho3DAll.h>
