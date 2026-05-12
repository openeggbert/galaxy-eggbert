#pragma once

#include <string>
#include <unordered_map>

#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    using std::string;
    using SharpRuntime::intcs;

    class MyResource
    {
    public:
        static const intcs TX_BUTTON_PLAY;
        static const intcs TX_BUTTON_MENU;
        static const intcs TX_BUTTON_BACK;
        static const intcs TX_BUTTON_RESTART;
        static const intcs TX_BUTTON_CONTINUE;
        static const intcs TX_BUTTON_BUY;
        static const intcs TX_BUTTON_SETUP;
        static const intcs TX_BUTTON_SETUP_SOUNDS;
        static const intcs TX_BUTTON_SETUP_JUMP;
        static const intcs TX_BUTTON_SETUP_ZOOM;
        static const intcs TX_BUTTON_SETUP_ACCEL;
        static const intcs TX_BUTTON_SETUP_RESET;
        static const intcs TX_BUTTON_RANKING;
        static const intcs TX_GAMER_TITLE;
        static const intcs TX_GAMER_MDOORS;
        static const intcs TX_GAMER_SDOORS;
        static const intcs TX_GAMER_LIFES;
        static const intcs TX_TRIAL1;
        static const intcs TX_TRIAL2;
        static const intcs TX_TRIAL3;
        static const intcs TX_TRIAL4;
        static const intcs TX_TRIAL5;
        static const intcs TX_TRIAL6;
        static const intcs TX_TRAINING101;
        static const intcs TX_TRAINING102;
        static const intcs TX_TRAINING103;
        static const intcs TX_TRAINING104;
        static const intcs TX_TRAINING105;
        static const intcs TX_TRAINING106;
        static const intcs TX_TRAINING107;
        static const intcs TX_TRAINING108;
        static const intcs TX_TRAINING109;
        static const intcs TX_TRAINING110;
        static const intcs TX_TRAINING111;
        static const intcs TX_TRAINING112;
        static const intcs TX_TRAINING113;
        static const intcs TX_TRAINING114;
        static const intcs TX_TRAINING115;
        static const intcs TX_TRAINING116;
        static const intcs TX_TRAINING117;
        static const intcs TX_TRAINING118;
        static const intcs TX_TRAINING119;
        static const intcs TX_TRAINING120;
        static const intcs TX_TRAINING121;
        static const intcs TX_TRAINING122;
        static const intcs TX_TRAINING123;
        static const intcs TX_TRAINING201;
        static const intcs TX_TRAINING202;
        static const intcs TX_TRAINING203;
        static const intcs TX_TRAINING204;
        static const intcs TX_TRAINING205;
        static const intcs TX_TRAINING206;
        static const intcs TX_TRAINING207;
        static const intcs TX_TRAINING208;
        static const intcs TX_TRAINING209;
        static const intcs TX_TRAINING210;
        static const intcs TX_TRAINING301;
        static const intcs TX_TRAINING302;
        static const intcs TX_TRAINING303;
        static const intcs TX_TRAINING304;
        static const intcs TX_TRAINING305;
        static const intcs TX_TRAINING306;
        static const intcs TX_TRAINING307;
        static const intcs TX_TRAINING308;
        static const intcs TX_TRAINING309;
        static const intcs TX_TRAINING310;
        static const intcs TX_TRAINING311;
        static const intcs TX_TRAINING401;
        static const intcs TX_TRAINING402;
        static const intcs TX_TRAINING403;
        static const intcs TX_TRAINING404;
        static const intcs TX_TRAINING405;
        static const intcs TX_TRAINING406;
        static const intcs TX_TRAINING407;
        static const intcs TX_TRAINING408;
        static const intcs TX_TRAINING409;
        static const intcs TX_TRAINING410;
        static const intcs TX_TRAINING101a;
        static const intcs TX_TRAINING102a;
        static const intcs TX_TRAINING103a;
        static const intcs TX_TRAINING104a;
        static const intcs TX_TRAINING105a;
        static const intcs TX_TRAINING106a;
        static const intcs TX_TRAINING107a;
        static const intcs TX_TRAINING108a;
        static const intcs TX_TRAINING109a;
        static const intcs TX_TRAINING110a;
        static const intcs TX_TRAINING111a;
        static const intcs TX_TRAINING112a;
        static const intcs TX_TRAINING113a;
        static const intcs TX_TRAINING114a;
        static const intcs TX_TRAINING115a;
        static const intcs TX_TRAINING116a;
        static const intcs TX_TRAINING117a;
        static const intcs TX_TRAINING118a;
        static const intcs TX_TRAINING119a;
        static const intcs TX_TRAINING120a;
        static const intcs TX_TRAINING121a;
        static const intcs TX_TRAINING122a;
        static const intcs TX_TRAINING123a;
        static const intcs TX_TRAINING201a;
        static const intcs TX_TRAINING202a;
        static const intcs TX_TRAINING203a;
        static const intcs TX_TRAINING204a;
        static const intcs TX_TRAINING205a;
        static const intcs TX_TRAINING206a;
        static const intcs TX_TRAINING207a;
        static const intcs TX_TRAINING208a;
        static const intcs TX_TRAINING209a;
        static const intcs TX_TRAINING210a;
        static const intcs TX_TRAINING301a;
        static const intcs TX_TRAINING302a;
        static const intcs TX_TRAINING303a;
        static const intcs TX_TRAINING304a;
        static const intcs TX_TRAINING305a;
        static const intcs TX_TRAINING306a;
        static const intcs TX_TRAINING307a;
        static const intcs TX_TRAINING308a;
        static const intcs TX_TRAINING309a;
        static const intcs TX_TRAINING310a;
        static const intcs TX_TRAINING311a;
        static const intcs TX_TRAINING401a;
        static const intcs TX_TRAINING402a;
        static const intcs TX_TRAINING403a;
        static const intcs TX_TRAINING404a;
        static const intcs TX_TRAINING405a;
        static const intcs TX_TRAINING406a;
        static const intcs TX_TRAINING407a;
        static const intcs TX_TRAINING408a;
        static const intcs TX_TRAINING409a;
        static const intcs TX_TRAINING410a;

    private:
        static std::unordered_map<intcs, std::string> resources;

    public:
        static const string& LoadString(intcs res);

    private:
        static void EnsureInitialized();
        static void Init();

        template <size_t N>
        static std::string MakeResourceString(const char (&text)[N])
        {
            return std::string(text, N - 1);
        }

        static void InitializeFR();
        static void InitializeEN();
        static void InitializeDE();
    };
}
