#include "Editor/GEEditCommandStack.hpp"
#include "Editor/GEVoxelRaycast.hpp"
#include "Editor/GEWorldEditor.hpp"
#include "Game/GEObjectVerticalPlacement.hpp"

#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cmath>
#include <iostream>

int main()
{
    using namespace GalaxyEggbert::CNA;
    using GalaxyEggbert::CollectMoveObjects;
    using GalaxyEggbert::MoveObjectRecord;
    using GalaxyEggbert::PlaceMoveObject;
    using GalaxyEggbert::Worlds::Block;
    using GalaxyEggbert::Worlds::World;
    using Microsoft::Xna::Framework::Input::ButtonState;
    using Microsoft::Xna::Framework::Input::KeyboardState;
    using Microsoft::Xna::Framework::Input::Keys;
    using Microsoft::Xna::Framework::Input::MouseState;

    bool allOk = true;
    const auto check = [&allOk](bool condition, const char* message)
    {
        std::cout << (condition ? "PASS" : "FAIL") << ": " << message << std::endl;
        if (!condition) allOk = false;
    };

    check(ObjectVisualCenterY(7.0f) == 7.0f,
          "an object's stored Y is its visual cell center without another block offset");
    check(LiftRiderCenterY(7.0f) == 8.0f,
          "a rider stands one cell-center unit above a lift cube");

    const auto makeRecord = [](GalaxyEggbert::Def::ObjectType type, float x, float y, float z)
    {
        MoveObjectRecord record;
        record.type = type;
        record.posStartX = x;
        record.posStartY = y;
        record.posStartZ = z;
        record.posEndX = x;
        record.posEndY = y;
        record.posEndZ = z;
        return record;
    };

    {
        World world;
        GEEditCommandStack stack;
        const auto placed =
            makeRecord(GalaxyEggbert::Def::ObjectType::ObjectType6, 12.0f, 3.0f, 20.0f);
        PlaceMoveObject(world, placed);

        GEEditCommand command;
        command.kind = GEEditCommand::Kind::MoveObjectEdit;
        command.objectAnchorX = 12;
        command.objectAnchorY = 3;
        command.objectAnchorZ = 20;
        command.objectAfter = placed;
        stack.Push(command);

        check(stack.Undo(world) && CollectMoveObjects(world).empty(),
              "undo removes a newly placed object");
        check(stack.Redo(world) &&
                  CollectMoveObjects(world).front().type ==
                      GalaxyEggbert::Def::ObjectType::ObjectType6,
              "redo restores the placed object");

        const auto replacement =
            makeRecord(GalaxyEggbert::Def::ObjectType::ObjectType44, 12.0f, 3.0f, 20.0f);
        GEEditCommand overwrite;
        overwrite.kind = GEEditCommand::Kind::MoveObjectEdit;
        overwrite.objectAnchorX = 12;
        overwrite.objectAnchorY = 3;
        overwrite.objectAnchorZ = 20;
        overwrite.objectBefore = placed;
        overwrite.objectAfter = replacement;
        PlaceMoveObject(world, replacement);
        stack.Push(overwrite);

        check(stack.Undo(world) &&
                  CollectMoveObjects(world).front().type ==
                      GalaxyEggbert::Def::ObjectType::ObjectType6,
              "undoing an overwrite restores the previous object");
        check(stack.Redo(world) &&
                  CollectMoveObjects(world).front().type ==
                      GalaxyEggbert::Def::ObjectType::ObjectType44,
              "redoing an overwrite restores the replacement");
    }

    World world;
    for (std::uint16_t z = 0; z < 100; ++z)
    {
        for (std::uint16_t y = 0; y < 4; ++y)
        {
            world.setBlock(50, y, z, Block::make(1));
        }
    }

    constexpr float kDefaultPitch = -0.35f;
    const float cosPitch = std::cos(kDefaultPitch);
    const RaycastHit expected =
        Raycast(world, 50.0f, 10.0f, 50.0f, 0.0f, std::sin(kDefaultPitch),
                -cosPitch, 200.0f);
    check(expected.hit, "the object test camera ray hits its reference slab");
    const float placeX = static_cast<float>(static_cast<int>(expected.x) + expected.normalX);
    const float placeY = static_cast<float>(static_cast<int>(expected.y) + expected.normalY);
    const float placeZ = static_cast<float>(static_cast<int>(expected.z) + expected.normalZ);

    GEWorldEditor editor;
    editor.EnterEditing(0.0f, 10.0f, 0.0f);
    Easy3D::Camera3D camera;
    const MouseState restMouse(
        0, 0, 0, ButtonState::Released, ButtonState::Released,
        ButtonState::Released, ButtonState::Released, ButtonState::Released);

    const auto click = [&](int x, int y)
    {
        const MouseState down(
            x, y, 0, ButtonState::Pressed, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        const MouseState up(
            x, y, 0, ButtonState::Released, ButtonState::Released,
            ButtonState::Released, ButtonState::Released, ButtonState::Released);
        editor.Update(KeyboardState{}, down, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, up, 0.0f, 800, 480, camera, world);
    };
    const auto pressKey = [&](Keys key)
    {
        editor.Update(KeyboardState{key}, restMouse, 0.0f, 800, 480, camera, world);
        editor.Update(KeyboardState{}, restMouse, 0.0f, 800, 480, camera, world);
    };

    click(30, 324);
    click(72, 324);
    check(CollectMoveObjects(world).empty(),
          "object palette clicks are consumed without placing into the world");

    click(400, 200);
    auto objects = CollectMoveObjects(world);
    check(objects.size() == 1 &&
              objects[0].type == GalaxyEggbert::Def::ObjectType::ObjectType5,
          "a 3D-view click places the selected chest object");
    check(objects.size() == 1 &&
              objects[0].posStartX == placeX &&
              objects[0].posStartY == placeY &&
              objects[0].posStartZ == placeZ,
          "the object is placed in the preview cell adjacent to the hit face");
    check(objects.size() == 1 &&
              objects[0].posEndX == objects[0].posStartX &&
              objects[0].posEndY == objects[0].posStartY &&
              objects[0].posEndZ == objects[0].posStartZ,
          "a freshly placed object starts stationary");
    check(editor.ConsumeNeedsPresentationRebuild(),
          "object placement requests a presentation rebuild");

    pressKey(Keys::U);
    check(CollectMoveObjects(world).empty(), "U undoes object placement");
    check(editor.ConsumeNeedsPresentationRebuild(),
          "undoing object placement requests a presentation rebuild");
    pressKey(Keys::R);
    check(CollectMoveObjects(world).size() == 1, "R redoes object placement");
    check(editor.ConsumeNeedsPresentationRebuild(),
          "redoing object placement requests a presentation rebuild");

    click(30, 30);
    check(CollectMoveObjects(world).empty(),
          "the visible delete glyph removes an object in the red preview cell");
    check(editor.ConsumeNeedsPresentationRebuild(),
          "toolbar object removal requests a presentation rebuild");
    pressKey(Keys::U);
    check(CollectMoveObjects(world).size() == 1,
          "undo restores an object removed through the toolbar");
    check(editor.ConsumeNeedsPresentationRebuild(),
          "undoing toolbar object removal requests a presentation rebuild");
    pressKey(Keys::R);
    check(CollectMoveObjects(world).empty(),
          "redo repeats object removal through the toolbar");
    pressKey(Keys::U);
    check(CollectMoveObjects(world).size() == 1,
          "a second undo restores the toolbar-deleted object again");
    (void)editor.ConsumeNeedsPresentationRebuild();

    pressKey(Keys::G);
    check(!editor.ConsumeNeedsPresentationRebuild(),
          "selecting an object does not mutate the presentation");
    pressKey(Keys::T);
    objects = CollectMoveObjects(world);
    check(editor.ConsumeNeedsPresentationRebuild() &&
              objects.size() == 1 &&
              (objects[0].posEndX != objects[0].posStartX ||
               objects[0].posEndY != objects[0].posStartY ||
               objects[0].posEndZ != objects[0].posStartZ),
          "T assigns the selected object a patrol target");

    const float speedBefore = objects.front().speed;
    pressKey(Keys::OemPlus);
    check(CollectMoveObjects(world).front().speed > speedBefore,
          "plus increases the selected object's active numeric field");
    pressKey(Keys::OemMinus);
    check(std::fabs(CollectMoveObjects(world).front().speed - speedBefore) < 0.0001f,
          "minus restores the previous speed");

    pressKey(Keys::Tab);
    const float stepAdvanceBefore =
        CollectMoveObjects(world).front().stepAdvanceTicks;
    pressKey(Keys::OemPlus);
    check(CollectMoveObjects(world).front().stepAdvanceTicks > stepAdvanceBefore,
          "Tab changes which object field plus edits");

    click(30, 30);
    check(CollectMoveObjects(world).empty(),
          "the visible delete glyph removes the explicitly selected object");
    pressKey(Keys::U);
    check(CollectMoveObjects(world).size() == 1,
          "undo restores a toolbar-deleted object with its edited record");

    pressKey(Keys::G);
    pressKey(Keys::Delete);
    check(CollectMoveObjects(world).empty(),
          "the Delete key still removes the selected object");
    pressKey(Keys::U);
    check(CollectMoveObjects(world).size() == 1,
          "undo still restores an object deleted from the keyboard");

    {
        World emptyWorld;
        GEWorldEditor emptyEditor;
        emptyEditor.EnterEditing(0.0f, 10.0f, 0.0f);
        Easy3D::Camera3D emptyCamera;
        emptyEditor.Update(
            KeyboardState{Keys::G}, restMouse, 0.0f, 800, 480,
            emptyCamera, emptyWorld);
        emptyEditor.Update(
            KeyboardState{Keys::Delete}, restMouse, 0.0f, 800, 480,
            emptyCamera, emptyWorld);
        check(CollectMoveObjects(emptyWorld).empty(),
              "selection and deletion are safe with no objects present");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
