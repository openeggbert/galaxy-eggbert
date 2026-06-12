#pragma once
#include "../GEEngine.hpp"

// Urho3D scene node for one game object — billboard UV-mapped from element.png.
// element.png: 600x1740 px, 64x64 tiles, 9 cols x 27 rows, 243 icons.
class ObjectNode {
public:
    ObjectNode(Urho3D::Context* context, Urho3D::Scene* scene);
    ~ObjectNode();

    void SetPosition(Urho3D::Vector3 pos);
    void UpdateIcon(int icon);
    void Remove();

private:
    Urho3D::Context*      context_;
    Urho3D::Node*         node_   = nullptr;
    Urho3D::BillboardSet* sprite_ = nullptr;

    static constexpr float kSheetW = 600.0f;
    static constexpr float kSheetH = 1740.0f;
    static constexpr float kTile   =  64.0f;
    static constexpr int   kCols   =   9;
};
