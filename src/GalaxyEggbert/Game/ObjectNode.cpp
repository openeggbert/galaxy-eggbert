#include "ObjectNode.hpp"

using namespace Urho3D;

ObjectNode::ObjectNode(Context* context, Scene* scene) : context_(context) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    node_   = scene->CreateChild("Object");
    sprite_ = node_->CreateComponent<BillboardSet>();
    sprite_->SetNumBillboards(1);
    sprite_->SetFaceCameraMode(FC_ROTATE_XYZ);

    auto* tex  = cache->GetResource<Texture2D>("icons/element.png");
    auto* tech = cache->GetResource<Technique>("Techniques/DiffAlpha.xml");
    SharedPtr<Material> mat(new Material(context_));
    if (tech) mat->SetTechnique(0, tech);
    if (tex)  mat->SetTexture(TU_DIFFUSE, tex);
    mat->SetShaderParameter("MatDiffColor", Color(1.0f, 1.0f, 1.0f, 1.0f));
    mat->SetShaderParameter("MatSpecColor",  Color(0.0f, 0.0f, 0.0f, 0.0f));
    sprite_->SetMaterial(mat);

    Billboard* bb = sprite_->GetBillboard(0);
    bb->position_ = Vector3::ZERO;
    // 60×60 px tile in a 64-px grid → visual size = 60/64 units (matches Blupi scale).
    bb->size_     = Vector2(60.0f / 64.0f, 60.0f / 64.0f);
    bb->enabled_  = true;
    UpdateIcon(0);
}

ObjectNode::~ObjectNode() { Remove(); }

void ObjectNode::SetPosition(Vector3 pos) {
    if (node_) node_->SetPosition(pos);
}

void ObjectNode::UpdateIcon(int icon, bool flipX) {
    if (!sprite_) return;
    int   col = icon % kCols;
    int   row = icon / kCols;
    float u0  = col * kTile / kSheetW;
    float v0  = row * kTile / kSheetH;
    float u1  = u0 + kTile / kSheetW;
    float v1  = v0 + kTile / kSheetH;
    Billboard* bb = sprite_->GetBillboard(0);
    // Sprites in element.png face left; flip U when moving right.
    bb->uv_ = flipX ? Rect(u1, v0, u0, v1) : Rect(u0, v0, u1, v1);
    sprite_->Commit();
}

void ObjectNode::SetVisible(bool visible) {
    if (node_) node_->SetEnabled(visible);
}

void ObjectNode::Remove() {
    if (node_) { node_->Remove(); node_ = nullptr; sprite_ = nullptr; }
}
