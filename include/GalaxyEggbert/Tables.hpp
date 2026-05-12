#pragma once
#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    using SharpRuntime::shortcs;

    class Tables
    {
    public:
        Tables() = delete;
        ~Tables() = delete;

        enum class CheatCodes
        {
            BuildOfficialMissions,
            OpenDoors,
            CleanAll,
            SuperBlupi,
            LayEgg,
            KillEgg,
            Skate,
            Copter,
            Jeep,
            AllTreasure,
            EndGoal,
            ShowSecret,
            RoundShield,
            Lollipop,
            Bombs,
            BirdLime,
            Tank,
            PowerCharge,
            Drink,
            Overcraft,
            Dynamite,
            WeelKeys
        };

        static const shortcs table_blupi[2911];

        static const shortcs table_mirror[335];

        static const shortcs table_vitesse_march[4];

        static const shortcs table_vitesse_nage[7];

        static const shortcs table_vitesse_surf[6];

        static const shortcs table_decor_quart[7056];

        static const shortcs table_bulldozer_left[8];

        static const shortcs table_bulldozer_right[8];

        static const shortcs table_bulldozer_turn2l[22];

        static const shortcs table_bulldozer_turn2r[22];

        static const shortcs table_poisson_left[8];

        static const shortcs table_poisson_right[8];

        static const shortcs table_poisson_turn2l[48];

        static const shortcs table_poisson_turn2r[48];

        static const shortcs table_oiseau_left[8];

        static const shortcs table_oiseau_right[8];

        static const shortcs table_oiseau_turn2l[10];

        static const shortcs table_oiseau_turn2r[10];

        static const shortcs table_guepe_left[6];

        static const shortcs table_guepe_right[6];

        static const shortcs table_guepe_turn2l[5];

        static const shortcs table_guepe_turn2r[5];

        static const shortcs table_creature_left[8];

        static const shortcs table_creature_right[8];

        static const shortcs table_creature_turn2[152];

        static const shortcs table_blupih_left[8];

        static const shortcs table_blupih_right[8];

        static const shortcs table_blupih_turn2l[26];

        static const shortcs table_blupih_turn2r[26];

        static const shortcs table_blupit_left[8];

        static const shortcs table_blupit_right[8];

        static const shortcs table_blupit_turn2l[24];

        static const shortcs table_blupit_turn2r[24];

        static constexpr shortcs table_explo1Length = 39;

        static const shortcs table_explo1[table_explo1Length];

        static const shortcs table_explo2[20];

        static const shortcs table_explo3[20];

        static const shortcs table_explo4[9];

        static const shortcs table_explo5[12];

        static const shortcs table_explo6[6];

        static const shortcs table_explo7[128];

        static const shortcs table_explo8[5];

        static const shortcs table_sploutch1[10];

        static const shortcs table_sploutch2[13];

        static const shortcs table_sploutch3[18];

        static const shortcs table_tentacule[45];

        static const shortcs table_bridge[157];

        static const shortcs table_pollution[8];

        static const shortcs table_invertstart[8];

        static const shortcs table_invertstop[8];

        static const shortcs table_invertpanel[8];

        static const shortcs table_plouf[7];

        static const shortcs table_tiplouf[3];

        static const shortcs table_blup[20];

        static const shortcs table_follow1[26];

        static const shortcs table_follow2[5];

        static const shortcs table_cle[12];

        static const shortcs table_cle1[12];

        static const shortcs table_cle2[12];

        static const shortcs table_cle3[12];

        static const shortcs table_dynamitef[100];

        static const shortcs table_skate[34];

        static const shortcs table_glu[25];

        static const shortcs table_clear[70];

        static const shortcs table_electro[90];

        static const shortcs table_chenille[6];

        static const shortcs table_chenillei[6];

        static const shortcs table_adapt_decor[144];

        static const shortcs table_adapt_fromage[32];

        static const shortcs table_shield[16];

        static const shortcs table_shield_blupi[16];

        static const shortcs table_power[8];

        static const shortcs table_invert[20];

        static const shortcs table_charge[6];

        static const shortcs table_magicloop[5];

        static const shortcs table_magictrack[24];

        static const shortcs table_shieldloop[5];

        static const shortcs table_shieldtrack[20];

        static const shortcs table_drinkeffect[5];

        static constexpr unsigned char table_drinkoffsetLength = 3;

        static const shortcs table_drinkoffset[table_drinkoffsetLength];

        static const shortcs table_tresortrack[11];

        static const shortcs table_decor_lave[8];

        static const shortcs table_decor_piege1[16];

        static const shortcs table_decor_piege2[4];

        static const shortcs table_decor_goutte[48];

        static const shortcs table_decor_ecraseur[10];

        static const shortcs table_decor_scie[6];

        static const shortcs table_decor_temp[20];

        static const shortcs table_decor_eau1[6];

        static const shortcs table_decor_eau2[6];

        static const shortcs table_decor_ventillog[3];

        static const shortcs table_decor_ventillod[3];

        static const shortcs table_decor_ventilloh[3];

        static const shortcs table_decor_ventillob[3];

        static const shortcs table_decor_ventg[4];

        static const shortcs table_decor_ventd[4];

        static const shortcs table_decor_venth[4];

        static const shortcs table_decor_ventb[4];

        static const shortcs table_marine[11];

        static const shortcs table_ressort[8];

        static shortcs table_training1[133];

        static shortcs table_training2[31];

        static shortcs table_training3[67];

        static shortcs table_training4[31];

        static const shortcs table_decor_action[519];

        static const shortcs table_explo_size[100];

        static const shortcs world_terminal[30];

        static void Init();
    };
}
