#include "TrainingHints.hpp"

#include "Localization.hpp"

#include <algorithm>
#include <array>
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
            // The text and its accelerometer variant (+10000, the "a" strings, Decor.cpp:1287-1291) in each
            // language, by LanguageIndex: the original's English (MyResource, TX_TRAINING<mission><record>)
            // and French (MyResource::InitializeFR, transcribed with the owner's approval, Q-023,
            // 2026-11-11; its spelling corrected in two words, "Prend" -> "Prends", "décole" -> "décolle"),
            // the others translated for Galaxy (2026-11-12). Bytes 0-13 are text.png's pad pictograms (the
            // wheel's 8 directions, Jump, Action, Down, the tilted phone), drawn inline as the original's
            // Text::DrawChar does (BD-0064), where each language puts them.
            std::array<std::string_view, kLanguageCount> text;
            std::array<std::string_view, kLanguageCount> accelText;
        };

        // Mission 11 (Tables::table_training1, 22 real records). Real
        // source's own action-flag values: -2 (0 elsewhere in this table)
        // and 1 mean "treasuresCollected == 0" / "== 1" -- an exact-count
        // gate, not a threshold.
        constexpr TrainingHintRecord kTraining1[] = {
            {1, 3, 0, 50, kGateAlways,
             {"Use the directional wheel \000."sv, "Utilise la roue directionnelle \000."sv, "Benutze das Steuerrad \000."sv, "Použij směrové kolečko \000."sv, "Použi smerové koliesko \000."sv, "Usa la rueda de dirección \000."sv, "Usa la rotella direzionale \000."sv, "Użyj kółka kierunkowego \000."sv, "Use a roda direcional \000."sv},
             {"Tilt the phone \013."sv, "Incline le téléphone à droite \013."sv, "Neige das Telefon \013."sv, "Nakloň telefon \013."sv, "Nakloň telefón \013."sv, "Inclina el teléfono \013."sv, "Inclina il telefono \013."sv, "Przechyl telefon \013."sv, "Incline o telefone \013."sv}},
            {4, 4, 0, 50, 0,
             {"Press Jump \010."sv, "Appuie maintenant sur Saut \010."sv, "Drücke Springen \010."sv, "Stiskni Skok \010."sv, "Stlač Skok \010."sv, "Pulsa Saltar \010."sv, "Premi Salto \010."sv, "Naciśnij Skok \010."sv, "Aperte Pular \010."sv},
             {"Press Jump \010."sv, "Appuie maintenant sur Saut \010."sv, "Drücke Springen \010."sv, "Stiskni Skok \010."sv, "Stlač Skok \010."sv, "Pulsa Saltar \010."sv, "Premi Salto \010."sv, "Naciśnij Skok \010."sv, "Aperte Pular \010."sv}},
            {6, 6, 0, 50, 1,
             {"Press Right \000 and Jump \010."sv, "Appuie à droite sur la roue directionnelle \000 et sur Saut \010."sv, "Drücke Rechts \000 und Springen \010."sv, "Stiskni Vpravo \000 a Skok \010."sv, "Stlač Vpravo \000 a Skok \010."sv, "Pulsa Derecha \000 y Saltar \010."sv, "Premi Destra \000 e Salto \010."sv, "Naciśnij Prawo \000 i Skok \010."sv, "Aperte Direita \000 e Pular \010."sv},
             {"\013 and Jump \010."sv, "\013 et appuie sur Saut \010."sv, "\013 und Springen \010."sv, "\013 a Skok \010."sv, "\013 a Skok \010."sv, "\013 y Saltar \010."sv, "\013 e Salto \010."sv, "\013 i Skok \010."sv, "\013 e Pular \010."sv}},
            {9, 9, 0, 50, kGateAlways,
             {"Press Right \000 and Jump \010."sv, "Appuie sur Droite \000 et Saut \010."sv, "Drücke Rechts \000 und Springen \010."sv, "Stiskni Vpravo \000 a Skok \010."sv, "Stlač Vpravo \000 a Skok \010."sv, "Pulsa Derecha \000 y Saltar \010."sv, "Premi Destra \000 e Salto \010."sv, "Naciśnij Prawo \000 i Skok \010."sv, "Aperte Direita \000 e Pular \010."sv},
             {"\013 and Jump \010."sv, "\013 et Saut \010."sv, "\013 und Springen \010."sv, "\013 a Skok \010."sv, "\013 a Skok \010."sv, "\013 y Saltar \010."sv, "\013 e Salto \010."sv, "\013 i Skok \010."sv, "\013 e Pular \010."sv}},
            {12, 14, 0, 50, 1,
             {"Don't fall into the water \000 \010 !"sv, "Essaie de ne pas te mouiller avec \000 \010 !"sv, "Fall nicht ins Wasser \000 \010!"sv, "Nespadni do vody \000 \010!"sv, "Nespadni do vody \000 \010!"sv, "¡No te caigas al agua \000 \010!"sv, "Non cadere nell'acqua \000 \010!"sv, "Nie wpadnij do wody \000 \010!"sv, "Não caia na água \000 \010!"sv},
             {"Don't fall into the water \013 \010 !"sv, "Essaie de ne pas te mouiller avec \013 \010 !"sv, "Fall nicht ins Wasser \013 \010!"sv, "Nespadni do vody \013 \010!"sv, "Nespadni do vody \013 \010!"sv, "¡No te caigas al agua \013 \010!"sv, "Non cadere nell'acqua \013 \010!"sv, "Nie wpadnij do wody \013 \010!"sv, "Não caia na água \013 \010!"sv}},
            {16, 16, 0, 50, kGateAlways,
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv},
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv}},
            {20, 21, 0, 50, kGateAlways,
             {"Take the elevator quietly, without jumping \000."sv, "Prends l'ascenseur calmement, sans sauter \000."sv, "Nimm den Aufzug ruhig, ohne zu springen \000."sv, "Jeď výtahem klidně, bez skákání \000."sv, "Choď výťahom pokojne, bez skákania \000."sv, "Toma el ascensor con calma, sin saltar \000."sv, "Prendi l'ascensore con calma, senza saltare \000."sv, "Jedź windą spokojnie, bez skakania \000."sv, "Pegue o elevador com calma, sem pular \000."sv},
             {"Take the elevator quietly, without jumping \013."sv, "Prends l'ascenseur calmement, sans sauter \013."sv, "Nimm den Aufzug ruhig, ohne zu springen \013."sv, "Jeď výtahem klidně, bez skákání \013."sv, "Choď výťahom pokojne, bez skákania \013."sv, "Toma el ascensor con calma, sin saltar \013."sv, "Prendi l'ascensore con calma, senza saltare \013."sv, "Jedź windą spokojnie, bez skakania \013."sv, "Pegue o elevador com calma, sem pular \013."sv}},
            {23, 24, 0, 50, kGateAlways,
             {"Jump on the elevator."sv, "Saute sur l'ascenseur."sv, "Spring auf den Aufzug."sv, "Skoč na výtah."sv, "Skoč na výťah."sv, "Salta al ascensor."sv, "Salta sull'ascensore."sv, "Wskocz do windy."sv, "Pule no elevador."sv},
             {"Jump on the elevator."sv, "Saute sur l'ascenseur."sv, "Spring auf den Aufzug."sv, "Skoč na výtah."sv, "Skoč na výťah."sv, "Salta al ascensor."sv, "Salta sull'ascensore."sv, "Wskocz do windy."sv, "Pule no elevador."sv}},
            {27, 27, 0, 50, kGateAlways,
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv},
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv}},
            {28, 28, 0, 50, kGateAlways,
             {"Move forward without jumping nor stopping \000 !"sv, "Avance sans sauter et sans t'arrêter \000 !"sv, "Geh weiter, ohne zu springen oder anzuhalten \000!"sv, "Jdi dál bez skákání a bez zastavení \000!"sv, "Choď ďalej bez skákania a bez zastavenia \000!"sv, "¡Avanza sin saltar ni pararte \000!"sv, "Avanza senza saltare e senza fermarti \000!"sv, "Idź naprzód bez skakania i bez zatrzymywania \000!"sv, "Avance sem pular e sem parar \000!"sv},
             {"Move forward without jumping nor stopping \013 !"sv, "Avance sans sauter et sans t'arrêter \013 !"sv, "Geh weiter, ohne zu springen oder anzuhalten \013!"sv, "Jdi dál bez skákání a bez zastavení \013!"sv, "Choď ďalej bez skákania a bez zastavenia \013!"sv, "¡Avanza sin saltar ni pararte \013!"sv, "Avanza senza saltare e senza fermarti \013!"sv, "Idź naprzód bez skakania i bez zatrzymywania \013!"sv, "Avance sem pular e sem parar \013!"sv}},
            {30, 31, 0, 50, kGateAlways,
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv},
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv}},
            {36, 36, 0, 50, kGateAlways,
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv},
             {""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv}},
            {39, 39, 0, 50, kGateAlways,
             {"Move forward on the platform \000."sv, "Avance sur la plateforme \000."sv, "Geh auf die Plattform \000."sv, "Vstup na plošinu \000."sv, "Vstúp na plošinu \000."sv, "Avanza sobre la plataforma \000."sv, "Avanza sulla piattaforma \000."sv, "Wejdź na platformę \000."sv, "Avance na plataforma \000."sv},
             {"Move forward on the platform \013."sv, "Avance sur la plateforme \013."sv, "Geh auf die Plattform \013."sv, "Vstup na plošinu \013."sv, "Vstúp na plošinu \013."sv, "Avanza sobre la plataforma \013."sv, "Avanza sulla piattaforma \013."sv, "Wejdź na platformę \013."sv, "Avance na plataforma \013."sv}},
            {44, 44, 0, 50, kGateAlways,
             {"Leave the platform \000."sv, "Quitte la plateforme \000."sv, "Verlass die Plattform \000."sv, "Opusť plošinu \000."sv, "Opusti plošinu \000."sv, "Sal de la plataforma \000."sv, "Lascia la piattaforma \000."sv, "Zejdź z platformy \000."sv, "Saia da plataforma \000."sv},
             {"Leave the platform \013."sv, "Quitte la plateforme \013."sv, "Verlass die Plattform \013."sv, "Opusť plošinu \013."sv, "Opusti plošinu \013."sv, "Sal de la plataforma \013."sv, "Lascia la piattaforma \013."sv, "Zejdź z platformy \013."sv, "Saia da plataforma \013."sv}},
            {46, 46, 0, 50, kGateAlways,
             {"Once again, but faster..."sv, "Encore une fois, mais plus vite..."sv, "Noch einmal, aber schneller..."sv, "Ještě jednou, ale rychleji..."sv, "Ešte raz, ale rýchlejšie..."sv, "Otra vez, pero más rápido..."sv, "Ancora una volta, ma più veloce..."sv, "Jeszcze raz, ale szybciej..."sv, "Mais uma vez, mas mais rápido..."sv},
             {"Once again, but faster..."sv, "Encore une fois, mais plus vite..."sv, "Noch einmal, aber schneller..."sv, "Ještě jednou, ale rychleji..."sv, "Ešte raz, ale rýchlejšie..."sv, "Otra vez, pero más rápido..."sv, "Ancora una volta, ma più veloce..."sv, "Jeszcze raz, ale szybciej..."sv, "Mais uma vez, mas mais rápido..."sv}},
            {53, 53, 0, 50, kGateAlways,
             {"Move forward on the platform \000, then jump \000 \010."sv, "Avance sur la plateforme \000 puis saute \000 \010."sv, "Geh auf die Plattform \000, dann spring \000 \010."sv, "Vstup na plošinu \000, pak skoč \000 \010."sv, "Vstúp na plošinu \000, potom skoč \000 \010."sv, "Avanza sobre la plataforma \000 y luego salta \000 \010."sv, "Avanza sulla piattaforma \000, poi salta \000 \010."sv, "Wejdź na platformę \000, potem skocz \000 \010."sv, "Avance na plataforma \000 e depois pule \000 \010."sv},
             {"Move forward on the platform \013, then jump \013 \010."sv, "Avance sur la plateforme \013 puis saute \013 \010."sv, "Geh auf die Plattform \013, dann spring \013 \010."sv, "Vstup na plošinu \013, pak skoč \013 \010."sv, "Vstúp na plošinu \013, potom skoč \013 \010."sv, "Avanza sobre la plataforma \013 y luego salta \013 \010."sv, "Avanza sulla piattaforma \013, poi salta \013 \010."sv, "Wejdź na platformę \013, potem skocz \013 \010."sv, "Avance na plataforma \013 e depois pule \013 \010."sv}},
            {56, 56, 0, 50, kGateAlways,
             {"Jump when you are on the platform \000 \010."sv, "Saute lorsque tu es sur la plateforme \000 \010."sv, "Spring, wenn du auf der Plattform bist \000 \010."sv, "Skoč, až budeš na plošině \000 \010."sv, "Skoč, keď budeš na plošine \000 \010."sv, "Salta cuando estés sobre la plataforma \000 \010."sv, "Salta quando sei sulla piattaforma \000 \010."sv, "Skocz, gdy będziesz na platformie \000 \010."sv, "Pule quando estiver na plataforma \000 \010."sv},
             {"Jump when you are on the platform \013 \010."sv, "Saute lorsque tu es sur la plateforme \013 \010."sv, "Spring, wenn du auf der Plattform bist \013 \010."sv, "Skoč, až budeš na plošině \013 \010."sv, "Skoč, keď budeš na plošine \013 \010."sv, "Salta cuando estés sobre la plataforma \013 \010."sv, "Salta quando sei sulla piattaforma \013 \010."sv, "Skocz, gdy będziesz na platformie \013 \010."sv, "Pule quando estiver na plataforma \013 \010."sv}},
            {62, 64, 0, 50, kGateAlways,
             {"Choose the upper path \000 \010."sv, "Passe par en haut \000 \010."sv, "Nimm den oberen Weg \000 \010."sv, "Vyber horní cestu \000 \010."sv, "Vyber hornú cestu \000 \010."sv, "Elige el camino de arriba \000 \010."sv, "Scegli la strada in alto \000 \010."sv, "Wybierz górną drogę \000 \010."sv, "Escolha o caminho de cima \000 \010."sv},
             {"Choose the upper path \013 \010."sv, "Passe par en haut \013 \010."sv, "Nimm den oberen Weg \013 \010."sv, "Vyber horní cestu \013 \010."sv, "Vyber hornú cestu \013 \010."sv, "Elige el camino de arriba \013 \010."sv, "Scegli la strada in alto \013 \010."sv, "Wybierz górną drogę \013 \010."sv, "Escolha o caminho de cima \013 \010."sv}},
            {65, 66, 0, 50, kGateAlways,
             {"Eggs give you extra lives."sv, "Les oeufs te redonnent des vies."sv, "Eier geben dir zusätzliche Leben."sv, "Vajíčka ti dají další životy."sv, "Vajíčka ti dajú ďalšie životy."sv, "Los huevos te dan vidas extra."sv, "Le uova ti danno vite in più."sv, "Jajka dają ci dodatkowe życia."sv, "Os ovos dão vidas extras."sv},
             {"Eggs give you extra lives."sv, "Les oeufs te redonnent des vies."sv, "Eier geben dir zusätzliche Leben."sv, "Vajíčka ti dají další životy."sv, "Vajíčka ti dajú ďalšie životy."sv, "Los huevos te dan vidas extra."sv, "Le uova ti danno vite in più."sv, "Jajka dają ci dodatkowe życia."sv, "Os ovos dão vidas extras."sv}},
            {69, 74, 0, 50, kGateAlways,
             {"Once on the top, move forward on the other platform without delay..."sv, "Arrivé en haut, avance sur l'autre plateforme sans tarder..."sv, "Oben angekommen, geh ohne Zögern auf die andere Plattform..."sv, "Až budeš nahoře, vstup bez otálení na druhou plošinu..."sv, "Keď budeš hore, vstúp bez váhania na druhú plošinu..."sv, "Una vez arriba, avanza sin tardar a la otra plataforma..."sv, "Una volta in cima, avanza subito sull'altra piattaforma..."sv, "Na górze bez zwłoki wejdź na drugą platformę..."sv, "Lá em cima, avance sem demora para a outra plataforma..."sv},
             {"Once on the top, move forward on the other platform without delay..."sv, "Arrivé en haut, avance sur l'autre plateforme sans tarder..."sv, "Oben angekommen, geh ohne Zögern auf die andere Plattform..."sv, "Až budeš nahoře, vstup bez otálení na druhou plošinu..."sv, "Keď budeš hore, vstúp bez váhania na druhú plošinu..."sv, "Una vez arriba, avanza sin tardar a la otra plataforma..."sv, "Una volta in cima, avanza subito sull'altra piattaforma..."sv, "Na górze bez zwłoki wejdź na drugą platformę..."sv, "Lá em cima, avance sem demora para a outra plataforma..."sv}},
            {80, 85, 0, 50, 1,
             {"Catch the second and last treasure."sv, "Attrape le deuxième et dernier trésor."sv, "Hol dir den zweiten und letzten Schatz."sv, "Seber druhý a poslední poklad."sv, "Zober druhý a posledný poklad."sv, "Coge el segundo y último tesoro."sv, "Prendi il secondo e ultimo tesoro."sv, "Zdobądź drugi i ostatni skarb."sv, "Pegue o segundo e último tesouro."sv},
             {"Catch the second and last treasure."sv, "Attrape le deuxième et dernier trésor."sv, "Hol dir den zweiten und letzten Schatz."sv, "Seber druhý a poslední poklad."sv, "Zober druhý a posledný poklad."sv, "Coge el segundo y último tesoro."sv, "Prendi il secondo e ultimo tesoro."sv, "Zdobądź drugi i ostatni skarb."sv, "Pegue o segundo e último tesouro."sv}},
            {87, 93, 0, 50, kGateAlways,
             {"Join the red arrow."sv, "Pour terminer, va sur la flèche rouge."sv, "Geh zum roten Pfeil."sv, "Jdi k červené šipce."sv, "Choď k červenej šípke."sv, "Ve a la flecha roja."sv, "Raggiungi la freccia rossa."sv, "Idź do czerwonej strzałki."sv, "Vá até a seta vermelha."sv},
             {"Join the red arrow."sv, "Pour terminer, va sur la flèche rouge."sv, "Geh zum roten Pfeil."sv, "Jdi k červené šipce."sv, "Choď k červenej šípke."sv, "Ve a la flecha roja."sv, "Raggiungi la freccia rossa."sv, "Idź do czerwonej strzałki."sv, "Vá até a seta vermelha."sv}},
        };

        // Mission 12 (Tables::table_training2, 5 real records, all
        // unconditional).
        constexpr TrainingHintRecord kTraining2[] = {
            {9, 15, 0, 100, kGateAlways,
             {"Push the box forward until the red dot with \000."sv, "Pousse la caisse sur le point rouge avec \000."sv, "Schieb die Kiste bis zum roten Punkt mit \000."sv, "Dotlač bednu na červenou tečku pomocí \000."sv, "Dotlač debnu na červenú bodku pomocou \000."sv, "Empuja la caja hasta el punto rojo con \000."sv, "Spingi la cassa fino al punto rosso con \000."sv, "Pchnij skrzynię aż do czerwonej kropki za pomocą \000."sv, "Empurre a caixa até o ponto vermelho com \000."sv},
             {"Push the box forward until the red dot with \013."sv, "Pousse la caisse sur le point rouge avec \013."sv, "Schieb die Kiste bis zum roten Punkt mit \013."sv, "Dotlač bednu na červenou tečku pomocí \013."sv, "Dotlač debnu na červenú bodku pomocou \013."sv, "Empuja la caja hasta el punto rojo con \013."sv, "Spingi la cassa fino al punto rosso con \013."sv, "Pchnij skrzynię aż do czerwonej kropki za pomocą \013."sv, "Empurre a caixa até o ponto vermelho com \013."sv}},
            {16, 16, 0, 100, kGateAlways,
             {"Practical, right?"sv, "Pratique, cette caisse, non ?"sv, "Praktisch, oder?"sv, "Praktické, ne?"sv, "Praktické, nie?"sv, "Práctico, ¿verdad?"sv, "Pratico, vero?"sv, "Praktyczne, prawda?"sv, "Prático, não é?"sv},
             {"Practical, right?"sv, "Pratique, cette caisse, non ?"sv, "Praktisch, oder?"sv, "Praktické, ne?"sv, "Praktické, nie?"sv, "Práctico, ¿verdad?"sv, "Pratico, vero?"sv, "Praktyczne, prawda?"sv, "Prático, não é?"sv}},
            {19, 21, 0, 100, kGateAlways,
             {"Pull the box backward until the red dot with \003."sv, "Tire la caisse sur le point rouge avec \003."sv, "Zieh die Kiste bis zum roten Punkt mit \003."sv, "Přitáhni bednu na červenou tečku pomocí \003."sv, "Pritiahni debnu na červenú bodku pomocou \003."sv, "Tira de la caja hasta el punto rojo con \003."sv, "Tira la cassa fino al punto rosso con \003."sv, "Przyciągnij skrzynię do czerwonej kropki za pomocą \003."sv, "Puxe a caixa até o ponto vermelho com \003."sv},
             {"Pull the box backward until the red dot with \014 and \012."sv, "Tire la caisse sur le point rouge avec \014 et \012."sv, "Zieh die Kiste bis zum roten Punkt mit \014 und \012."sv, "Přitáhni bednu na červenou tečku pomocí \014 a \012."sv, "Pritiahni debnu na červenú bodku pomocou \014 a \012."sv, "Tira de la caja hasta el punto rojo con \014 y \012."sv, "Tira la cassa fino al punto rosso con \014 e \012."sv, "Przyciągnij skrzynię do czerwonej kropki za pomocą \014 i \012."sv, "Puxe a caixa até o ponto vermelho com \014 e \012."sv}},
            {24, 31, 0, 100, kGateAlways,
             {"Stack both boxes up on the red dot to move on."sv, "Empile les 2 caisses sur le point rouge pour passer."sv, "Stapel beide Kisten auf dem roten Punkt, um weiterzukommen."sv, "Postav obě bedny na sebe na červenou tečku, abys prošel."sv, "Polož obe debny na seba na červenú bodku, aby si prešiel."sv, "Apila las dos cajas sobre el punto rojo para pasar."sv, "Impila le due casse sul punto rosso per passare."sv, "Ustaw obie skrzynie jedna na drugiej na czerwonej kropce, aby przejść."sv, "Empilhe as duas caixas no ponto vermelho para passar."sv},
             {"Stack both boxes up on the red dot to move on."sv, "Empile les 2 caisses sur le point rouge pour passer."sv, "Stapel beide Kisten auf dem roten Punkt, um weiterzukommen."sv, "Postav obě bedny na sebe na červenou tečku, abys prošel."sv, "Polož obe debny na seba na červenú bodku, aby si prešiel."sv, "Apila las dos cajas sobre el punto rojo para pasar."sv, "Impila le due casse sul punto rosso per passare."sv, "Ustaw obie skrzynie jedna na drugiej na czerwonej kropce, aby przejść."sv, "Empilhe as duas caixas no ponto vermelho para passar."sv}},
            {33, 40, 0, 100, kGateAlways,
             {"Stack the three boxes up on the red dot to move on."sv, "Empile 3 caisses sur le point rouge pour passer."sv, "Stapel die drei Kisten auf dem roten Punkt, um weiterzukommen."sv, "Postav tři bedny na sebe na červenou tečku, abys prošel."sv, "Polož tri debny na seba na červenú bodku, aby si prešiel."sv, "Apila las tres cajas sobre el punto rojo para pasar."sv, "Impila le tre casse sul punto rosso per passare."sv, "Ustaw trzy skrzynie jedna na drugiej na czerwonej kropce, aby przejść."sv, "Empilhe as três caixas no ponto vermelho para passar."sv},
             {"Stack the three boxes up on the red dot to move on."sv, "Empile 3 caisses sur le point rouge pour passer."sv, "Stapel die drei Kisten auf dem roten Punkt, um weiterzukommen."sv, "Postav tři bedny na sebe na červenou tečku, abys prošel."sv, "Polož tri debny na seba na červenú bodku, aby si prešiel."sv, "Apila las tres cajas sobre el punto rojo para pasar."sv, "Impila le tre casse sul punto rosso per passare."sv, "Ustaw trzy skrzynie jedna na drugiej na czerwonej kropce, aby przejść."sv, "Empilhe as três caixas no ponto vermelho para passar."sv}},
        };

        // Mission 13 (Tables::table_training3, 11 real records). Real
        // source's -2/-3 gates only ever test Helicopter/Skateboard/Tank
        // vs. Jeep to resolve to the same "in ANY vehicle at all" result
        // either way (Decor.cpp:1319-1332) -- collapsed to a single
        // inAnyVehicle bool here, not a Jeep-specific distinction.
        constexpr TrainingHintRecord kTraining3[] = {
            {16, 24, 36, 40, kGateNoVehicle,
             {"Take a helicopter with \011."sv, "Prends un hélico avec \011."sv, "Nimm einen Hubschrauber mit \011."sv, "Nastup do vrtulníku pomocí \011."sv, "Nastúp do vrtuľníka pomocou \011."sv, "Sube a un helicóptero con \011."sv, "Prendi un elicottero con \011."sv, "Wsiądź do helikoptera za pomocą \011."sv, "Entre num helicóptero com \011."sv},
             {"Take a helicopter with \011."sv, "Prends un hélico avec \011."sv, "Nimm einen Hubschrauber mit \011."sv, "Nastup do vrtulníku pomocí \011."sv, "Nastúp do vrtuľníka pomocou \011."sv, "Sube a un helicóptero con \011."sv, "Prendi un elicottero con \011."sv, "Wsiądź do helikoptera za pomocą \011."sv, "Entre num helicóptero com \011."sv}},
            {16, 24, 36, 40, kGateAnyVehicle,
             {"Use \006 or \010 to take off. Direct with \004 and \000."sv, "Utilise \006 ou \010 pour décoller. Dirige avec \004 et \000."sv, "Benutze \006 oder \010 zum Abheben. Steuere mit \004 und \000."sv, "Vzlétni pomocí \006 nebo \010. Řiď pomocí \004 a \000."sv, "Vzlietni pomocou \006 alebo \010. Riaď pomocou \004 a \000."sv, "Usa \006 o \010 para despegar. Dirige con \004 y \000."sv, "Usa \006 o \010 per decollare. Dirigi con \004 e \000."sv, "Wystartuj za pomocą \006 lub \010. Steruj za pomocą \004 i \000."sv, "Use \006 ou \010 para decolar. Dirija com \004 e \000."sv},
             {"Use \010 to take off. Direct with \014 and \013."sv, "Utilise \010 pour décoller. Dirige avec \014 et \013."sv, "Benutze \010 zum Abheben. Steuere mit \014 und \013."sv, "Vzlétni pomocí \010. Řiď pomocí \014 a \013."sv, "Vzlietni pomocou \010. Riaď pomocou \014 a \013."sv, "Usa \010 para despegar. Dirige con \014 y \013."sv, "Usa \010 per decollare. Dirigi con \014 e \013."sv, "Wystartuj za pomocą \010. Steruj za pomocą \014 i \013."sv, "Use \010 para decolar. Dirija com \014 e \013."sv}},
            {22, 25, 34, 34, kGateAnyVehicle,
             {"Leave the helicopter with \011, it dislikes water!"sv, "Quitte l'hélico avec \011, il n'aime pas l'eau !"sv, "Verlass den Hubschrauber mit \011, er mag kein Wasser!"sv, "Vystup z vrtulníku pomocí \011, nemá rád vodu!"sv, "Vystúp z vrtuľníka pomocou \011, nemá rád vodu!"sv, "Baja del helicóptero con \011, ¡no le gusta el agua!"sv, "Lascia l'elicottero con \011, non gli piace l'acqua!"sv, "Wysiądź z helikoptera za pomocą \011, nie lubi wody!"sv, "Saia do helicóptero com \011, ele não gosta de água!"sv},
             {"Leave the helicopter with \011, it dislikes water!"sv, "Quitte l'hélico avec \011, il n'aime pas l'eau !"sv, "Verlass den Hubschrauber mit \011, er mag kein Wasser!"sv, "Vystup z vrtulníku pomocí \011, nemá rád vodu!"sv, "Vystúp z vrtuľníka pomocou \011, nemá rád vodu!"sv, "Baja del helicóptero con \011, ¡no le gusta el agua!"sv, "Lascia l'elicottero con \011, non gli piace l'acqua!"sv, "Wysiądź z helikoptera za pomocą \011, nie lubi wody!"sv, "Saia do helicóptero com \011, ele não gosta de água!"sv}},
            {22, 31, 34, 37, kGateNoVehicle,
             {"Plunge. Use \004 \000 \002 \006 to direct."sv, "Plonge. Utilise \004 \000 \002 \006 pour te diriger."sv, "Tauch. Steuere mit \004 \000 \002 \006."sv, "Ponoř se. Řiď pomocí \004 \000 \002 \006."sv, "Ponor sa. Riaď pomocou \004 \000 \002 \006."sv, "Bucea. Usa \004 \000 \002 \006 para dirigirte."sv, "Tuffati. Usa \004 \000 \002 \006 per dirigerti."sv, "Zanurkuj. Steruj za pomocą \004 \000 \002 \006."sv, "Mergulhe. Use \004 \000 \002 \006 para se dirigir."sv},
             {"Plunge. Use \014 \013 \010 \012 to direct."sv, "Plonge. Utilise \014 \013 \010 \012 pour te diriger."sv, "Tauch. Steuere mit \014 \013 \010 \012."sv, "Ponoř se. Řiď pomocí \014 \013 \010 \012."sv, "Ponor sa. Riaď pomocou \014 \013 \010 \012."sv, "Bucea. Usa \014 \013 \010 \012 para dirigirte."sv, "Tuffati. Usa \014 \013 \010 \012 per dirigerti."sv, "Zanurkuj. Steruj za pomocą \014 \013 \010 \012."sv, "Mergulhe. Use \014 \013 \010 \012 para se dirigir."sv}},
            {32, 43, 34, 34, kGateNoVehicle,
             {"Take a helicopter \011 then take off with \006 or \010."sv, "Prends un hélico \011 puis décolle avec \006 ou \010."sv, "Nimm einen Hubschrauber \011 und heb dann mit \006 oder \010 ab."sv, "Nastup do vrtulníku \011 a vzlétni pomocí \006 nebo \010."sv, "Nastúp do vrtuľníka \011 a vzlietni pomocou \006 alebo \010."sv, "Sube a un helicóptero \011 y despega con \006 o \010."sv, "Prendi un elicottero \011 poi decolla con \006 o \010."sv, "Wsiądź do helikoptera \011 i wystartuj za pomocą \006 lub \010."sv, "Entre num helicóptero \011 e decole com \006 ou \010."sv},
             {"Take a helicopter \011 then take off with \010."sv, "Prends un hélico \011 puis décolle avec \010."sv, "Nimm einen Hubschrauber \011 und heb dann mit \010 ab."sv, "Nastup do vrtulníku \011 a vzlétni pomocí \010."sv, "Nastúp do vrtuľníka \011 a vzlietni pomocou \010."sv, "Sube a un helicóptero \011 y despega con \010."sv, "Prendi un elicottero \011 poi decolla con \010."sv, "Wsiądź do helikoptera \011 i wystartuj za pomocą \010."sv, "Entre num helicóptero \011 e decole com \010."sv}},
            {32, 43, 26, 34, kGateAnyVehicle,
             {"Grab the three treasures, then go up."sv, "Attrape les 3 trésors puis monte."sv, "Schnapp dir die drei Schätze, dann flieg hoch."sv, "Seber tři poklady a pak leť nahoru."sv, "Zober tri poklady a potom leť hore."sv, "Coge los tres tesoros y luego sube."sv, "Prendi i tre tesori, poi sali."sv, "Zbierz trzy skarby, potem leć w górę."sv, "Pegue os três tesouros e depois suba."sv},
             {"Grab the three treasures, then go up."sv, "Attrape les 3 trésors puis monte."sv, "Schnapp dir die drei Schätze, dann flieg hoch."sv, "Seber tři poklady a pak leť nahoru."sv, "Zober tri poklady a potom leť hore."sv, "Coge los tres tesoros y luego sube."sv, "Prendi i tre tesori, poi sali."sv, "Zbierz trzy skarby, potem leć w górę."sv, "Pegue os três tesouros e depois suba."sv}},
            {44, 48, 20, 22, kGateNoVehicle,
             {"Go and get a skate in the top left corner."sv, "Pour passer, va chercher un skate en haut à gauche."sv, "Hol dir ein Skateboard oben links."sv, "Dojdi si pro skateboard vlevo nahoře."sv, "Choď si po skateboard vľavo hore."sv, "Ve a buscar un monopatín arriba a la izquierda."sv, "Vai a prendere uno skate in alto a sinistra."sv, "Idź po deskorolkę w lewym górnym rogu."sv, "Vá buscar um skate no canto de cima à esquerda."sv},
             {"Go and get a skate in the top left corner."sv, "Pour passer, va chercher un skate en haut à gauche."sv, "Hol dir ein Skateboard oben links."sv, "Dojdi si pro skateboard vlevo nahoře."sv, "Choď si po skateboard vľavo hore."sv, "Ve a buscar un monopatín arriba a la izquierda."sv, "Vai a prendere uno skate in alto a sinistra."sv, "Idź po deskorolkę w lewym górnym rogu."sv, "Vá buscar um skate no canto de cima à esquerda."sv}},
            {29, 31, 12, 12, kGateAlways,
             {"Take a skate with \011."sv, "Prends un skate avec \011."sv, "Nimm ein Skateboard mit \011."sv, "Vezmi si skateboard pomocí \011."sv, "Zober si skateboard pomocou \011."sv, "Coge un monopatín con \011."sv, "Prendi uno skate con \011."sv, "Weź deskorolkę za pomocą \011."sv, "Pegue um skate com \011."sv},
             {"Take a skate with \011."sv, "Prends un skate avec \011."sv, "Nimm ein Skateboard mit \011."sv, "Vezmi si skateboard pomocí \011."sv, "Zober si skateboard pomocou \011."sv, "Coge un monopatín con \011."sv, "Prendi uno skate con \011."sv, "Weź deskorolkę za pomocą \011."sv, "Pegue um skate com \011."sv}},
            {44, 54, 20, 22, kGateAnyVehicle,
             {"You can move on with your skate without fear \000."sv, "Tu peux passer sans crainte avec ton skate \000."sv, "Mit deinem Skateboard kommst du ohne Angst weiter \000."sv, "Se skateboardem můžeš jet dál bez obav \000."sv, "So skateboardom môžeš ísť ďalej bez obáv \000."sv, "Con tu monopatín puedes pasar sin miedo \000."sv, "Con il tuo skate puoi passare senza paura \000."sv, "Na deskorolce możesz jechać dalej bez obaw \000."sv, "Com o seu skate você pode passar sem medo \000."sv},
             {"You can move on with your skate without fear \013."sv, "Tu peux passer sans crainte avec ton skate \013."sv, "Mit deinem Skateboard kommst du ohne Angst weiter \013."sv, "Se skateboardem můžeš jet dál bez obav \013."sv, "So skateboardom môžeš ísť ďalej bez obáv \013."sv, "Con tu monopatín puedes pasar sin miedo \013."sv, "Con il tuo skate puoi passare senza paura \013."sv, "Na deskorolce możesz jechać dalej bez obaw \013."sv, "Com o seu skate você pode passar sem medo \013."sv}},
            {63, 65, 20, 22, kGateAlways,
             {"Your skate dislikes water! Jump!"sv, "Ton skate n'aime pas l'eau ! Utilise \000 \010."sv, "Dein Skateboard mag kein Wasser! Spring!"sv, "Tvůj skateboard nemá rád vodu! Skoč!"sv, "Tvoj skateboard nemá rád vodu! Skoč!"sv, "¡A tu monopatín no le gusta el agua! ¡Salta!"sv, "Al tuo skate non piace l'acqua! Salta!"sv, "Twoja deskorolka nie lubi wody! Skacz!"sv, "O seu skate não gosta de água! Pule!"sv},
             {"Your skate dislikes water! Jump!"sv, "Ton skate n'aime pas l'eau ! Utilise \013 \010."sv, "Dein Skateboard mag kein Wasser! Spring!"sv, "Tvůj skateboard nemá rád vodu! Skoč!"sv, "Tvoj skateboard nemá rád vodu! Skoč!"sv, "¡A tu monopatín no le gusta el agua! ¡Salta!"sv, "Al tuo skate non piace l'acqua! Salta!"sv, "Twoja deskorolka nie lubi wody! Skacz!"sv, "O seu skate não gosta de água! Pule!"sv}},
            {77, 81, 20, 22, kGateAnyVehicle,
             {"Leave your skate with \011."sv, "Pose ton skate avec \011."sv, "Leg dein Skateboard mit \011 ab."sv, "Odlož skateboard pomocí \011."sv, "Odlož skateboard pomocou \011."sv, "Deja tu monopatín con \011."sv, "Lascia il tuo skate con \011."sv, "Zostaw deskorolkę za pomocą \011."sv, "Largue o seu skate com \011."sv},
             {"Leave your skate with \011."sv, "Pose ton skate avec \011."sv, "Leg dein Skateboard mit \011 ab."sv, "Odlož skateboard pomocí \011."sv, "Odlož skateboard pomocou \011."sv, "Deja tu monopatín con \011."sv, "Lascia il tuo skate con \011."sv, "Zostaw deskorolkę za pomocą \011."sv, "Largue o seu skate com \011."sv}},
        };

        // Mission 14 (Tables::table_training4, 5 real records).
        constexpr TrainingHintRecord kTraining4[] = {
            {7, 14, 0, 100, kGateNoDynamite,
             {"Take the dynamite sticks with \011."sv, "Prends la dynamite avec \011."sv, "Nimm das Dynamit mit \011."sv, "Vezmi dynamit pomocí \011."sv, "Zober dynamit pomocou \011."sv, "Coge la dinamita con \011."sv, "Prendi la dinamite con \011."sv, "Weź dynamit za pomocą \011."sv, "Pegue a dinamite com \011."sv},
             {"Take the dynamite sticks with \011."sv, "Prends la dynamite avec \011."sv, "Nimm das Dynamit mit \011."sv, "Vezmi dynamit pomocí \011."sv, "Zober dynamit pomocou \011."sv, "Coge la dinamita con \011."sv, "Prendi la dinamite con \011."sv, "Weź dynamit za pomocą \011."sv, "Pegue a dinamite com \011."sv}},
            {7, 19, 0, 100, kGateHasDynamite,
             {"Do not put down the dynamite here!"sv, "Ne pose surtout pas la dynamite ici avec \011 !"sv, "Leg das Dynamit nicht hier ab!"sv, "Tady dynamit nepokládej!"sv, "Tu dynamit neklaď!"sv, "¡No dejes la dinamita aquí!"sv, "Non mettere la dinamite qui!"sv, "Nie kładź tu dynamitu!"sv, "Não largue a dinamite aqui!"sv},
             {"Do not put down the dynamite here!"sv, "Ne pose surtout pas la dynamite ici avec \011 !"sv, "Leg das Dynamit nicht hier ab!"sv, "Tady dynamit nepokládej!"sv, "Tu dynamit neklaď!"sv, "¡No dejes la dinamita aquí!"sv, "Non mettere la dinamite qui!"sv, "Nie kładź tu dynamitu!"sv, "Não largue a dinamite aqui!"sv}},
            {20, 22, 0, 100, kGateNoDynamite,
             {"Go and get the dynamite sticks on the left."sv, "Va chercher de la dynamite à gauche."sv, "Hol dir das Dynamit links."sv, "Dojdi si pro dynamit vlevo."sv, "Choď si po dynamit vľavo."sv, "Ve a buscar la dinamita a la izquierda."sv, "Vai a prendere la dinamite a sinistra."sv, "Idź po dynamit po lewej."sv, "Vá buscar a dinamite à esquerda."sv},
             {"Go and get the dynamite sticks on the left."sv, "Va chercher de la dynamite à gauche."sv, "Hol dir das Dynamit links."sv, "Dojdi si pro dynamit vlevo."sv, "Choď si po dynamit vľavo."sv, "Ve a buscar la dinamita a la izquierda."sv, "Vai a prendere la dinamite a sinistra."sv, "Idź po dynamit po lewej."sv, "Vá buscar a dinamite à esquerda."sv}},
            {20, 22, 0, 100, kGateHasDynamite,
             {"Put down the dynamite with \011, then clear off!"sv, "Pose la dynamite avec \011, puis barre-toi !"sv, "Leg das Dynamit mit \011 ab, dann nichts wie weg!"sv, "Polož dynamit pomocí \011 a pak zmiz!"sv, "Polož dynamit pomocou \011 a potom zmizni!"sv, "Deja la dinamita con \011 y ¡lárgate!"sv, "Metti la dinamite con \011, poi scappa!"sv, "Połóż dynamit za pomocą \011, a potem uciekaj!"sv, "Largue a dinamite com \011 e dê o fora!"sv},
             {"Put down the dynamite with \011, then clear off!"sv, "Pose la dynamite avec \011, puis barre-toi !"sv, "Leg das Dynamit mit \011 ab, dann nichts wie weg!"sv, "Polož dynamit pomocí \011 a pak zmiz!"sv, "Polož dynamit pomocou \011 a potom zmizni!"sv, "Deja la dinamita con \011 y ¡lárgate!"sv, "Metti la dinamite con \011, poi scappa!"sv, "Połóż dynamit za pomocą \011, a potem uciekaj!"sv, "Largue a dinamite com \011 e dê o fora!"sv}},
            {27, 28, 42, 100, kGateAlways,
             {"Put down another stick of dynamite here to move on."sv, "Pose encore de la dynamite pour passer."sv, "Leg hier noch ein Dynamit ab, um weiterzukommen."sv, "Polož sem další dynamit, abys prošel."sv, "Polož sem ďalší dynamit, aby si prešiel."sv, "Deja otra dinamita aquí para pasar."sv, "Metti un'altra dinamite qui per passare."sv, "Połóż tu jeszcze jeden dynamit, aby przejść."sv, "Largue outra dinamite aqui para passar."sv},
             {"Put down another stick of dynamite here to move on."sv, "Pose encore de la dynamite pour passer."sv, "Leg hier noch ein Dynamit ab, um weiterzukommen."sv, "Polož sem další dynamit, abys prošel."sv, "Polož sem ďalší dynamit, aby si prešiel."sv, "Deja otra dinamita aquí para pasar."sv, "Metti un'altra dinamite qui per passare."sv, "Połóż tu jeszcze jeden dynamit, aby przejść."sv, "Largue outra dinamite aqui para passar."sv}},
        };

        // The record's text and accelerometer variant in @p language.
        std::pair<std::string_view, std::string_view> Variant(const TrainingHintRecord& record, Language language) noexcept
        {
            return {record.text[LanguageIndex(language)], record.accelText[LanguageIndex(language)]};
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
