#pragma once
#include <cstdint>

namespace GalaxyEggbert {

// Type of a moving object (enemy, collectible, effect, vehicle, etc.).
// Numeric IDs must NOT be renumbered — they are stored in level files
// and must match the original Windows Phone Speedy Blupi data format exactly.
enum class ObjectType : uint8_t
{
    ObjectType0   =   0, // null / inactive slot

    // Platform lifts
    ObjectType1   =   1, // standard platform lift
    ObjectType47  =  47, // platform lift, rightward carry bonus
    ObjectType48  =  48, // platform lift, leftward carry bonus

    // Horizontal patrol enemies
    ObjectType2   =   2, // standard patrol enemy
    ObjectType3   =   3, // patrol enemy variant
    ObjectType96  =  96, // follow enemy variant 1
    ObjectType97  =  97, // follow enemy variant 2 (tracks Blupi exactly)

    // Bulldozer
    ObjectType4   =   4,

    // Collectibles
    ObjectType5   =   5, // treasure
    ObjectType6   =   6, // extra-life egg
    ObjectType7   =   7, // level-exit goal marker
    ObjectType21  =  21, // secret-level exit marker
    ObjectType39  =  39, // sparkle trail (spawned on pickup)

    // Key collectibles (correspond to DoorKeyFlags::Key1/2/3)
    ObjectType49  =  49, // key 1
    ObjectType50  =  50, // key 2
    ObjectType51  =  51, // key 3

    // Vehicle / power-up pickups
    ObjectType13  =  13, // helicopter
    ObjectType19  =  19, // jeep
    ObjectType24  =  24, // skateboard
    ObjectType25  =  25, // shield (100 ticks invincibility)
    ObjectType26  =  26, // suction-cup power-up (Sucette)
    ObjectType28  =  28, // tank
    ObjectType29  =  29, // bullet ammo pack (+10 bullets)
    ObjectType30  =  30, // drink power-up
    ObjectType31  =  31, // charge/cloud power-up (100 ticks)
    ObjectType40  =  40, // mirror/invert power-up (100 ticks)
    ObjectType46  =  46, // balloon
    ObjectType55  =  55, // dynamite stick

    // Explosions and visual effects (transient, auto-expire)
    ObjectType8   =   8, // primary explosion (Explosion channel)
    ObjectType9   =   9, // secondary small explosion
    ObjectType10  =  10, // tertiary explosion
    ObjectType11  =  11, // fan-hit shockwave (triggers BigShake)
    ObjectType12  =  12,
    ObjectType36  =  36, // pollution/cloud puff (8 frames)
    ObjectType37  =  37, // clear/dissipate effect (70 frames)
    ObjectType38  =  38, // electric arc (90 frames)
    ObjectType41  =  41, // invert-start particle burst
    ObjectType42  =  42, // invert-stop particle burst
    ObjectType53  =  53, // tentacle hazard (45 frames, Explosion channel)
    ObjectType90  =  90, // electric spark (triggers ElectricShake)
    ObjectType91  =  91, // small flash
    ObjectType92  =  92, // long energy arc (128 frames)
    ObjectType93  =  93, // tiny flash (5 frames)
    ObjectType98  =  98, // water splash variant 1 (10 frames)
    ObjectType99  =  99, // water splash variant 2 (13 frames)
    ObjectType100 = 100, // water splash variant 3 (18 frames)

    // Water / goo effects
    ObjectType14  =  14, // water plouf splash
    ObjectType15  =  15, // water bubble rising
    ObjectType34  =  34, // goo/glue particle (25-frame loop)
    ObjectType35  =  35, // small plouf splash

    // Projectiles
    ObjectType23  =  23, // fired projectile (from blupih/blupit enemies)

    // Patrol walker enemies
    ObjectType16  =  16, // spider/arthropod
    ObjectType17  =  17, // fish (water sections)
    ObjectType18  =  18, // patrol variant
    ObjectType20  =  20, // bird
    ObjectType32  =  32, // blupih — fires ObjectType23 projectiles on turn
    ObjectType33  =  33, // blupit — fires two projectiles per turn
    ObjectType44  =  44, // wasp/bee (fast patrol)
    ObjectType54  =  54, // large creature (destroys helicopter on contact)

    // Moving level objects
    ObjectType22  =  22, // door opening animation
    ObjectType27  =  27, // magic track sparkle (24 frames)
    ObjectType52  =  52, // bridge construction (157 frames)
    ObjectType56  =  56, // dynamite fuse (100 frames, triggers blasts 50-69)
    ObjectType57  =  57, // shield trail sparkle (20 frames)
    ObjectType58  =  58, // shield disappear effect

    // Blupi avatar colour variants
    ObjectType200 = 200, // default skin
    ObjectType201 = 201, // skin variant 1 (damages Blupi on contact)
    ObjectType202 = 202, // skin variant 2
    ObjectType203 = 203, // skin variant 3

    // Unidentified / reserved (declared to keep enum contiguous for level-file round-trips)
    ObjectType43  =  43,
    ObjectType45  =  45,
    ObjectType59  =  59,
    ObjectType60  =  60,
    ObjectType61  =  61,
    ObjectType62  =  62,
    ObjectType63  =  63,
    ObjectType64  =  64,
    ObjectType65  =  65,
    ObjectType66  =  66,
    ObjectType67  =  67,
    ObjectType68  =  68,
    ObjectType69  =  69,
    ObjectType70  =  70,
    ObjectType71  =  71,
    ObjectType72  =  72,
    ObjectType73  =  73,
    ObjectType74  =  74,
    ObjectType75  =  75,
    ObjectType76  =  76,
    ObjectType77  =  77,
    ObjectType78  =  78,
    ObjectType79  =  79,
    ObjectType80  =  80,
    ObjectType81  =  81,
    ObjectType82  =  82,
    ObjectType83  =  83,
    ObjectType84  =  84,
    ObjectType85  =  85,
    ObjectType86  =  86,
    ObjectType87  =  87,
    ObjectType88  =  88,
    ObjectType89  =  89,
    ObjectType94  =  94,
    ObjectType95  =  95,
    ObjectType101 = 101,
    ObjectType102 = 102,
    ObjectType103 = 103,
    ObjectType104 = 104,
    ObjectType105 = 105,
    ObjectType106 = 106,
    ObjectType107 = 107,
    ObjectType108 = 108,
    ObjectType109 = 109,
    ObjectType110 = 110,
    ObjectType111 = 111,
    ObjectType112 = 112,
    ObjectType113 = 113,
    ObjectType114 = 114,
    ObjectType115 = 115,
    ObjectType116 = 116,
    ObjectType117 = 117,
    ObjectType118 = 118,
    ObjectType119 = 119,
    ObjectType120 = 120,
    ObjectType121 = 121,
    ObjectType122 = 122,
    ObjectType123 = 123,
    ObjectType124 = 124,
    ObjectType125 = 125,
    ObjectType126 = 126,
    ObjectType127 = 127,
    ObjectType128 = 128,
    ObjectType129 = 129,
    ObjectType130 = 130,
    ObjectType131 = 131,
    ObjectType132 = 132,
    ObjectType133 = 133,
    ObjectType134 = 134,
    ObjectType135 = 135,
    ObjectType136 = 136,
    ObjectType137 = 137,
    ObjectType138 = 138,
    ObjectType139 = 139,
    ObjectType140 = 140,
    ObjectType141 = 141,
    ObjectType142 = 142,
    ObjectType143 = 143,
    ObjectType144 = 144,
    ObjectType145 = 145,
    ObjectType146 = 146,
    ObjectType147 = 147,
    ObjectType148 = 148,
    ObjectType149 = 149,
    ObjectType150 = 150,
    ObjectType151 = 151,
    ObjectType152 = 152,
    ObjectType153 = 153,
    ObjectType154 = 154,
    ObjectType155 = 155,
    ObjectType156 = 156,
    ObjectType157 = 157,
    ObjectType158 = 158,
    ObjectType159 = 159,
    ObjectType160 = 160,
    ObjectType161 = 161,
    ObjectType162 = 162,
    ObjectType163 = 163,
    ObjectType164 = 164,
    ObjectType165 = 165,
    ObjectType166 = 166,
    ObjectType167 = 167,
    ObjectType168 = 168,
    ObjectType169 = 169,
    ObjectType170 = 170,
    ObjectType171 = 171,
    ObjectType172 = 172,
    ObjectType173 = 173,
    ObjectType174 = 174,
    ObjectType175 = 175,
    ObjectType176 = 176,
    ObjectType177 = 177,
    ObjectType178 = 178,
    ObjectType179 = 179,
    ObjectType180 = 180,
    ObjectType181 = 181,
    ObjectType182 = 182,
    ObjectType183 = 183,
    ObjectType184 = 184,
    ObjectType185 = 185,
    ObjectType186 = 186,
    ObjectType187 = 187,
    ObjectType188 = 188,
    ObjectType189 = 189,
    ObjectType190 = 190,
    ObjectType191 = 191,
    ObjectType192 = 192,
    ObjectType193 = 193,
    ObjectType194 = 194,
    ObjectType195 = 195,
    ObjectType196 = 196,
    ObjectType197 = 197,
    ObjectType198 = 198,
    ObjectType199 = 199,
};

constexpr uint8_t   ToRaw(ObjectType t)       { return static_cast<uint8_t>(t); }
constexpr ObjectType ToObjectType(int v)      { return static_cast<ObjectType>(static_cast<uint8_t>(v)); }

constexpr bool operator<(ObjectType a, ObjectType b)  { return ToRaw(a) <  ToRaw(b); }
constexpr bool operator<=(ObjectType a, ObjectType b) { return ToRaw(a) <= ToRaw(b); }
constexpr bool operator>(ObjectType a, ObjectType b)  { return ToRaw(a) >  ToRaw(b); }
constexpr bool operator>=(ObjectType a, ObjectType b) { return ToRaw(a) >= ToRaw(b); }

} // namespace GalaxyEggbert
