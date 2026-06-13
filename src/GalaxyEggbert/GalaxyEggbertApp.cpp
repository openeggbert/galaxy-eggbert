#include "GalaxyEggbertApp.hpp"
#include "Game/GalaxyEggbertGame.hpp"

using namespace Urho3D;

GalaxyEggbertApp::GalaxyEggbertApp(Urho3D::Context* context)
    : Application(context) {}

GalaxyEggbertApp::~GalaxyEggbertApp() = default;

void GalaxyEggbertApp::Setup() {
    engineParameters_[EP_WINDOW_TITLE]   = "Galaxy Eggbert";
    engineParameters_[EP_FULL_SCREEN]    = false;
    engineParameters_[EP_WINDOW_WIDTH]   = 1280;
    engineParameters_[EP_WINDOW_HEIGHT]  = 720;
    engineParameters_[EP_RESOURCE_PATHS] = "Data;CoreData;Content";
    engineParameters_[EP_LOG_NAME]       = "GalaxyEggbert.log";
    engineParameters_[EP_HEADLESS]       = false;
}

void GalaxyEggbertApp::Start() {
    game_ = std::make_unique<GalaxyEggbertGame>(context_);
    game_->Start();
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(GalaxyEggbertApp, HandleUpdate));
}

void GalaxyEggbertApp::HandleUpdate(StringHash /*eventType*/, VariantMap& eventData) {
    using namespace Update;
    GameUpdate(eventData[P_TIMESTEP].GetFloat());
}

void GalaxyEggbertApp::GameUpdate(float dt) {
    if (game_) game_->Update(dt);
}

void GalaxyEggbertApp::Stop() {
    game_.reset();
}
