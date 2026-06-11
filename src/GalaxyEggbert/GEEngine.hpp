#pragma once

// Single include point for the 3D engine headers.
// Both U3D and Nova3D expose the Urho3D namespace but organise headers differently.
#ifdef GE_ENGINE_U3D
#include <Urho3D/Urho3DAll.h>
#else
#include <Urho3D/Urho3D.h>
#endif
