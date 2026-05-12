#pragma once

#include <algorithm>
#include <ranges>

#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    using SharpRuntime::intcs;

    /**
     * @brief Provides global game constants, enums, and static helper members.
     *
     * This class is a C++ port of the original C# static class Def from
     * GalaxyEggbert.
     *
     * @note Status: Ported
     */
    class Def
    {
    public:
        Def() = delete;
        ~Def() = delete;

        /**
         * @brief Represents the current game phase.
         *
         * @note Status: Ported
         */
        enum class Phase
        {
            None,
            First,
            Wait,
            Init,
            Play,
            Pause,
            Lost,
            Win,
            Trial,
            MainSetup,
            PlaySetup,
            Resume,
            Ranking
        };

        /**
         * @brief Identifies a button glyph.
         *
         * @note Status: Ported
         */
        enum class ButtonGlyph
        {
            None,
            InitGamerA,
            InitGamerB,
            InitGamerC,
            InitSetup,
            InitPlay,
            InitBuy,
            InitRanking,
            WinLostReturn,
            TrialBuy,
            TrialCancel,
            SetupSounds,
            SetupJump,
            SetupZoom,
            SetupAccel,
            SetupReset,
            SetupReturn,
            PauseMenu,
            PauseBack,
            PauseSetup,
            PauseRestart,
            PauseContinue,
            PlayPause,
            PlayJump,
            PlayAction,
            PlayDown,
            ResumeMenu,
            ResumeContinue,
            RankingContinue,
            Cheat11,
            Cheat12,
            Cheat21,
            Cheat22,
            Cheat31,
            Cheat32,
            Cheat1,
            Cheat2,
            Cheat3,
            Cheat4,
            Cheat5,
            Cheat6,
            Cheat7,
            Cheat8,
            Cheat9
        };

        /**
         * @brief Returns true if the specified glyph is not present in the given list.
         *
         * This is an additional helper not present in the original C# code.
         *
         * @param buttonGlyphToBeChecked Glyph to search for.
         * @param buttonGlyphs Collection of glyphs.
         * @return True if the glyph is not present; otherwise false.
         *
         * @note Additional
         */
        static bool isNotOneOf(const ButtonGlyph& buttonGlyphToBeChecked,
                               std::initializer_list<ButtonGlyph> buttonGlyphs)
        {
            return std::ranges::none_of(buttonGlyphs,
                                        [&buttonGlyphToBeChecked](const ButtonGlyph candidate)
                                        {
                                            return candidate == buttonGlyphToBeChecked;
                                        });
        }

        static constexpr intcs LXIMAGE = 640;
        static constexpr intcs LYIMAGE = 480;
        static constexpr intcs MAXCELX = 100;
        static constexpr intcs MAXCELY = 100;
        static constexpr intcs DIMOBJX = 64;
        static constexpr intcs DIMOBJY = 64;
        static constexpr intcs DIMBLUPIX = 60;
        static constexpr intcs DIMBLUPIY = 60;
        static constexpr intcs DIMEXPLOX = 128;
        static constexpr intcs DIMEXPLOY = 128;
        static constexpr intcs DIMBUTTONX = 40;
        static constexpr intcs DIMBUTTONY = 40;
        static constexpr intcs DIMJAUGEX = 124;
        static constexpr intcs DIMJAUGEY = 22;
        static constexpr intcs POSSTATX = 12;
        static constexpr intcs POSSTATY = 220;
        static constexpr intcs DIMSTATX = 60;
        static constexpr intcs DIMSTATY = 30;
        static constexpr intcs DIMTEXTX = 32;
        static constexpr intcs DIMTEXTY = 32;

        static constexpr intcs CHOBJECT = 1;
        static constexpr intcs CHBLUPI = 2;
        static constexpr intcs CHDECOR = 3;
        static constexpr intcs CHBUTTON = 4;
        static constexpr intcs CHJAUGE = 5;
        static constexpr intcs CHTEXT = 6;
        static constexpr intcs CHEXPLO = 9;
        static constexpr intcs CHELEMENT = 10;
        static constexpr intcs CHBLUPI1 = 11;
        static constexpr intcs CHBLUPI2 = 12;
        static constexpr intcs CHBLUPI3 = 13;
        static constexpr intcs CHPAD = 14;
        static constexpr intcs CHSPEEDYBLUPI = 15;
        static constexpr intcs CHBLUPIYOUPIE = 16;
        static constexpr intcs CHGEAR = 17;

        static constexpr intcs ACTION_STOP = 1;
        static constexpr intcs ACTION_MARCH = 2;
        static constexpr intcs ACTION_TURN = 3;
        static constexpr intcs ACTION_JUMP = 4;
        static constexpr intcs ACTION_AIR = 5;
        static constexpr intcs ACTION_DOWN = 6;
        static constexpr intcs ACTION_UP = 7;
        static constexpr intcs ACTION_VERTIGO = 8;
        static constexpr intcs ACTION_RECEDE = 9;
        static constexpr intcs ACTION_ADVANCE = 10;
        static constexpr intcs ACTION_CLEAR1 = 11;
        static constexpr intcs ACTION_SET = 12;
        static constexpr intcs ACTION_WIN = 13;
        static constexpr intcs ACTION_PUSH = 14;
        static constexpr intcs ACTION_STOPHELICO = 15;
        static constexpr intcs ACTION_MARCHHELICO = 16;
        static constexpr intcs ACTION_TURNHELICO = 17;
        static constexpr intcs ACTION_STOPNAGE = 18;
        static constexpr intcs ACTION_MARCHNAGE = 19;
        static constexpr intcs ACTION_TURNNAGE = 20;
        static constexpr intcs ACTION_STOPSURF = 21;
        static constexpr intcs ACTION_MARCHSURF = 22;
        static constexpr intcs ACTION_TURNSURF = 23;
        static constexpr intcs ACTION_DROWN = 24;
        static constexpr intcs ACTION_STOPJEEP = 25;
        static constexpr intcs ACTION_MARCHJEEP = 26;
        static constexpr intcs ACTION_TURNJEEP = 27;
        static constexpr intcs ACTION_STOPPOP = 28;
        static constexpr intcs ACTION_POP = 29;
        static constexpr intcs ACTION_BYE = 30;
        static constexpr intcs ACTION_STOPSUSPEND = 31;
        static constexpr intcs ACTION_MARCHSUSPEND = 32;
        static constexpr intcs ACTION_TURNSUSPEND = 33;
        static constexpr intcs ACTION_JUMPSUSPEND = 34;
        static constexpr intcs ACTION_HIDE = 35;
        static constexpr intcs ACTION_JUMPAIE = 36;
        static constexpr intcs ACTION_STOPSKATE = 37;
        static constexpr intcs ACTION_MARCHSKATE = 38;
        static constexpr intcs ACTION_TURNSKATE = 39;
        static constexpr intcs ACTION_JUMPSKATE = 40;
        static constexpr intcs ACTION_AIRSKATE = 41;
        static constexpr intcs ACTION_TAKESKATE = 42;
        static constexpr intcs ACTION_DEPOSESKATE = 43;
        static constexpr intcs ACTION_OUF1a = 44;
        static constexpr intcs ACTION_OUF1b = 45;
        static constexpr intcs ACTION_OUF2 = 46;
        static constexpr intcs ACTION_OUF3 = 47;
        static constexpr intcs ACTION_OUF4 = 48;
        static constexpr intcs ACTION_SUCETTE = 49;
        static constexpr intcs ACTION_STOPTANK = 50;
        static constexpr intcs ACTION_MARCHTANK = 51;
        static constexpr intcs ACTION_TURNTANK = 52;
        static constexpr intcs ACTION_FIRETANK = 53;
        static constexpr intcs ACTION_GLU = 54;
        static constexpr intcs ACTION_DRINK = 55;
        static constexpr intcs ACTION_CHARGE = 56;
        static constexpr intcs ACTION_ELECTRO = 57;
        static constexpr intcs ACTION_HELICOGLU = 58;
        static constexpr intcs ACTION_TURNAIR = 59;
        static constexpr intcs ACTION_STOPMARCH = 60;
        static constexpr intcs ACTION_STOPJUMP = 61;
        static constexpr intcs ACTION_STOPJUMPh = 62;
        static constexpr intcs ACTION_MOCKERY = 63;
        static constexpr intcs ACTION_MOCKERYi = 64;
        static constexpr intcs ACTION_OUF5 = 65;
        static constexpr intcs ACTION_BALLOON = 66;
        static constexpr intcs ACTION_STOPOVER = 67;
        static constexpr intcs ACTION_MARCHOVER = 68;
        static constexpr intcs ACTION_TURNOVER = 69;
        static constexpr intcs ACTION_RECEDEq = 70;
        static constexpr intcs ACTION_ADVANCEq = 71;
        static constexpr intcs ACTION_STOPECRASE = 72;
        static constexpr intcs ACTION_MARCHECRASE = 73;
        static constexpr intcs ACTION_TELEPORTE = 74;
        static constexpr intcs ACTION_CLEAR2 = 75;
        static constexpr intcs ACTION_CLEAR3 = 76;
        static constexpr intcs ACTION_CLEAR4 = 77;
        static constexpr intcs ACTION_CLEAR5 = 78;
        static constexpr intcs ACTION_CLEAR6 = 79;
        static constexpr intcs ACTION_CLEAR7 = 80;
        static constexpr intcs ACTION_CLEAR8 = 81;
        static constexpr intcs ACTION_SWITCH = 82;
        static constexpr intcs ACTION_MOCKERYp = 83;
        static constexpr intcs ACTION_NON = 84;
        static constexpr intcs ACTION_SLOWDOWNSKATE = 85;
        static constexpr intcs ACTION_TAKEDYNAMITE = 86;
        static constexpr intcs ACTION_PUTDYNAMITE = 87;

        static constexpr intcs DIR_LEFT = 1;
        static constexpr intcs DIR_RIGHT = 2;

        static constexpr intcs SEC_SHIELD = 1;
        static constexpr intcs SEC_POWER = 2;
        static constexpr intcs SEC_CLOUD = 3;
        static constexpr intcs SEC_HIDE = 4;

        static constexpr intcs KEY_JUMP = 1;
        static constexpr intcs KEY_FIRE = 2;
        static constexpr intcs KEY_DOWN = 4;

        /**
         * @brief Gets the HasSound property value.
         *
         * Corresponds to the original C# static read-only property HasSound.
         *
         * @return Always true.
         *
         * @note Status: Ported
         */
        static constexpr bool getHasSoundProperty()
        {
            return true;
        }

        /**
         * @brief Gets the EasyMove property value.
         *
         * Corresponds to the original C# static read-only property EasyMove.
         *
         * @return Always true.
         *
         * @note Status: Ported
         */
        static constexpr bool getEasyMoveProperty()
        {
            return true;
        }
    };
}
