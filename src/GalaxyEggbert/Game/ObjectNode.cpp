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
    bb->size_     = Vector2(1.0f, 1.0f);
    bb->enabled_  = true;
    UpdateIcon(0);
}

ObjectNode::~ObjectNode() { Remove(); }

void ObjectNode::SetPosition(Vector3 pos) {
    if (node_) node_->SetPosition(pos);
}

void ObjectNode::UpdateIcon(int icon) {
    if (!sprite_) return;
    int   col = icon % kCols;
    int   row = icon / kCols;
    float u0  = col * kTile / kSheetW;
    float v0  = row * kTile / kSheetH;
    Billboard* bb = sprite_->GetBillboard(0);
    bb->uv_ = Rect(u0, v0, u0 + kTile / kSheetW, v0 + kTile / kSheetH);
    sprite_->Commit();
}

void ObjectNode::SetVisible(bool visible) {
    if (node_) node_->SetEnabled(visible);
}

void ObjectNode::Remove() {
    if (node_) { node_->Remove(); node_ = nullptr; sprite_ = nullptr; }
}
