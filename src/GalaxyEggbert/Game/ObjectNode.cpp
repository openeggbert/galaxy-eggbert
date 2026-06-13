#include "ObjectNode.hpp"

using namespace Urho3D;

ObjectNode::ObjectNode(Context* context, Scene* scene) : context_(context) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    node_   = scene->CreateChild("Object");
    sprite_ = node_->CreateComponent<BillboardSet>();
    sprite_->SetNumBillboards(1);
    sprite_->SetFaceCameraMode(FC_ROTATE_Y);

    auto* tex  = cache->GetResource<Texture2D>("icons/element.png");
    auto* tech = cache->GetResource<Technique>("Techniques/DiffAlpha.xml");
    SharedPtr<Material> mat(new Material(context_));
    if (tech) mat->SetTechnique(0, tech);
    if (tex)  mat->SetTexture(TU_DIFFUSE, tex);
    mat->SetShaderParameter("MatDiffColor", Color(1.0f, 1.0f, 1.0f, 1.0f));
    mat->SetShaderParameter("MatSpecColor",  Color(0.0f, 0.0f, 0.0f, 0.0f));
    sprite_->SetMaterial(mat);

    Billboard* bb = sprite_->GetBillboard(0);
    // Objects placed at node Y=1.0 (one unit above block center at Y=0).
    // Block top = Y+0.5. For sprite bottom to land at block top:
    //   offset = blockTop + visHalf - nodeY = 0.5 + 60/128 - 1.0 = -1/32 ≈ -0.031
    static constexpr float kVisHalf = 60.0f / 64.0f / 2.0f;
    bb->position_ = Vector3(0.0f, kVisHalf - 0.5f, 0.0f); // ≈ -0.031 downward
    bb->size_     = Vector2(60.0f / 64.0f, 60.0f / 64.0f);
    bb->enabled_  = true;
    UpdateIcon(0);

    // Blob shadow: flat Box.mdl at ground surface below the object.
    auto* boxModel = cache->GetResource<Model>("Models/Box.mdl");
    auto* shadowTech = cache->GetResource<Technique>("Techniques/NoTexture.xml");
    shadowNode_ = scene->CreateChild("ObjShadow");
    auto* sm = shadowNode_->CreateComponent<StaticModel>();
    if (boxModel) sm->SetModel(boxModel);
    SharedPtr<Material> shadowMat(new Material(context_));
    if (shadowTech) shadowMat->SetTechnique(0, shadowTech);
    shadowMat->SetShaderParameter("MatDiffColor",     Color(0.05f, 0.05f, 0.05f, 1.0f));
    shadowMat->SetShaderParameter("MatEmissiveColor", Color(0.0f, 0.0f, 0.0f, 0.0f));
    sm->SetMaterial(shadowMat);
    shadowNode_->SetScale(Vector3(0.40f, 0.01f, 0.40f));
}

ObjectNode::~ObjectNode() { Remove(); }

void ObjectNode::SetPosition(Vector3 pos) {
    if (node_) node_->SetPosition(pos);
    // pos.y = blockTop + 0.5 for standing objects → shadow at pos.y - 0.48 = blockTop + 0.02
    if (shadowNode_) shadowNode_->SetPosition(Vector3(pos.x_, pos.y_ - 0.48f, pos.z_));
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
    if (node_)       node_->SetEnabled(visible);
    if (shadowNode_) shadowNode_->SetEnabled(visible);
}

void ObjectNode::SetShadowEnabled(bool enabled) {
    if (shadowNode_) shadowNode_->SetEnabled(enabled);
}

void ObjectNode::Remove() {
    if (shadowNode_) { shadowNode_->Remove(); shadowNode_ = nullptr; }
    if (node_)       { node_->Remove();       node_       = nullptr; sprite_ = nullptr; }
}
