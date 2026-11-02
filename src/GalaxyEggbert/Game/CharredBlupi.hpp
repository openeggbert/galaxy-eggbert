#pragma once

// The charred Blupi of the Clear1 death (lightning, mines, dynamite) and of Clear3's first half
// (lava), drawn as his own model (owner 2026-11-02: no billboard of him). The original draws these
// frames from element.png, icons 40-47 (Decor.cpp:2403-2410; BlupiController's kClear1Frames and
// kClear3Frames): 40 and 41 Blupi burnt black standing (40 with his face still pale, 41 with his
// eyes shut), 42-44 crumbling from the head down, 45 a flat heap of ash, 46/47 the heap smoking.
// The model's face is painted on its body's texture, so the whole of him is black (40's pale face
// is not drawn).
// No Ray Dream scene of the charred Blupi exists, so the host draws his Stop model tinted black
// and shrinks it toward his feet to each frame's height, spreading it out as the heap; on 46/47 it
// draws the sprite's smoke above the heap.
// The heights and widths are the sprites' own: the rows and columns of their dark body against
// the standing 40's (46 rows, 12-57; 37 columns, 11-47).

namespace GalaxyEggbert::Game
{
    struct CharredShape
    {
        bool charred = false;   // false: not one of the charred icons
        float height = 1.0f;    // the model's height, a share of his standing height
        float spread = 1.0f;    // its width and depth, a share of his own
        bool smokes = false;    // the heap smoking (46, 47): the sprite's smoke over it
    };

    // The shape for an element.png icon of his death frames; not charred for any other icon.
    [[nodiscard]] constexpr CharredShape CharredModelShape(int elementIcon) noexcept
    {
        switch (elementIcon)
        {
        case 40:
        case 41: return {true, 1.0f, 1.0f, false};
        case 42: return {true, 0.80f, 1.0f, false};  // rows 21-57
        case 43: return {true, 0.67f, 1.0f, false};  // rows 27-57
        case 44: return {true, 0.43f, 1.0f, false};  // rows 38-57
        case 45: return {true, 0.20f, 1.22f, false}; // rows 50-58, columns 7-51
        case 46:
        case 47: return {true, 0.24f, 1.27f, true};  // rows 48-59, columns 7-53
        default: return {};
        }
    }

    // The smoke of 46/47: the icon's rows above the heap (0-46 of its 60), drawn as a billboard
    // whose bottom edge stands this high over his feet (the icon is drawn with its bottom on them).
    inline constexpr int kCharredSmokeRows = 47;
    inline constexpr float kCharredSmokeBottomPixels = 60.0f - static_cast<float>(kCharredSmokeRows);
}
