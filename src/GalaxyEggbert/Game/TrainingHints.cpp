#include "TrainingHints.hpp"

#include "Localization.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

using namespace std::string_view_literals;

namespace GalaxyEggbert::Game
{
    namespace
    {
        // Real Decor::IsDisplayInfo gate codes (Decor.cpp:1313-1340).
        constexpr int kGateAlways = -1;
        constexpr int kGateNoVehicle = -2;
        constexpr int kGateAnyVehicle = -3;
        constexpr int kGateNoDynamite = -4;
        constexpr int kGateHasDynamite = -5;
        // Any gate value >= 0 means "treasuresCollected == gate" (real
        // `m_nbTresor == tableTresor`) -- checked directly, no named
        // constant needed.

        struct TrainingHintRecord
        {
            int colMin, colMax, rowMin, rowMax;
            int gate;
            // The original's English text (MyResource, TX_TRAINING<mission><record>) and its
            // accelerometer variant (+10000, the "a" strings, Decor.cpp:1287-1291). Bytes 0-13 are
            // text.png's pad pictograms (the wheel's 8 directions, Jump, Action, Down, the tilted
            // phone), drawn inline as the original's Text::DrawChar does (BD-0064).
            std::string_view text;
            std::string_view accelText;
            // The French (MyResource::InitializeFR, the same TX_TRAINING ids; transcribed with the owner's
            // approval, Q-023, 2026-11-11), its pictograms where the French puts them, its spelling
            // corrected in two words ("Prend" -> "Prends", "décole" -> "décolle").
            std::string_view textFr;
            std::string_view accelTextFr;
            // German and Czech, translated for Galaxy (the owner's Q-023 answers, 2026-11-12), the
            // pictograms where each puts them.
            std::string_view textDe;
            std::string_view accelTextDe;
            std::string_view textCs;
            std::string_view accelTextCs;
        };

        // Mission 11 (Tables::table_training1, 22 real records). Real
        // source's own action-flag values: -2 (0 elsewhere in this table)
        // and 1 mean "treasuresCollected == 0" / "== 1" -- an exact-count
        // gate, not a threshold.
        constexpr TrainingHintRecord kTraining1[] = {
            {1, 3, 0, 50, kGateAlways, "Use the directional wheel \000."sv, "Tilt the phone \013."sv,
             "Utilise la roue directionnelle \000."sv, "Incline le téléphone à droite \013."sv,
             "Benutze das Steuerrad \000."sv, "Neige das Telefon \013."sv,
             "Použij směrové kolečko \000."sv, "Nakloň telefon \013."sv},
            {4, 4, 0, 50, 0, "Press Jump \010."sv, "Press Jump \010."sv,
             "Appuie maintenant sur Saut \010."sv, "Appuie maintenant sur Saut \010."sv,
             "Drücke Springen \010."sv, "Drücke Springen \010."sv,
             "Stiskni Skok \010."sv, "Stiskni Skok \010."sv},
            {6, 6, 0, 50, 1, "Press Right \000 and Jump \010."sv, "\013 and Jump \010."sv,
             "Appuie à droite sur la roue directionnelle \000 et sur Saut \010."sv, "\013 et appuie sur Saut \010."sv,
             "Drücke Rechts \000 und Springen \010."sv, "\013 und Springen \010."sv,
             "Stiskni Vpravo \000 a Skok \010."sv, "\013 a Skok \010."sv},
            {9, 9, 0, 50, kGateAlways, "Press Right \000 and Jump \010."sv, "\013 and Jump \010."sv,
             "Appuie sur Droite \000 et Saut \010."sv, "\013 et Saut \010."sv,
             "Drücke Rechts \000 und Springen \010."sv, "\013 und Springen \010."sv,
             "Stiskni Vpravo \000 a Skok \010."sv, "\013 a Skok \010."sv},
            {12, 14, 0, 50, 1, "Don't fall into the water \000 \010 !"sv, "Don't fall into the water \013 \010 !"sv,
             "Essaie de ne pas te mouiller avec \000 \010 !"sv, "Essaie de ne pas te mouiller avec \013 \010 !"sv,
             "Fall nicht ins Wasser \000 \010!"sv, "Fall nicht ins Wasser \013 \010!"sv,
             "Nespadni do vody \000 \010!"sv, "Nespadni do vody \013 \010!"sv},
            {16, 16, 0, 50, kGateAlways, ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv},
            {20, 21, 0, 50, kGateAlways, "Take the elevator quietly, without jumping \000."sv, "Take the elevator quietly, without jumping \013."sv,
             "Prends l'ascenseur calmement, sans sauter \000."sv, "Prends l'ascenseur calmement, sans sauter \013."sv,
             "Nimm den Aufzug ruhig, ohne zu springen \000."sv, "Nimm den Aufzug ruhig, ohne zu springen \013."sv,
             "Jeď výtahem klidně, bez skákání \000."sv, "Jeď výtahem klidně, bez skákání \013."sv},
            {23, 24, 0, 50, kGateAlways, "Jump on the elevator."sv, "Jump on the elevator."sv,
             "Saute sur l'ascenseur."sv, "Saute sur l'ascenseur."sv,
             "Spring auf den Aufzug."sv, "Spring auf den Aufzug."sv,
             "Skoč na výtah."sv, "Skoč na výtah."sv},
            {27, 27, 0, 50, kGateAlways, ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv},
            {28, 28, 0, 50, kGateAlways, "Move forward without jumping nor stopping \000 !"sv, "Move forward without jumping nor stopping \013 !"sv,
             "Avance sans sauter et sans t'arrêter \000 !"sv, "Avance sans sauter et sans t'arrêter \013 !"sv,
             "Geh weiter, ohne zu springen oder anzuhalten \000!"sv, "Geh weiter, ohne zu springen oder anzuhalten \013!"sv,
             "Jdi dál bez skákání a bez zastavení \000!"sv, "Jdi dál bez skákání a bez zastavení \013!"sv},
            {30, 31, 0, 50, kGateAlways, ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv},
            {36, 36, 0, 50, kGateAlways, ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv,
             ""sv, ""sv},
            {39, 39, 0, 50, kGateAlways, "Move forward on the platform \000."sv, "Move forward on the platform \013."sv,
             "Avance sur la plateforme \000."sv, "Avance sur la plateforme \013."sv,
             "Geh auf die Plattform \000."sv, "Geh auf die Plattform \013."sv,
             "Vstup na plošinu \000."sv, "Vstup na plošinu \013."sv},
            {44, 44, 0, 50, kGateAlways, "Leave the platform \000."sv, "Leave the platform \013."sv,
             "Quitte la plateforme \000."sv, "Quitte la plateforme \013."sv,
             "Verlass die Plattform \000."sv, "Verlass die Plattform \013."sv,
             "Opusť plošinu \000."sv, "Opusť plošinu \013."sv},
            {46, 46, 0, 50, kGateAlways, "Once again, but faster..."sv, "Once again, but faster..."sv,
             "Encore une fois, mais plus vite..."sv, "Encore une fois, mais plus vite..."sv,
             "Noch einmal, aber schneller..."sv, "Noch einmal, aber schneller..."sv,
             "Ještě jednou, ale rychleji..."sv, "Ještě jednou, ale rychleji..."sv},
            {53, 53, 0, 50, kGateAlways, "Move forward on the platform \000, then jump \000 \010."sv, "Move forward on the platform \013, then jump \013 \010."sv,
             "Avance sur la plateforme \000 puis saute \000 \010."sv, "Avance sur la plateforme \013 puis saute \013 \010."sv,
             "Geh auf die Plattform \000, dann spring \000 \010."sv, "Geh auf die Plattform \013, dann spring \013 \010."sv,
             "Vstup na plošinu \000, pak skoč \000 \010."sv, "Vstup na plošinu \013, pak skoč \013 \010."sv},
            {56, 56, 0, 50, kGateAlways, "Jump when you are on the platform \000 \010."sv, "Jump when you are on the platform \013 \010."sv,
             "Saute lorsque tu es sur la plateforme \000 \010."sv, "Saute lorsque tu es sur la plateforme \013 \010."sv,
             "Spring, wenn du auf der Plattform bist \000 \010."sv, "Spring, wenn du auf der Plattform bist \013 \010."sv,
             "Skoč, až budeš na plošině \000 \010."sv, "Skoč, až budeš na plošině \013 \010."sv},
            {62, 64, 0, 50, kGateAlways, "Choose the upper path \000 \010."sv, "Choose the upper path \013 \010."sv,
             "Passe par en haut \000 \010."sv, "Passe par en haut \013 \010."sv,
             "Nimm den oberen Weg \000 \010."sv, "Nimm den oberen Weg \013 \010."sv,
             "Vyber horní cestu \000 \010."sv, "Vyber horní cestu \013 \010."sv},
            {65, 66, 0, 50, kGateAlways, "Eggs give you extra lives."sv, "Eggs give you extra lives."sv,
             "Les oeufs te redonnent des vies."sv, "Les oeufs te redonnent des vies."sv,
             "Eier geben dir zusätzliche Leben."sv, "Eier geben dir zusätzliche Leben."sv,
             "Vajíčka ti dají další životy."sv, "Vajíčka ti dají další životy."sv},
            {69, 74, 0, 50, kGateAlways, "Once on the top, move forward on the other platform without delay..."sv, "Once on the top, move forward on the other platform without delay..."sv,
             "Arrivé en haut, avance sur l'autre plateforme sans tarder..."sv, "Arrivé en haut, avance sur l'autre plateforme sans tarder..."sv,
             "Oben angekommen, geh ohne Zögern auf die andere Plattform..."sv, "Oben angekommen, geh ohne Zögern auf die andere Plattform..."sv,
             "Až budeš nahoře, vstup bez otálení na druhou plošinu..."sv, "Až budeš nahoře, vstup bez otálení na druhou plošinu..."sv},
            {80, 85, 0, 50, 1, "Catch the second and last treasure."sv, "Catch the second and last treasure."sv,
             "Attrape le deuxième et dernier trésor."sv, "Attrape le deuxième et dernier trésor."sv,
             "Hol dir den zweiten und letzten Schatz."sv, "Hol dir den zweiten und letzten Schatz."sv,
             "Seber druhý a poslední poklad."sv, "Seber druhý a poslední poklad."sv},
            {87, 93, 0, 50, kGateAlways, "Join the red arrow."sv, "Join the red arrow."sv,
             "Pour terminer, va sur la flèche rouge."sv, "Pour terminer, va sur la flèche rouge."sv,
             "Geh zum roten Pfeil."sv, "Geh zum roten Pfeil."sv,
             "Jdi k červené šipce."sv, "Jdi k červené šipce."sv},
        };

        // Mission 12 (Tables::table_training2, 5 real records, all
        // unconditional).
        constexpr TrainingHintRecord kTraining2[] = {
            {9, 15, 0, 100, kGateAlways, "Push the box forward until the red dot with \000."sv, "Push the box forward until the red dot with \013."sv,
             "Pousse la caisse sur le point rouge avec \000."sv, "Pousse la caisse sur le point rouge avec \013."sv,
             "Schieb die Kiste bis zum roten Punkt mit \000."sv, "Schieb die Kiste bis zum roten Punkt mit \013."sv,
             "Dotlač bednu na červenou tečku pomocí \000."sv, "Dotlač bednu na červenou tečku pomocí \013."sv},
            {16, 16, 0, 100, kGateAlways, "Practical, right?"sv, "Practical, right?"sv,
             "Pratique, cette caisse, non ?"sv, "Pratique, cette caisse, non ?"sv,
             "Praktisch, oder?"sv, "Praktisch, oder?"sv,
             "Praktické, ne?"sv, "Praktické, ne?"sv},
            {19, 21, 0, 100, kGateAlways, "Pull the box backward until the red dot with \003."sv, "Pull the box backward until the red dot with \014 and \012."sv,
             "Tire la caisse sur le point rouge avec \003."sv, "Tire la caisse sur le point rouge avec \014 et \012."sv,
             "Zieh die Kiste bis zum roten Punkt mit \003."sv, "Zieh die Kiste bis zum roten Punkt mit \014 und \012."sv,
             "Přitáhni bednu na červenou tečku pomocí \003."sv, "Přitáhni bednu na červenou tečku pomocí \014 a \012."sv},
            {24, 31, 0, 100, kGateAlways, "Stack both boxes up on the red dot to move on."sv, "Stack both boxes up on the red dot to move on."sv,
             "Empile les 2 caisses sur le point rouge pour passer."sv, "Empile les 2 caisses sur le point rouge pour passer."sv,
             "Stapel beide Kisten auf dem roten Punkt, um weiterzukommen."sv, "Stapel beide Kisten auf dem roten Punkt, um weiterzukommen."sv,
             "Postav obě bedny na sebe na červenou tečku, abys prošel."sv, "Postav obě bedny na sebe na červenou tečku, abys prošel."sv},
            {33, 40, 0, 100, kGateAlways, "Stack the three boxes up on the red dot to move on."sv, "Stack the three boxes up on the red dot to move on."sv,
             "Empile 3 caisses sur le point rouge pour passer."sv, "Empile 3 caisses sur le point rouge pour passer."sv,
             "Stapel die drei Kisten auf dem roten Punkt, um weiterzukommen."sv, "Stapel die drei Kisten auf dem roten Punkt, um weiterzukommen."sv,
             "Postav tři bedny na sebe na červenou tečku, abys prošel."sv, "Postav tři bedny na sebe na červenou tečku, abys prošel."sv},
        };

        // Mission 13 (Tables::table_training3, 11 real records). Real
        // source's -2/-3 gates only ever test Helicopter/Skateboard/Tank
        // vs. Jeep to resolve to the same "in ANY vehicle at all" result
        // either way (Decor.cpp:1319-1332) -- collapsed to a single
        // inAnyVehicle bool here, not a Jeep-specific distinction.
        constexpr TrainingHintRecord kTraining3[] = {
            {16, 24, 36, 40, kGateNoVehicle, "Take a helicopter with \011."sv, "Take a helicopter with \011."sv,
             "Prends un hélico avec \011."sv, "Prends un hélico avec \011."sv,
             "Nimm einen Hubschrauber mit \011."sv, "Nimm einen Hubschrauber mit \011."sv,
             "Nastup do vrtulníku pomocí \011."sv, "Nastup do vrtulníku pomocí \011."sv},
            {16, 24, 36, 40, kGateAnyVehicle, "Use \006 or \010 to take off. Direct with \004 and \000."sv, "Use \010 to take off. Direct with \014 and \013."sv,
             "Utilise \006 ou \010 pour décoller. Dirige avec \004 et \000."sv, "Utilise \010 pour décoller. Dirige avec \014 et \013."sv,
             "Benutze \006 oder \010 zum Abheben. Steuere mit \004 und \000."sv, "Benutze \010 zum Abheben. Steuere mit \014 und \013."sv,
             "Vzlétni pomocí \006 nebo \010. Řiď pomocí \004 a \000."sv, "Vzlétni pomocí \010. Řiď pomocí \014 a \013."sv},
            {22, 25, 34, 34, kGateAnyVehicle, "Leave the helicopter with \011, it dislikes water!"sv, "Leave the helicopter with \011, it dislikes water!"sv,
             "Quitte l'hélico avec \011, il n'aime pas l'eau !"sv, "Quitte l'hélico avec \011, il n'aime pas l'eau !"sv,
             "Verlass den Hubschrauber mit \011, er mag kein Wasser!"sv, "Verlass den Hubschrauber mit \011, er mag kein Wasser!"sv,
             "Vystup z vrtulníku pomocí \011, nemá rád vodu!"sv, "Vystup z vrtulníku pomocí \011, nemá rád vodu!"sv},
            {22, 31, 34, 37, kGateNoVehicle, "Plunge. Use \004 \000 \002 \006 to direct."sv, "Plunge. Use \014 \013 \010 \012 to direct."sv,
             "Plonge. Utilise \004 \000 \002 \006 pour te diriger."sv, "Plonge. Utilise \014 \013 \010 \012 pour te diriger."sv,
             "Tauch. Steuere mit \004 \000 \002 \006."sv, "Tauch. Steuere mit \014 \013 \010 \012."sv,
             "Ponoř se. Řiď pomocí \004 \000 \002 \006."sv, "Ponoř se. Řiď pomocí \014 \013 \010 \012."sv},
            {32, 43, 34, 34, kGateNoVehicle, "Take a helicopter \011 then take off with \006 or \010."sv, "Take a helicopter \011 then take off with \010."sv,
             "Prends un hélico \011 puis décolle avec \006 ou \010."sv, "Prends un hélico \011 puis décolle avec \010."sv,
             "Nimm einen Hubschrauber \011 und heb dann mit \006 oder \010 ab."sv, "Nimm einen Hubschrauber \011 und heb dann mit \010 ab."sv,
             "Nastup do vrtulníku \011 a vzlétni pomocí \006 nebo \010."sv, "Nastup do vrtulníku \011 a vzlétni pomocí \010."sv},
            {32, 43, 26, 34, kGateAnyVehicle, "Grab the three treasures, then go up."sv, "Grab the three treasures, then go up."sv,
             "Attrape les 3 trésors puis monte."sv, "Attrape les 3 trésors puis monte."sv,
             "Schnapp dir die drei Schätze, dann flieg hoch."sv, "Schnapp dir die drei Schätze, dann flieg hoch."sv,
             "Seber tři poklady a pak leť nahoru."sv, "Seber tři poklady a pak leť nahoru."sv},
            {44, 48, 20, 22, kGateNoVehicle, "Go and get a skate in the top left corner."sv, "Go and get a skate in the top left corner."sv,
             "Pour passer, va chercher un skate en haut à gauche."sv, "Pour passer, va chercher un skate en haut à gauche."sv,
             "Hol dir ein Skateboard oben links."sv, "Hol dir ein Skateboard oben links."sv,
             "Dojdi si pro skateboard vlevo nahoře."sv, "Dojdi si pro skateboard vlevo nahoře."sv},
            {29, 31, 12, 12, kGateAlways, "Take a skate with \011."sv, "Take a skate with \011."sv,
             "Prends un skate avec \011."sv, "Prends un skate avec \011."sv,
             "Nimm ein Skateboard mit \011."sv, "Nimm ein Skateboard mit \011."sv,
             "Vezmi si skateboard pomocí \011."sv, "Vezmi si skateboard pomocí \011."sv},
            {44, 54, 20, 22, kGateAnyVehicle, "You can move on with your skate without fear \000."sv, "You can move on with your skate without fear \013."sv,
             "Tu peux passer sans crainte avec ton skate \000."sv, "Tu peux passer sans crainte avec ton skate \013."sv,
             "Mit deinem Skateboard kommst du ohne Angst weiter \000."sv, "Mit deinem Skateboard kommst du ohne Angst weiter \013."sv,
             "Se skateboardem můžeš jet dál bez obav \000."sv, "Se skateboardem můžeš jet dál bez obav \013."sv},
            {63, 65, 20, 22, kGateAlways, "Your skate dislikes water! Jump!"sv, "Your skate dislikes water! Jump!"sv,
             "Ton skate n'aime pas l'eau ! Utilise \000 \010."sv, "Ton skate n'aime pas l'eau ! Utilise \013 \010."sv,
             "Dein Skateboard mag kein Wasser! Spring!"sv, "Dein Skateboard mag kein Wasser! Spring!"sv,
             "Tvůj skateboard nemá rád vodu! Skoč!"sv, "Tvůj skateboard nemá rád vodu! Skoč!"sv},
            {77, 81, 20, 22, kGateAnyVehicle, "Leave your skate with \011."sv, "Leave your skate with \011."sv,
             "Pose ton skate avec \011."sv, "Pose ton skate avec \011."sv,
             "Leg dein Skateboard mit \011 ab."sv, "Leg dein Skateboard mit \011 ab."sv,
             "Odlož skateboard pomocí \011."sv, "Odlož skateboard pomocí \011."sv},
        };

        // Mission 14 (Tables::table_training4, 5 real records).
        constexpr TrainingHintRecord kTraining4[] = {
            {7, 14, 0, 100, kGateNoDynamite, "Take the dynamite sticks with \011."sv, "Take the dynamite sticks with \011."sv,
             "Prends la dynamite avec \011."sv, "Prends la dynamite avec \011."sv,
             "Nimm das Dynamit mit \011."sv, "Nimm das Dynamit mit \011."sv,
             "Vezmi dynamit pomocí \011."sv, "Vezmi dynamit pomocí \011."sv},
            {7, 19, 0, 100, kGateHasDynamite, "Do not put down the dynamite here!"sv, "Do not put down the dynamite here!"sv,
             "Ne pose surtout pas la dynamite ici avec \011 !"sv, "Ne pose surtout pas la dynamite ici avec \011 !"sv,
             "Leg das Dynamit nicht hier ab!"sv, "Leg das Dynamit nicht hier ab!"sv,
             "Tady dynamit nepokládej!"sv, "Tady dynamit nepokládej!"sv},
            {20, 22, 0, 100, kGateNoDynamite, "Go and get the dynamite sticks on the left."sv, "Go and get the dynamite sticks on the left."sv,
             "Va chercher de la dynamite à gauche."sv, "Va chercher de la dynamite à gauche."sv,
             "Hol dir das Dynamit links."sv, "Hol dir das Dynamit links."sv,
             "Dojdi si pro dynamit vlevo."sv, "Dojdi si pro dynamit vlevo."sv},
            {20, 22, 0, 100, kGateHasDynamite, "Put down the dynamite with \011, then clear off!"sv, "Put down the dynamite with \011, then clear off!"sv,
             "Pose la dynamite avec \011, puis barre-toi !"sv, "Pose la dynamite avec \011, puis barre-toi !"sv,
             "Leg das Dynamit mit \011 ab, dann nichts wie weg!"sv, "Leg das Dynamit mit \011 ab, dann nichts wie weg!"sv,
             "Polož dynamit pomocí \011 a pak zmiz!"sv, "Polož dynamit pomocí \011 a pak zmiz!"sv},
            {27, 28, 42, 100, kGateAlways, "Put down another stick of dynamite here to move on."sv, "Put down another stick of dynamite here to move on."sv,
             "Pose encore de la dynamite pour passer."sv, "Pose encore de la dynamite pour passer."sv,
             "Leg hier noch ein Dynamit ab, um weiterzukommen."sv, "Leg hier noch ein Dynamit ab, um weiterzukommen."sv,
             "Polož sem další dynamit, abys prošel."sv, "Polož sem další dynamit, abys prošel."sv},
        };

        // The record's text and accelerometer variant in @p language.
        std::pair<std::string_view, std::string_view> Variant(const TrainingHintRecord& record, Language language) noexcept
        {
            switch (language)
            {
                case Language::French: return {record.textFr, record.accelTextFr};
                case Language::German: return {record.textDe, record.accelTextDe};
                case Language::Czech: return {record.textCs, record.accelTextCs};
                case Language::English: break;
            }
            return {record.text, record.accelText};
        }

        // The record's text in the current language (the accelerometer's variant with it on).
        std::string_view Text(const TrainingHintRecord& record, bool accel) noexcept
        {
            return accel ? Variant(record, CurrentLanguage()).second : Variant(record, CurrentLanguage()).first;
        }

        bool GateOpen(int gate, int treasuresCollected, bool inAnyVehicle, bool hasDynamite) noexcept
        {
            if (gate >= 0)
            {
                return treasuresCollected == gate;
            }
            switch (gate)
            {
                case kGateNoVehicle:
                    return !inAnyVehicle;
                case kGateAnyVehicle:
                    return inAnyVehicle;
                case kGateNoDynamite:
                    return !hasDynamite;
                case kGateHasDynamite:
                    return hasDynamite;
                default: // kGateAlways and anything unrecognized
                    return true;
            }
        }

        template <std::size_t N>
        std::string_view Scan(const TrainingHintRecord (&table)[N], int gridX, int gridZ,
                              int treasuresCollected, bool inAnyVehicle, bool hasDynamite, bool accel) noexcept
        {
            for (const auto& record : table)
            {
                if (gridX < record.colMin || gridX > record.colMax ||
                    gridZ < record.rowMin || gridZ > record.rowMax)
                {
                    continue;
                }
                if (!GateOpen(record.gate, treasuresCollected, inAnyVehicle, hasDynamite))
                {
                    continue;
                }
                // First rect+gate match wins (real source's own `break`),
                // even if its text is empty -- do not fall through.
                return Text(record, accel);
            }
            return {};
        }
    }

    namespace
    {
        template <std::size_t N>
        std::string_view ScanZones(const TrainingHintRecord (&table)[N], const std::vector<HintZoneRecord>& zones,
                                   int gridX, int gridY, int gridZ, int treasuresCollected, bool inAnyVehicle,
                                   bool hasDynamite, bool accel) noexcept
        {
            // In the table's own order, as the original scans its records.
            for (std::size_t index = 0; index < N; ++index)
            {
                const bool inside = std::any_of(zones.begin(), zones.end(), [&](const HintZoneRecord& zone)
                {
                    return zone.recordIndex == index && zone.Contains(gridX, gridY, gridZ);
                });
                if (!inside || !GateOpen(table[index].gate, treasuresCollected, inAnyVehicle, hasDynamite))
                {
                    continue;
                }
                return Text(table[index], accel);
            }
            return {};
        }
    }

    std::vector<std::string_view> AllTrainingHintTexts(Language language)
    {
        std::vector<std::string_view> texts;
        const auto add = [&](const auto& table) {
            for (const auto& record : table)
            {
                texts.push_back(Variant(record, language).first);
                texts.push_back(Variant(record, language).second);
            }
        };
        add(kTraining1);
        add(kTraining2);
        add(kTraining3);
        add(kTraining4);
        return texts;
    }

    std::string_view FindTrainingHintInZones(int mission, const std::vector<HintZoneRecord>& zones, int gridX,
                                             int gridY, int gridZ, int treasuresCollected, bool inAnyVehicle,
                                             bool hasDynamite, bool accelerometer) noexcept
    {
        switch (mission)
        {
            case 11:
                return ScanZones(kTraining1, zones, gridX, gridY, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            case 12:
                return ScanZones(kTraining2, zones, gridX, gridY, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            case 13:
                return ScanZones(kTraining3, zones, gridX, gridY, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            case 14:
                return ScanZones(kTraining4, zones, gridX, gridY, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            default:
                return {};
        }
    }

    std::string_view FindTrainingHint(int mission, int gridX, int gridZ,
                                      int treasuresCollected,
                                      bool inAnyVehicle, bool hasDynamite, bool accelerometer) noexcept
    {
        switch (mission)
        {
            case 11:
                return Scan(kTraining1, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            case 12:
                return Scan(kTraining2, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            case 13:
                return Scan(kTraining3, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            case 14:
                return Scan(kTraining4, gridX, gridZ, treasuresCollected, inAnyVehicle, hasDynamite, accelerometer);
            default:
                return {};
        }
    }
}
