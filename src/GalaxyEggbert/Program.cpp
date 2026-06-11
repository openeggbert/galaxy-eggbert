#include "GalaxyEggbertApp.hpp"

#ifdef GE_ENGINE_U3D
URHO3D_DEFINE_APPLICATION_MAIN(GalaxyEggbertApp)
#else
// Nova3D entry point — no macro, plain main()
int main() {
    Urho3D::Context context;
    GalaxyEggbertApp app(&context);
    return app.Run();
}
#endif
