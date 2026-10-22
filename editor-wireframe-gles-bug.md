# Editor highlight wireframe on OpenGL ES contexts (unresolved, 2026-10-20)

Found by cna-engine's agent on 2026-10-20 while driving the editor migrated to
cna-engine under Xvfb with the native default renderer (`OPENGLES3`); recorded
in `upstream-issues.md` of [libcna/cna-engine](https://github.com/libcna/cna-engine)
as UP-014. Independent of the easy-3d migration. Still present on `develop` at
`924a847` (2026-10-22).

`src/GalaxyEggbert/Editor/EditorHighlightRenderer.cpp` (`Draw`, around line 121)
draws the highlight box's edges with `FillMode::WireFrame` without asking
`GraphicsDevice::SupportsCapability(GraphicsCapability::WireFrame)`. On an OpenGL
ES context without `GL_NV_polygon_mode`, CNA's EasyGL renderer can only emulate
wireframe for a Position+Normal mesh drawn with a stock effect; the highlight
box is Position+Texture (as it was with easy-3d's `CubeMeshRenderer`), so CNA
throws `System::NotSupportedException` and the game terminates as soon as the
editor shows a placement preview.

Neither llvmpipe's nor the Radeon 780M's (radeonsi) OpenGL ES context on the
development machine offers `GL_NV_polygon_mode`, so the native OpenGL ES 3
build's editor cannot be used there. The OpenGL 3.3 build (the verified
configuration, `build-cna-opengl3`) has native polygon mode and is unaffected.

Suggested fix: when the capability is missing, draw the edges with
`FillMode::Solid` as thin boxes, or as a line list (cna-engine's
`CnaEngine::Graphics::DebugDraw` draws box outlines as lines and works on every
context).
