//using GalaxyEggbert;

#include "GalaxyEggbert/MyResource.hpp"
#include <locale>
#include <string>
#include <algorithm>

namespace GalaxyEggbert
{
    std::unordered_map<intcs, std::string> MyResource::resources = {
        // optional: initialize with default values
    };

    void MyResource::EnsureInitialized()
    {
        static const bool initialized = []()
        {
            Init();
            return true;
        }();
        (void)initialized;
    }

    const string& MyResource::LoadString(const intcs res)
    {
        EnsureInitialized();

        auto it = resources.find(res);
        if (it != resources.end())
        {
            return it->second;
        }

        static const string DEFAULT_VALUE = "???";
        return DEFAULT_VALUE;
    }

    const intcs MyResource::TX_BUTTON_PLAY = 100;
    const intcs MyResource::TX_BUTTON_MENU = 101;
    const intcs MyResource::TX_BUTTON_BACK = 102;
    const intcs MyResource::TX_BUTTON_RESTART = 103;
    const intcs MyResource::TX_BUTTON_CONTINUE = 104;
    const intcs MyResource::TX_BUTTON_BUY = 105;
    const intcs MyResource::TX_BUTTON_SETUP = 107;
    const intcs MyResource::TX_BUTTON_SETUP_SOUNDS = 108;
    const intcs MyResource::TX_BUTTON_SETUP_JUMP = 109;
    const intcs MyResource::TX_BUTTON_SETUP_ZOOM = 110;
    const intcs MyResource::TX_BUTTON_SETUP_ACCEL = 111;
    const intcs MyResource::TX_BUTTON_SETUP_RESET = 112;
    const intcs MyResource::TX_BUTTON_RANKING = 113;
    const intcs MyResource::TX_GAMER_TITLE = 200;
    const intcs MyResource::TX_GAMER_MDOORS = 201;
    const intcs MyResource::TX_GAMER_SDOORS = 202;
    const intcs MyResource::TX_GAMER_LIFES = 203;
    const intcs MyResource::TX_TRIAL1 = 300;
    const intcs MyResource::TX_TRIAL2 = 301;
    const intcs MyResource::TX_TRIAL3 = 302;
    const intcs MyResource::TX_TRIAL4 = 303;
    const intcs MyResource::TX_TRIAL5 = 304;
    const intcs MyResource::TX_TRIAL6 = 305;
    const intcs MyResource::TX_TRAINING101 = 1000;
    const intcs MyResource::TX_TRAINING102 = 1001;
    const intcs MyResource::TX_TRAINING103 = 1002;
    const intcs MyResource::TX_TRAINING104 = 1003;
    const intcs MyResource::TX_TRAINING105 = 1004;
    const intcs MyResource::TX_TRAINING106 = 1005;
    const intcs MyResource::TX_TRAINING107 = 1006;
    const intcs MyResource::TX_TRAINING108 = 1007;
    const intcs MyResource::TX_TRAINING109 = 1008;
    const intcs MyResource::TX_TRAINING110 = 1009;
    const intcs MyResource::TX_TRAINING111 = 1010;
    const intcs MyResource::TX_TRAINING112 = 1011;
    const intcs MyResource::TX_TRAINING113 = 1012;
    const intcs MyResource::TX_TRAINING114 = 1013;
    const intcs MyResource::TX_TRAINING115 = 1014;
    const intcs MyResource::TX_TRAINING116 = 1015;
    const intcs MyResource::TX_TRAINING117 = 1016;
    const intcs MyResource::TX_TRAINING118 = 1017;
    const intcs MyResource::TX_TRAINING119 = 1018;
    const intcs MyResource::TX_TRAINING120 = 1019;
    const intcs MyResource::TX_TRAINING121 = 1020;
    const intcs MyResource::TX_TRAINING122 = 1021;
    const intcs MyResource::TX_TRAINING123 = 1022;
    const intcs MyResource::TX_TRAINING201 = 2000;
    const intcs MyResource::TX_TRAINING202 = 2001;
    const intcs MyResource::TX_TRAINING203 = 2002;
    const intcs MyResource::TX_TRAINING204 = 2003;
    const intcs MyResource::TX_TRAINING205 = 2004;
    const intcs MyResource::TX_TRAINING206 = 2005;
    const intcs MyResource::TX_TRAINING207 = 2006;
    const intcs MyResource::TX_TRAINING208 = 2007;
    const intcs MyResource::TX_TRAINING209 = 2008;
    const intcs MyResource::TX_TRAINING210 = 2009;
    const intcs MyResource::TX_TRAINING301 = 3000;
    const intcs MyResource::TX_TRAINING302 = 3001;
    const intcs MyResource::TX_TRAINING303 = 3002;
    const intcs MyResource::TX_TRAINING304 = 3003;
    const intcs MyResource::TX_TRAINING305 = 3004;
    const intcs MyResource::TX_TRAINING306 = 3005;
    const intcs MyResource::TX_TRAINING307 = 3006;
    const intcs MyResource::TX_TRAINING308 = 3007;
    const intcs MyResource::TX_TRAINING309 = 3008;
    const intcs MyResource::TX_TRAINING310 = 3009;
    const intcs MyResource::TX_TRAINING311 = 3010;
    const intcs MyResource::TX_TRAINING401 = 4000;
    const intcs MyResource::TX_TRAINING402 = 4001;
    const intcs MyResource::TX_TRAINING403 = 4002;
    const intcs MyResource::TX_TRAINING404 = 4003;
    const intcs MyResource::TX_TRAINING405 = 4004;
    const intcs MyResource::TX_TRAINING406 = 4005;
    const intcs MyResource::TX_TRAINING407 = 4006;
    const intcs MyResource::TX_TRAINING408 = 4007;
    const intcs MyResource::TX_TRAINING409 = 4008;
    const intcs MyResource::TX_TRAINING410 = 4009;
    const intcs MyResource::TX_TRAINING101a = 11000;
    const intcs MyResource::TX_TRAINING102a = 11001;
    const intcs MyResource::TX_TRAINING103a = 11002;
    const intcs MyResource::TX_TRAINING104a = 11003;
    const intcs MyResource::TX_TRAINING105a = 11004;
    const intcs MyResource::TX_TRAINING106a = 11005;
    const intcs MyResource::TX_TRAINING107a = 11006;
    const intcs MyResource::TX_TRAINING108a = 11007;
    const intcs MyResource::TX_TRAINING109a = 11008;
    const intcs MyResource::TX_TRAINING110a = 11009;
    const intcs MyResource::TX_TRAINING111a = 11010;
    const intcs MyResource::TX_TRAINING112a = 11011;
    const intcs MyResource::TX_TRAINING113a = 11012;
    const intcs MyResource::TX_TRAINING114a = 11013;
    const intcs MyResource::TX_TRAINING115a = 11014;
    const intcs MyResource::TX_TRAINING116a = 11015;
    const intcs MyResource::TX_TRAINING117a = 11016;
    const intcs MyResource::TX_TRAINING118a = 11017;
    const intcs MyResource::TX_TRAINING119a = 11018;
    const intcs MyResource::TX_TRAINING120a = 11019;
    const intcs MyResource::TX_TRAINING121a = 11020;
    const intcs MyResource::TX_TRAINING122a = 11021;
    const intcs MyResource::TX_TRAINING123a = 11022;
    const intcs MyResource::TX_TRAINING201a = 12000;
    const intcs MyResource::TX_TRAINING202a = 12001;
    const intcs MyResource::TX_TRAINING203a = 12002;
    const intcs MyResource::TX_TRAINING204a = 12003;
    const intcs MyResource::TX_TRAINING205a = 12004;
    const intcs MyResource::TX_TRAINING206a = 12005;
    const intcs MyResource::TX_TRAINING207a = 12006;
    const intcs MyResource::TX_TRAINING208a = 12007;
    const intcs MyResource::TX_TRAINING209a = 12008;
    const intcs MyResource::TX_TRAINING210a = 12009;
    const intcs MyResource::TX_TRAINING301a = 13000;
    const intcs MyResource::TX_TRAINING302a = 13001;
    const intcs MyResource::TX_TRAINING303a = 13002;
    const intcs MyResource::TX_TRAINING304a = 13003;
    const intcs MyResource::TX_TRAINING305a = 13004;
    const intcs MyResource::TX_TRAINING306a = 13005;
    const intcs MyResource::TX_TRAINING307a = 13006;
    const intcs MyResource::TX_TRAINING308a = 13007;
    const intcs MyResource::TX_TRAINING309a = 13008;
    const intcs MyResource::TX_TRAINING310a = 13009;
    const intcs MyResource::TX_TRAINING311a = 13010;
    const intcs MyResource::TX_TRAINING401a = 14000;
    const intcs MyResource::TX_TRAINING402a = 14001;
    const intcs MyResource::TX_TRAINING403a = 14002;
    const intcs MyResource::TX_TRAINING404a = 14003;
    const intcs MyResource::TX_TRAINING405a = 14004;
    const intcs MyResource::TX_TRAINING406a = 14005;
    const intcs MyResource::TX_TRAINING407a = 14006;
    const intcs MyResource::TX_TRAINING408a = 14007;
    const intcs MyResource::TX_TRAINING409a = 14008;
    const intcs MyResource::TX_TRAINING410a = 14009;

    void MyResource::Init()
    {
        std::string languageCode = "en";

        try
        {
            std::locale loc("");
            std::string localeName = loc.name();

            if (localeName.size() >= 2)
            {
                languageCode = localeName.substr(0, 2);
                std::transform(
                    languageCode.begin(),
                    languageCode.end(),
                    languageCode.begin(),
                    [](unsigned char c)
                    {
                        return static_cast<char>(std::tolower(c));
                    });
            }
        }
        catch (...)
        {
            languageCode = "en";
        }

        if (languageCode == "fr")
        {
            InitializeFR();
        }
        else
        {
            InitializeEN();
        }
    }

    void MyResource::InitializeFR()
    {
        resources.emplace(TX_BUTTON_PLAY, "Jouer");
        resources.emplace(TX_BUTTON_MENU, "Menu");
        resources.emplace(TX_BUTTON_BACK, "Retour");
        resources.emplace(TX_BUTTON_RESTART, "Recommencer");
        resources.emplace(TX_BUTTON_CONTINUE, "Continuer");
        resources.emplace(TX_BUTTON_BUY, "Acheter");
        resources.emplace(TX_BUTTON_RANKING, "Classement");
        resources.emplace(TX_BUTTON_SETUP, "Réglages");
        resources.emplace(TX_BUTTON_SETUP_SOUNDS, "Bruitages");
        resources.emplace(TX_BUTTON_SETUP_JUMP, "Bouton de saut à droite");
        resources.emplace(TX_BUTTON_SETUP_ZOOM, "Zoom automatique sur l'action");
        resources.emplace(TX_BUTTON_SETUP_ACCEL, "Accéléromètre");
        resources.emplace(TX_BUTTON_SETUP_RESET, "Joueur {0} :\nEffacer la progression");
        resources.emplace(TX_GAMER_TITLE, "Joueur {0}");
        resources.emplace(TX_GAMER_MDOORS, "Portes principales : {0}/12");
        resources.emplace(TX_GAMER_SDOORS, "Portes secondaires : {0}/52");
        resources.emplace(TX_GAMER_LIFES, "Blupi : {0}");
        resources.emplace(TX_TRIAL1, "Achetez la version complète");
        resources.emplace(TX_TRIAL2, "\u000e 64 niveaux passionnants");
        resources.emplace(TX_TRIAL3, "\u000e Des décors variés");
        resources.emplace(TX_TRIAL4, "\u000e Une difficulté progressive");
        resources.emplace(TX_TRIAL5, "\u000e De nouveaux pièges");
        resources.emplace(TX_TRIAL6, "\u000e Un challenge fun");
        resources.emplace(TX_TRAINING101, MakeResourceString("Utilise la roue directionnelle \0."));
        resources.emplace(TX_TRAINING102, "Appuie maintenant sur Saut \b.");
        resources.emplace(TX_TRAINING103,
                          MakeResourceString("Appuie à droite sur la roue directionnelle \0 et sur Saut \b."));
        resources.emplace(TX_TRAINING104, MakeResourceString("Appuie sur Droite \0 et Saut \b."));
        resources.emplace(TX_TRAINING105, MakeResourceString("Essaie de ne pas te mouiller avec \0 \b !"));
        resources.emplace(TX_TRAINING106, "");
        resources.emplace(TX_TRAINING107, MakeResourceString("Prend l'ascenseur calmement, sans sauter \0."));
        resources.emplace(TX_TRAINING108, "Saute sur l'ascenseur.");
        resources.emplace(TX_TRAINING109, "");
        resources.emplace(TX_TRAINING110, MakeResourceString("Avance sans sauter et sans t'arrêter \0 !"));
        resources.emplace(TX_TRAINING111, "");
        resources.emplace(TX_TRAINING112, "");
        resources.emplace(TX_TRAINING113, MakeResourceString("Avance sur la plateforme \0."));
        resources.emplace(TX_TRAINING114, MakeResourceString("Quitte la plateforme \0."));
        resources.emplace(TX_TRAINING115, "Encore une fois, mais plus vite...");
        resources.emplace(TX_TRAINING116, MakeResourceString("Avance sur la plateforme \0 puis saute \0 \b."));
        resources.emplace(TX_TRAINING117, MakeResourceString("Saute lorsque tu es sur la plateforme \0 \b."));
        resources.emplace(TX_TRAINING118, MakeResourceString("Passe par en haut \0 \b."));
        resources.emplace(TX_TRAINING119, "Les oeufs te redonnent des vies.");
        resources.emplace(TX_TRAINING120, "Arrivé en haut, avance sur l'autre plateforme sans tarder...");
        resources.emplace(TX_TRAINING121, "Attrape le deuxième et dernier trésor.");
        resources.emplace(TX_TRAINING122, "Pour terminer, va sur la flèche rouge.");
        resources.emplace(TX_TRAINING201, MakeResourceString("Pousse la caisse sur le point rouge avec \0."));
        resources.emplace(TX_TRAINING202, "Pratique, cette caisse, non ?");
        resources.emplace(TX_TRAINING203, "Tire la caisse sur le point rouge avec \u0003.");
        resources.emplace(TX_TRAINING204, "Empile les 2 caisses sur le point rouge pour passer.");
        resources.emplace(TX_TRAINING205, "Empile 3 caisses sur le point rouge pour passer.");
        resources.emplace(TX_TRAINING206, "");
        resources.emplace(TX_TRAINING207, "");
        resources.emplace(TX_TRAINING208, "");
        resources.emplace(TX_TRAINING209, "");
        resources.emplace(TX_TRAINING210, "");
        resources.emplace(TX_TRAINING301, "Prend un hélico avec \t.");
        resources.emplace(TX_TRAINING302,
                          MakeResourceString("Utilise \u0006 ou \b pour décoller. Dirige avec \u0004 et \0."));
        resources.emplace(TX_TRAINING303, "Quitte l'hélico avec \t, il n'aime pas l'eau !");
        resources.emplace(TX_TRAINING304,
                          MakeResourceString("Plonge. Utilise \u0004 \0 \u0002 \u0006 pour te diriger."));
        resources.emplace(TX_TRAINING305, "Prend un hélico \t puis décole avec \u0006 ou \b.");
        resources.emplace(TX_TRAINING306, "Attrape les 3 trésors puis monte.");
        resources.emplace(TX_TRAINING307, "Pour passer, va chercher un skate en haut à gauche.");
        resources.emplace(TX_TRAINING308, "Prend un skate avec \t.");
        resources.emplace(TX_TRAINING309, MakeResourceString("Tu peux passer sans crainte avec ton skate \0."));
        resources.emplace(TX_TRAINING310, MakeResourceString("Ton skate n'aime pas l'eau ! Utilise \0 \b."));
        resources.emplace(TX_TRAINING311, "Pose ton skate avec \t.");
        resources.emplace(TX_TRAINING401, "Prend la dynamite avec \t.");
        resources.emplace(TX_TRAINING402, "Ne pose surtout pas la dynamite ici avec \t !");
        resources.emplace(TX_TRAINING403, "Va chercher de la dynamite à gauche.");
        resources.emplace(TX_TRAINING404, "Pose la dynamite avec \t, puis barre-toi !");
        resources.emplace(TX_TRAINING405, "Pose encore de la dynamite pour passer.");
        resources.emplace(TX_TRAINING406, "");
        resources.emplace(TX_TRAINING407, "");
        resources.emplace(TX_TRAINING408, "");
        resources.emplace(TX_TRAINING409, "");
        resources.emplace(TX_TRAINING410, "");
        resources.emplace(TX_TRAINING101a, "Incline le téléphone à droite \v.");
        resources.emplace(TX_TRAINING102a, "Appuie maintenant sur Saut \b.");
        resources.emplace(TX_TRAINING103a, "\v et appuie sur Saut \b.");
        resources.emplace(TX_TRAINING104a, "\v et Saut \b.");
        resources.emplace(TX_TRAINING105a, "Essaie de ne pas te mouiller avec \v \b !");
        resources.emplace(TX_TRAINING106a, "");
        resources.emplace(TX_TRAINING107a, "Prend l'ascenseur calmement, sans sauter \v.");
        resources.emplace(TX_TRAINING108a, "Saute sur l'ascenseur.");
        resources.emplace(TX_TRAINING109a, "");
        resources.emplace(TX_TRAINING110a, "Avance sans sauter et sans t'arrêter \v !");
        resources.emplace(TX_TRAINING111a, "");
        resources.emplace(TX_TRAINING112a, "");
        resources.emplace(TX_TRAINING113a, "Avance sur la plateforme \v.");
        resources.emplace(TX_TRAINING114a, "Quitte la plateforme \v.");
        resources.emplace(TX_TRAINING115a, "Encore une fois, mais plus vite...");
        resources.emplace(TX_TRAINING116a, "Avance sur la plateforme \v puis saute \v \b.");
        resources.emplace(TX_TRAINING117a, "Saute lorsque tu es sur la plateforme \v \b.");
        resources.emplace(TX_TRAINING118a, "Passe par en haut \v \b.");
        resources.emplace(TX_TRAINING119a, "Les oeufs te redonnent des vies.");
        resources.emplace(TX_TRAINING120a, "Arrivé en haut, avance sur l'autre plateforme sans tarder...");
        resources.emplace(TX_TRAINING121a, "Attrape le deuxième et dernier trésor.");
        resources.emplace(TX_TRAINING122a, "Pour terminer, va sur la flèche rouge.");
        resources.emplace(TX_TRAINING201a, "Pousse la caisse sur le point rouge avec \v.");
        resources.emplace(TX_TRAINING202a, "Pratique, cette caisse, non ?");
        resources.emplace(TX_TRAINING203a, "Tire la caisse sur le point rouge avec \f et \n.");
        resources.emplace(TX_TRAINING204a, "Empile les 2 caisses sur le point rouge pour passer.");
        resources.emplace(TX_TRAINING205a, "Empile 3 caisses sur le point rouge pour passer.");
        resources.emplace(TX_TRAINING206a, "");
        resources.emplace(TX_TRAINING207a, "");
        resources.emplace(TX_TRAINING208a, "");
        resources.emplace(TX_TRAINING209a, "");
        resources.emplace(TX_TRAINING210a, "");
        resources.emplace(TX_TRAINING301a, "Prend un hélico avec \t.");
        resources.emplace(TX_TRAINING302a, "Utilise \b pour décoller. Dirige avec \f et \v.");
        resources.emplace(TX_TRAINING303a, "Quitte l'hélico avec \t, il n'aime pas l'eau !");
        resources.emplace(TX_TRAINING304a, "Plonge. Utilise \f \v \b \n pour te diriger.");
        resources.emplace(TX_TRAINING305a, "Prend un hélico \t puis décole avec \b.");
        resources.emplace(TX_TRAINING306a, "Attrape les 3 trésors puis monte.");
        resources.emplace(TX_TRAINING307a, "Pour passer, va chercher un skate en haut à gauche.");
        resources.emplace(TX_TRAINING308a, "Prend un skate avec \t.");
        resources.emplace(TX_TRAINING309a, "Tu peux passer sans crainte avec ton skate \v.");
        resources.emplace(TX_TRAINING310a, "Ton skate n'aime pas l'eau ! Utilise \v \b.");
        resources.emplace(TX_TRAINING311a, "Pose ton skate avec \t.");
        resources.emplace(TX_TRAINING401a, "Prend la dynamite avec \t.");
        resources.emplace(TX_TRAINING402a, "Ne pose surtout pas la dynamite ici avec \t !");
        resources.emplace(TX_TRAINING403a, "Va chercher de la dynamite à gauche.");
        resources.emplace(TX_TRAINING404a, "Pose la dynamite avec \t, puis barre-toi !");
        resources.emplace(TX_TRAINING405a, "Pose encore de la dynamite pour passer.");
        resources.emplace(TX_TRAINING406a, "");
        resources.emplace(TX_TRAINING407a, "");
        resources.emplace(TX_TRAINING408a, "");
        resources.emplace(TX_TRAINING409a, "");
        resources.emplace(TX_TRAINING410a, "");
    }

    void MyResource::InitializeEN()
    {
        resources.emplace(TX_BUTTON_PLAY, "Play");
        resources.emplace(TX_BUTTON_MENU, "Home");
        resources.emplace(TX_BUTTON_BACK, "Back");
        resources.emplace(TX_BUTTON_RESTART, "Restart");
        resources.emplace(TX_BUTTON_CONTINUE, "Continue");
        resources.emplace(TX_BUTTON_BUY, "Buy");
        resources.emplace(TX_BUTTON_RANKING, "Ranking");
        resources.emplace(TX_BUTTON_SETUP, "Setup");
        resources.emplace(TX_BUTTON_SETUP_SOUNDS, "Sound effects");
        resources.emplace(TX_BUTTON_SETUP_JUMP, "Jump button on the right");
        resources.emplace(TX_BUTTON_SETUP_ZOOM, "Automatic zoom on action");
        resources.emplace(TX_BUTTON_SETUP_ACCEL, "Accelerometer");
        resources.emplace(TX_BUTTON_SETUP_RESET, "Player {0} :\nErase progress");
        resources.emplace(TX_GAMER_TITLE, "Player {0}");
        resources.emplace(TX_GAMER_MDOORS, "Main gates : {0}/12");
        resources.emplace(TX_GAMER_SDOORS, "Secondary gates : {0}/52");
        resources.emplace(TX_GAMER_LIFES, "Blupi : {0}");
        resources.emplace(TX_TRIAL1, "Buy the full version");
        resources.emplace(TX_TRIAL2, "\u000e 64 exciting stages");
        resources.emplace(TX_TRIAL3, "\u000e Varied backgrounds");
        resources.emplace(TX_TRIAL4, "\u000e An increasing difficulty");
        resources.emplace(TX_TRIAL5, "\u000e New traps");
        resources.emplace(TX_TRIAL6, "\u000e Challenge and fun");
        resources.emplace(TX_TRAINING101, MakeResourceString("Use the directional wheel \0."));
        resources.emplace(TX_TRAINING102, "Press Jump \b.");
        resources.emplace(TX_TRAINING103, MakeResourceString("Press Right \0 and Jump \b."));
        resources.emplace(TX_TRAINING104, MakeResourceString("Press Right \0 and Jump \b."));
        resources.emplace(TX_TRAINING105, MakeResourceString("Don't fall into the water \0 \b !"));
        resources.emplace(TX_TRAINING106, "");
        resources.emplace(TX_TRAINING107, MakeResourceString("Take the elevator quietly, without jumping \0."));
        resources.emplace(TX_TRAINING108, "Jump on the elevator.");
        resources.emplace(TX_TRAINING109, "");
        resources.emplace(TX_TRAINING110, MakeResourceString("Move forward without jumping nor stopping \0 !"));
        resources.emplace(TX_TRAINING111, "");
        resources.emplace(TX_TRAINING112, "");
        resources.emplace(TX_TRAINING113, MakeResourceString("Move forward on the platform \0."));
        resources.emplace(TX_TRAINING114, MakeResourceString("Leave the platform \0."));
        resources.emplace(TX_TRAINING115, "Once again, but faster...");
        resources.emplace(TX_TRAINING116, MakeResourceString("Move forward on the platform \0, then jump \0 \b."));
        resources.emplace(TX_TRAINING117, MakeResourceString("Jump when you are on the platform \0 \b."));
        resources.emplace(TX_TRAINING118, MakeResourceString("Choose the upper path \0 \b."));
        resources.emplace(TX_TRAINING119, "Eggs give you extra lives.");
        resources.emplace(TX_TRAINING120, "Once on the top, move forward on the other platform without delay...");
        resources.emplace(TX_TRAINING121, "Catch the second and last treasure.");
        resources.emplace(TX_TRAINING122, "Join the red arrow.");
        resources.emplace(TX_TRAINING201, MakeResourceString("Push the box forward until the red dot with \0."));
        resources.emplace(TX_TRAINING202, "Practical, right?");
        resources.emplace(TX_TRAINING203, "Pull the box backward until the red dot with \u0003.");
        resources.emplace(TX_TRAINING204, "Stack both boxes up on the red dot to move on.");
        resources.emplace(TX_TRAINING205, "Stack the three boxes up on the red dot to move on.");
        resources.emplace(TX_TRAINING206, "");
        resources.emplace(TX_TRAINING207, "");
        resources.emplace(TX_TRAINING208, "");
        resources.emplace(TX_TRAINING209, "");
        resources.emplace(TX_TRAINING210, "");
        resources.emplace(TX_TRAINING301, "Take a helicopter with \t.");
        resources.emplace(TX_TRAINING302,
                          MakeResourceString("Use \u0006 or \b to take off. Direct with \u0004 and \0."));
        resources.emplace(TX_TRAINING303, "Leave the helicopter with \t, it dislikes water!");
        resources.emplace(TX_TRAINING304, MakeResourceString("Plunge. Use \u0004 \0 \u0002 \u0006 to direct."));
        resources.emplace(TX_TRAINING305, MakeResourceString("Take a helicopter \t then take off with \u0006 or \b."));
        resources.emplace(TX_TRAINING306, "Grab the three treasures, then go up.");
        resources.emplace(TX_TRAINING307, "Go and get a skate in the top left corner.");
        resources.emplace(TX_TRAINING308, "Take a skate with \t.");
        resources.emplace(TX_TRAINING309, MakeResourceString("You can move on with your skate without fear \0."));
        resources.emplace(TX_TRAINING310, "Your skate dislikes water! Jump!");
        resources.emplace(TX_TRAINING311, "Leave your skate with \t.");
        resources.emplace(TX_TRAINING401, "Take the dynamite sticks with \t.");
        resources.emplace(TX_TRAINING402, "Do not put down the dynamite here!");
        resources.emplace(TX_TRAINING403, "Go and get the dynamite sticks on the left.");
        resources.emplace(TX_TRAINING404, "Put down the dynamite with \t, then clear off!");
        resources.emplace(TX_TRAINING405, "Put down another stick of dynamite here to move on.");
        resources.emplace(TX_TRAINING406, "");
        resources.emplace(TX_TRAINING407, "");
        resources.emplace(TX_TRAINING408, "");
        resources.emplace(TX_TRAINING409, "");
        resources.emplace(TX_TRAINING410, "");
        resources.emplace(TX_TRAINING101a, "Tilt the phone \v.");
        resources.emplace(TX_TRAINING102a, "Press Jump \b.");
        resources.emplace(TX_TRAINING103a, "\v and Jump \b.");
        resources.emplace(TX_TRAINING104a, "\v and Jump \b.");
        resources.emplace(TX_TRAINING105a, "Don't fall into the water \v \b !");
        resources.emplace(TX_TRAINING106a, "");
        resources.emplace(TX_TRAINING107a, "Take the elevator quietly, without jumping \v.");
        resources.emplace(TX_TRAINING108a, "Jump on the elevator.");
        resources.emplace(TX_TRAINING109a, "");
        resources.emplace(TX_TRAINING110a, "Move forward without jumping nor stopping \v !");
        resources.emplace(TX_TRAINING111a, "");
        resources.emplace(TX_TRAINING112a, "");
        resources.emplace(TX_TRAINING113a, "Move forward on the platform \v.");
        resources.emplace(TX_TRAINING114a, "Leave the platform \v.");
        resources.emplace(TX_TRAINING115a, "Once again, but faster...");
        resources.emplace(TX_TRAINING116a, "Move forward on the platform \v, then jump \v \b.");
        resources.emplace(TX_TRAINING117a, "Jump when you are on the platform \v \b.");
        resources.emplace(TX_TRAINING118a, "Choose the upper path \v \b.");
        resources.emplace(TX_TRAINING119a, "Eggs give you extra lives.");
        resources.emplace(TX_TRAINING120a, "Once on the top, move forward on the other platform without delay...");
        resources.emplace(TX_TRAINING121a, "Catch the second and last treasure.");
        resources.emplace(TX_TRAINING122a, "Join the red arrow.");
        resources.emplace(TX_TRAINING201a, "Push the box forward until the red dot with \v.");
        resources.emplace(TX_TRAINING202a, "Practical, right?");
        resources.emplace(TX_TRAINING203a, "Pull the box backward until the red dot with \f and \n.");
        resources.emplace(TX_TRAINING204a, "Stack both boxes up on the red dot to move on.");
        resources.emplace(TX_TRAINING205a, "Stack the three boxes up on the red dot to move on.");
        resources.emplace(TX_TRAINING206a, "");
        resources.emplace(TX_TRAINING207a, "");
        resources.emplace(TX_TRAINING208a, "");
        resources.emplace(TX_TRAINING209a, "");
        resources.emplace(TX_TRAINING210a, "");
        resources.emplace(TX_TRAINING301a, "Take a helicopter with \t.");
        resources.emplace(TX_TRAINING302a, "Use \b to take off. Direct with \f and \v.");
        resources.emplace(TX_TRAINING303a, "Leave the helicopter with \t, it dislikes water!");
        resources.emplace(TX_TRAINING304a, "Plunge. Use \f \v \b \n to direct.");
        resources.emplace(TX_TRAINING305a, "Take a helicopter \t then take off with \b.");
        resources.emplace(TX_TRAINING306a, "Grab the three treasures, then go up.");
        resources.emplace(TX_TRAINING307a, "Go and get a skate in the top left corner.");
        resources.emplace(TX_TRAINING308a, "Take a skate with \t.");
        resources.emplace(TX_TRAINING309a, "You can move on with your skate without fear \v.");
        resources.emplace(TX_TRAINING310a, "Your skate dislikes water! Jump!");
        resources.emplace(TX_TRAINING311a, "Leave your skate with \t.");
        resources.emplace(TX_TRAINING401a, "Take the dynamite sticks with \t.");
        resources.emplace(TX_TRAINING402a, "Do not put down the dynamite here!");
        resources.emplace(TX_TRAINING403a, "Go and get the dynamite sticks on the left.");
        resources.emplace(TX_TRAINING404a, "Put down the dynamite with \t, then clear off!");
        resources.emplace(TX_TRAINING405a, "Put down another stick of dynamite here to move on.");
        resources.emplace(TX_TRAINING406a, "");
        resources.emplace(TX_TRAINING407a, "");
        resources.emplace(TX_TRAINING408a, "");
        resources.emplace(TX_TRAINING409a, "");
        resources.emplace(TX_TRAINING410a, "");
    }

    void MyResource::InitializeDE()
    {
        resources.emplace(TX_BUTTON_PLAY, "Play");
        resources.emplace(TX_BUTTON_MENU, "Home");
        resources.emplace(TX_BUTTON_BACK, "Back");
        resources.emplace(TX_BUTTON_RESTART, "Restart");
        resources.emplace(TX_BUTTON_CONTINUE, "Continue");
        resources.emplace(TX_BUTTON_BUY, "Buy");
        resources.emplace(TX_BUTTON_RANKING, "Ranking");
        resources.emplace(TX_BUTTON_SETUP, "Setup");
        resources.emplace(TX_BUTTON_SETUP_SOUNDS, "Sounds");
        resources.emplace(TX_BUTTON_SETUP_JUMP, "Jump button to the right");
        resources.emplace(TX_BUTTON_SETUP_ZOOM, "Automatically zoom action");
        resources.emplace(TX_BUTTON_SETUP_ACCEL, "Accelerometer");
        resources.emplace(TX_BUTTON_SETUP_RESET, "Gamer {0} :\nReset progression");
        resources.emplace(TX_GAMER_TITLE, "Gamer {0}");
        resources.emplace(TX_GAMER_MDOORS, "Main doors : {0}/12");
        resources.emplace(TX_GAMER_SDOORS, "Secondary doors : {0}/52");
        resources.emplace(TX_GAMER_LIFES, "Blupi : {0}");
        resources.emplace(TX_TRIAL1, "Buy the full version");
        resources.emplace(TX_TRIAL2, "\u000e 64 niveaux passionnants");
        resources.emplace(TX_TRIAL3, "\u000e Des décors variés");
        resources.emplace(TX_TRIAL4, "\u000e Une difficulté progressive");
        resources.emplace(TX_TRIAL5, "\u000e De nouveaux pièges");
        resources.emplace(TX_TRIAL6, "\u000e Un challenge fun");
        resources.emplace(TX_TRAINING101,
                          MakeResourceString("Utilise la roue directionnelle \0 pour faire avancer Blupi."));
        resources.emplace(TX_TRAINING102, "Appuie maintenant sur le bouton de saut \b.");
        resources.emplace(TX_TRAINING103,
                          MakeResourceString("Appuie à droite sur la roue directionnelle \0 et sur Saut \b."));
        resources.emplace(TX_TRAINING104, MakeResourceString("Appuie sur Droite \0 et Saut \b."));
        resources.emplace(TX_TRAINING105, MakeResourceString("Essaie de ne pas te mouiller avec \0 \b !"));
        resources.emplace(TX_TRAINING106, "");
        resources.emplace(TX_TRAINING107, MakeResourceString("Prend l'ascenseur calmement, sans sauter \0."));
        resources.emplace(TX_TRAINING108, "Saute sur l'ascenseur.");
        resources.emplace(TX_TRAINING109, "");
        resources.emplace(TX_TRAINING110, MakeResourceString("Avance sans sauter et sans t'arrêter \0 !"));
        resources.emplace(TX_TRAINING111, "");
        resources.emplace(TX_TRAINING112, "");
        resources.emplace(TX_TRAINING113, MakeResourceString("Avance sur la plateforme \0."));
        resources.emplace(TX_TRAINING114, MakeResourceString("Quitte la plateforme \0."));
        resources.emplace(TX_TRAINING115, "Encore une fois, mais plus vite...");
        resources.emplace(TX_TRAINING116, MakeResourceString("Avance sur la plateforme \0 puis saute \0 \b."));
        resources.emplace(TX_TRAINING117, MakeResourceString("Saute lorsque tu es sur la plateforme \0 \b."));
        resources.emplace(TX_TRAINING118, MakeResourceString("Passe par en haut \0 \b."));
        resources.emplace(TX_TRAINING119, "Les oeufs te redonnent des vies.");
        resources.emplace(TX_TRAINING120, "Arrivé en haut, avance sur l'autre plateforme sans tarder...");
        resources.emplace(TX_TRAINING121, "Attrape le deuxième et dernier trésor.");
        resources.emplace(TX_TRAINING122, "Pour terminer, va sur la flèche rouge.");
        resources.emplace(TX_TRAINING201, MakeResourceString("Pousse la caisse sur le point rouge avec \0."));
        resources.emplace(TX_TRAINING202, MakeResourceString("Utilise la caisse pour passer l'obstacle avec \0 \b."));
        resources.emplace(TX_TRAINING203, "Tire la caisse sur le point rouge avec \u0003.");
        resources.emplace(TX_TRAINING204, "Empile les 2 caisses sur le point rouge pour passer.");
        resources.emplace(TX_TRAINING205, "Empile 3 caisses sur le point rouge pour passer.");
        resources.emplace(TX_TRAINING206, "");
        resources.emplace(TX_TRAINING207, "");
        resources.emplace(TX_TRAINING208, "");
        resources.emplace(TX_TRAINING209, "");
        resources.emplace(TX_TRAINING210, "");
        resources.emplace(TX_TRAINING301, "Prend un hélico avec \t.");
        resources.emplace(TX_TRAINING302,
                          MakeResourceString("Utilise \u0006 ou \b pour décoler. Dirige avec \u0004 et \0."));
        resources.emplace(TX_TRAINING303, "Quitte l'hélico avec \t, il n'aime pas l'eau !");
        resources.emplace(TX_TRAINING304,
                          MakeResourceString("Plonge. Utilise \u0004 \0 \u0002 \u0006 pour te diriger."));
        resources.emplace(TX_TRAINING305, MakeResourceString("Prend un hélico \t puis décole avec \u0006 ou \b."));
        resources.emplace(TX_TRAINING306, "Attrape les 3 trésors puis monte.");
        resources.emplace(TX_TRAINING307, "Pour passer, va chercher un skate en haut à gauche.");
        resources.emplace(TX_TRAINING308, "Prend un skate avec \t.");
        resources.emplace(TX_TRAINING309, MakeResourceString("Tu peux passer sans crainte avec ton skate \0."));
        resources.emplace(TX_TRAINING310, MakeResourceString("Ton skate n'aime pas l'eau ! Utilise \0 \b."));
        resources.emplace(TX_TRAINING311, "Pose ton skate avec \t.");
        resources.emplace(TX_TRAINING401, "Prend la dynamite avec \t.");
        resources.emplace(TX_TRAINING402, "Ne pose surtout pas la dynamite ici avec \t !");
        resources.emplace(TX_TRAINING403, "Va chercher de la dynamite à gauche.");
        resources.emplace(TX_TRAINING404, "Pose la dynamite avec \t, puis barre-toi !");
        resources.emplace(TX_TRAINING405, "Pose encore de la dynamite pour passer.");
        resources.emplace(TX_TRAINING406, "");
        resources.emplace(TX_TRAINING407, "");
        resources.emplace(TX_TRAINING408, "");
        resources.emplace(TX_TRAINING409, "");
        resources.emplace(TX_TRAINING410, "");
    }
}
