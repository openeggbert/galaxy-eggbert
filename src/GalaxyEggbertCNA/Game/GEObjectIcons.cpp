#include "GEObjectIcons.hpp"

namespace GalaxyEggbert::CNA
{
    int GetObjIcon(ObjectType type, int p)
    {
        static const int kCle1[12]    = {209,210,211,212,213,214,215,214,213,212,211,210};
        static const int kCle2[12]    = {220,221,222,221,220,219,218,217,216,217,218,219};
        static const int kCle3[12]    = {229,228,227,226,225,224,223,224,225,226,227,228};
        static const int kShield[8]   = {144,145,146,147,148,149,150,151};
        static const int kBulldozer[8]= {66,66,67,67,66,66,65,65};
        static const int kBird[8]     = {98,99,100,101,102,103,104,105};
        static const int kFish[8]     = {82,82,81,81,82,82,83,83};
        static const int kBlupit[8]   = {249,249,250,250,249,249,248,248};
        static const int kGuepeLeft[6]    = {195,196,197,198,197,196};
        static const int kCreature[8]     = {247,248,249,250,251,250,249,248};
        static const int kBlupihLeft[8]   = {66,67,68,67,66,69,70,69};
        static const int kFollow1[26]     = {256,256,256,257,257,258,259,260,261,262,
                                              263,264,264,265,265,265,264,264,263,262,
                                              261,260,259,258,257,257};
        static const int kChenille[6]     = {311,312,313,314,315,316};
        static const int kCleGeneric[12]  = {122,123,124,125,126,127,128,127,126,125,124,123};
        static const int kSkate[34]       = {129,129,129,129,130,130,130,131,131,132,
                                              132,133,133,134,134,134,135,135,135,135,
                                              134,134,134,133,133,132,132,131,131,131,
                                              130,130,130,130};
        static const int kPower[8]        = {136,137,138,139,140,141,142,143};
        static const int kInvert[20]      = {187,187,187,188,189,190,191,192,193,194,
                                              187,187,187,194,193,192,191,190,189,188};
        switch (type)
        {
            case ObjectType::ObjectType1:  return 29;
            case ObjectType::ObjectType2:  return 12 + (p / 6) % 9;
            case ObjectType::ObjectType3:  return 48 + (p / 6) % 9;
            case ObjectType::ObjectType4:  return kBulldozer[(p / 9) % 8];
            case ObjectType::ObjectType12: return 32;
            case ObjectType::ObjectType13: return 68;
            case ObjectType::ObjectType16: return 69 + (p / 3) % 9;
            case ObjectType::ObjectType17: return kFish[(p / 6) % 8];
            case ObjectType::ObjectType20: return kBird[(p / 6) % 8];
            case ObjectType::ObjectType30: return 178;
            case ObjectType::ObjectType33: return kBlupit[(p / 6) % 8];
            case ObjectType::ObjectType5: { int q = (p / 9) % 22; return (q < 11) ? q : (21 - q); }
            case ObjectType::ObjectType6:  return 21 + (p / 12) % 8;
            case ObjectType::ObjectType7:  return 29 + (p /  9) % 8;
            case ObjectType::ObjectType49: return kCle1[(p / 9) % 12];
            case ObjectType::ObjectType50: return kCle2[(p / 9) % 12];
            case ObjectType::ObjectType51: return kCle3[(p / 9) % 12];
            case ObjectType::ObjectType25: return kShield[(p / 6) % 8];
            case ObjectType::ObjectType19: return 89;
            case ObjectType::ObjectType46: return 208;
            case ObjectType::ObjectType55: return 252;
            case ObjectType::ObjectType21: return kCleGeneric[(p / 9) % 12];
            case ObjectType::ObjectType24: return kSkate[(p / 3) % 34];
            case ObjectType::ObjectType26: return kPower[(p / 6) % 8];
            case ObjectType::ObjectType40: return kInvert[(p / 4) % 20];
            case ObjectType::ObjectType47: return kChenille[(p / 6) % 6];
            case ObjectType::ObjectType32: return kBlupihLeft[(p / 6) % 8];
            case ObjectType::ObjectType44: return kGuepeLeft[(p / 6) % 6];
            case ObjectType::ObjectType54: return kCreature[(p / 6) % 8];
            case ObjectType::ObjectType96: return kFollow1[(p / 3) % 26];
            default:                       return 0;
        }
    }

    ObjectIconUv GetElementIconUv(int icon)
    {
        constexpr int kTilePx = 60;
        constexpr int kCols = 10;
        constexpr float kSheetW = 600.0f;
        constexpr float kSheetH = 1740.0f;
        const int col = icon % kCols;
        const int row = icon / kCols;
        const float u0 = static_cast<float>(col * kTilePx) / kSheetW;
        const float v0 = static_cast<float>(row * kTilePx) / kSheetH;
        const float u1 = static_cast<float>(col * kTilePx + kTilePx) / kSheetW;
        const float v1 = static_cast<float>(row * kTilePx + kTilePx) / kSheetH;
        return ObjectIconUv{u0, v0, u1, v1};
    }
}
