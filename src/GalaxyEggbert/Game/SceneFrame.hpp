#pragma once

// SceneFrame -- what the 3D world draws this frame (renderers.md, the seam between the simulation and
// the renderer). GalaxyEggbertGame builds it from the simulation's state (GalaxyEggbertGameScene.cpp:
// which objects, which icon, which model and pose, where), and WorldRenderer draws only from it and
// from the loaded assets (WorldRenderer.hpp: how -- effects, meshes, device state, and the optional
// effects layer). The lists keep the order the world has always been drawn in: what is drawn first
// stays first, so blending and depth come out the same.
//
// No GraphicsDevice, effect or GPU mesh here: geometry is XNA math and cna-engine's CPU-side texture
// rectangles and box faces; the loaded models and textures it names are only pointed at (forward
// declared), never touched.

#include <GalaxyEggbert/Def/ObjectType.hpp>
#include <GalaxyEggbert/Game/ProceduralMaterial.hpp>

#include <CnaEngine/Graphics/MeshBuilder.hpp>
#include <CnaEngine/Graphics/TextureAtlas.hpp>

#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Microsoft::Xna::Framework::Graphics
{
    class SkinnedModelEXT;
    class Texture2D;
}

namespace GalaxyEggbert::Game
{
    struct SceneCamera
    {
        Microsoft::Xna::Framework::Matrix view;
        Microsoft::Xna::Framework::Matrix projection;
        Microsoft::Xna::Framework::Vector3 position;
        Microsoft::Xna::Framework::Vector3 target; // the chase camera's look-at point
        float fieldOfView = 0.0f;                  // vertical, radians
        float aspectRatio = 1.0f;
    };

    // A camera-facing (or turned) quad: centre, size in blocks, its right and up vectors, the texture
    // rectangle. `source` names the MoveObject it shows, if any (the effects layer's particles).
    struct SceneQuad
    {
        Microsoft::Xna::Framework::Vector3 center;
        Microsoft::Xna::Framework::Vector2 size;
        Microsoft::Xna::Framework::Vector3 right;
        Microsoft::Xna::Framework::Vector3 up;
        CnaEngine::Graphics::UvRect uv;
        int sourceObject = -1;                     // index in WorldRuntime::GetMobileObjects(), or -1
        int sourceIcon = -1;                       // its icon on its sheet
        float rotation = 0.0f;                     // radians in the view plane (particles)
        Microsoft::Xna::Framework::Color color = Microsoft::Xna::Framework::Color::White; // particles: tint, alpha
    };

    // The sheets a billboard batch is cut from: each its own texture and effect.
    enum class SceneSheet : std::uint8_t
    {
        Element,  // element.png
        ObjectM,  // object-m.png (the terrain's sheet)
        Explo,    // explo.png, the explosion channel's objects
        Blupi,    // blupi.png
        Blupi1,   // blupi1.png
        BigDecor, // explo.png, the big decor layer
    };

    struct SceneBillboardBatch
    {
        SceneSheet sheet = SceneSheet::Element;
        float alpha = 1.0f;
        std::vector<SceneQuad> quads;
    };

    // A cube of the cube-object pass (lifts, crates: SolidCube MoveObjects), in the world as it is.
    struct SceneCube
    {
        Microsoft::Xna::Framework::Vector3 center;
        Microsoft::Xna::Framework::Vector3 size;
        CnaEngine::Graphics::BoxFaces faces;
    };

    // A crate flying away (ByeByePiece): a unit cube in its own world transform.
    struct SceneCubePiece
    {
        Microsoft::Xna::Framework::Matrix world;
        CnaEngine::Graphics::UvRect uv;
    };

    enum class SceneCull : std::uint8_t
    {
        CounterClockwise, // XNA's default
        Clockwise,        // a mirrored model (its winding turned round)
    };

    // One skinned model posed and drawn: Blupi, the vehicles, the enemy Blupis, the editor's player
    // starts, every object, decor and terrain model.
    struct SceneModel
    {
        const Microsoft::Xna::Framework::Graphics::SkinnedModelEXT* model = nullptr;
        std::string clip;
        double seconds = 0.0;
        bool loop = true;
        Microsoft::Xna::Framework::Matrix world;
        SceneCull cull = SceneCull::CounterClockwise;
        float alpha = 1.0f;
        bool translucent = false;          // alpha blended over the scene, depth read but not written
        bool tinted = false;               // the charred or glued colour below, without sheen
        Microsoft::Xna::Framework::Vector3 tint;
        // Each part's texture when not its own (a face, the enemy's or a start's recoloured copies):
        // empty, or one entry per part, nullptr keeping the part's own.
        std::vector<Microsoft::Xna::Framework::Graphics::Texture2D*> partTextures;
        float sawSeconds = -1.0f;          // >= 0: the saw's parts shown at this pose time only
        bool boothFrontOnly = false;       // a teleporter's other sides: its plate and lights only
        bool editorSelected = false;       // the editor's red overlay over it
        float cullRadius = -1.0f;          // >= 0: skipped when this sphere round `cullCenter` is out of view
        Microsoft::Xna::Framework::Vector3 cullCenter;
        bool counted = false;              // counted in the perf statistics (object models)
        // Each part's Ray Dream material (ProceduralMaterial.hpp), for the effects' model shader: one
        // per part, or nullptr (none).
        const std::vector<ProceduralMaterial>* partMaterials = nullptr;
    };

    // A textured quad set drawn with one texture (the signs' painted faces, the sparks).
    struct SceneDecal
    {
        Microsoft::Xna::Framework::Graphics::Texture2D* texture = nullptr;
        std::vector<SceneQuad> quads;
    };

    // The glue death's strands (GlueStrands.hpp): thin tubes on element.png, already placed.
    struct SceneGlueStrands
    {
        bool shown = false;
        CnaEngine::Graphics::MeshData mesh;
    };

    // A soft round shadow on the ground under something (the effects layer's sun shadows): the
    // point on the ground it falls on, its radius in blocks and how dark it is (0..1).
    struct SceneShadow
    {
        Microsoft::Xna::Framework::Vector3 center;
        float radius = 0.5f;
        float strength = 0.5f;
        bool model = false;      // thrown by a 3D model: the shadow map draws its real shadow instead
    };

    // A light something bright throws on what is near it (the effects' lava, explosions, lightning).
    struct ScenePointLight
    {
        Microsoft::Xna::Framework::Vector3 position;
        Microsoft::Xna::Framework::Vector3 color;  // its strength in it (1 = a full white light)
        float radius = 4.0f;                        // blocks: nothing beyond
    };

    // The 3D effects (the Setup's "3D effects", SaveData::GetEffects3D): presentation only. Off, the
    // world is drawn exactly as without them.
    struct SceneEffects
    {
        bool enabled = false;    // any level: fog, the soft round shadows, the particles
        int level = 0;           // 1 low, 2 medium, 3 high, 4 ultra (SaveData::GetEffectsLevel)
        bool occlusion = false;  // medium: the terrain's ambient occlusion
        bool surfaces = false;   // medium: the water and lava shaders
        bool smoothing = false;  // medium: FXAA
        bool shadowMap = false;  // high: the sun's shadow map
        bool pointLights = false; // high: the point lights
        bool glow = false;       // high: bloom and the grade
        bool ultra = false;      // ultra: the world at the screen's own resolution, in HDR (UltraFinish.hpp)
        Microsoft::Xna::Framework::Vector3 fogColor;
        float fogStart = 0.0f;   // blocks from the camera where the fog begins
        float fogEnd = 1.0f;     // where it is whole
        float seconds = 0.0f;    // the effects' clock (the simulation's ticks / 60): water, lava, glow
        std::vector<SceneShadow> shadows;
        std::vector<ScenePointLight> lights;
        std::vector<SceneBillboardBatch> particles; // the explosions, splashes and sparks
    };

    struct SceneFrame
    {
        SceneCamera camera;
        bool world = false;                // the 3D world is drawn (not the menus' own screens)
        bool terrain = false;              // the terrain and everything drawn with it

        // The terrain's reliefs round the camera's target (TerrainRelief.hpp).
        Microsoft::Xna::Framework::Vector3 reliefTarget;
        float reliefRadius = 0.0f;
        int reliefBudget = -1;             // chunks built this frame, -1 all (the first frame)
        bool reportReliefTiming = false;

        std::vector<SceneCube> cubes;
        std::vector<SceneCubePiece> cubePieces;

        // In drawing order: Blupi (unless translucent), the parked and enemy vehicles, then every
        // object model; then the translucent Blupi, then the editor's player starts.
        std::vector<SceneModel> opaqueModels;
        int vehiclePassStart = -1;         // where in opaqueModels the parked vehicles' pass begins, if it runs
        std::vector<SceneModel> objectModels;
        bool objectModelPass = false;      // the object model pass runs (its closing cull state)
        bool signDecalPass = false;        // the sign pass runs (its states)
        std::vector<SceneDecal> signDecals;
        SceneGlueStrands glueStrands;
        std::vector<SceneDecal> electrodeSparks;
        std::vector<SceneDecal> dynamiteSparks;
        std::vector<SceneModel> translucentModels;
        std::vector<SceneModel> editorStarts;

        // The billboards after the terrain's translucent pass, batch by batch.
        std::vector<SceneBillboardBatch> billboards;
        // The big decor cells drawn as the marine animation (on object-m.png, its own pass).
        std::vector<SceneQuad> marine;

        SceneEffects effects;

        void Clear()
        {
            world = terrain = false;
            reportReliefTiming = false;
            cubes.clear();
            cubePieces.clear();
            opaqueModels.clear();
            vehiclePassStart = -1;
            objectModels.clear();
            objectModelPass = signDecalPass = false;
            signDecals.clear();
            glueStrands = {};
            electrodeSparks.clear();
            dynamiteSparks.clear();
            translucentModels.clear();
            editorStarts.clear();
            billboards.clear();
            marine.clear();
            effects.enabled = false;
            effects.level = 0;
            effects.occlusion = effects.surfaces = effects.smoothing = false;
            effects.shadowMap = effects.pointLights = effects.glow = false;
            effects.shadows.clear();
            effects.lights.clear();
            effects.particles.clear();
        }
    };
}
