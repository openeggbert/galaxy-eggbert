#include "GalaxyEggbertApp.hpp"
#include "Game/GalaxyEggbertGame.hpp"

#ifdef GE_ENGINE_U3D
using namespace Urho3D;
#endif

GalaxyEggbertApp::GalaxyEggbertApp(Urho3D::Context* context)
    : Application(context) {}

void GalaxyEggbertApp::Setup() {
    engineParameters_[EP_WINDOW_TITLE] = "Galaxy Eggbert";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_WINDOW_WIDTH] = 1280;
    engineParameters_[EP_WINDOW_HEIGHT] = 720;
#ifdef GE_ENGINE_U3D
    engineParameters_[EP_RESOURCE_PATHS] = "Data;CoreData";
    engineParameters_[EP_LOG_NAME]       = "GalaxyEggbert.log";
    engineParameters_[EP_HEADLESS]       = false;
#endif
}

void GalaxyEggbertApp::Start() {
    game_ = std::make_unique<GalaxyEggbertGame>(context_);
    game_->Start();
#ifdef GE_ENGINE_U3D
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(GalaxyEggbertApp, HandleUpdate));
#endif
}

#ifdef GE_ENGINE_U3D
void GalaxyEggbertApp::HandleUpdate(StringHash /*eventType*/, VariantMap& eventData) {
    using namespace Update;
    GameUpdate(eventData[P_TIMESTEP].GetFloat());
}
#endif

void GalaxyEggbertApp::GameUpdate(float dt) {
    if (game_) game_->Update(dt);
}

void GalaxyEggbertApp::Stop() {
    game_.reset();
}
