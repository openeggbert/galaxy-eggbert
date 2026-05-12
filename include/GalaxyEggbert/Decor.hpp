#pragma once

#include "GameData.hpp"
#include "IPixmap.hpp"
#include "ISound.hpp"
#include "Tables.hpp"
#include "System/Random.hpp"
#include "Jauge.hpp"
#include "SharpRuntime/Prop.hpp"
#include "GalaxyEggbert/Helper.hpp"
#include "decor/ObjectType.hpp"
#include "enums/PixmapChannel.hpp"

namespace GalaxyEggbert
{
    class Decor
    {
        enum class IconType
        {

        };

        struct Cellule
        {
            intcs icon;
        };

        struct MoveObject
        {
            ObjectType type;
            intcs stepAdvance;
            intcs stepRecede;
            intcs timeStopStart;
            intcs timeStopEnd;
            TinyPoint posStart;
            TinyPoint posEnd;
            TinyPoint posCurrent;
            intcs step;
            intcs time;
            intcs phase;
            PixmapChannel channel;
            intcs icon;
        };

        class ByeByeObject
        {
        public:
            PixmapChannel channel;
            intcs icon;
            double posX;
            double posY;
            double rotation;
            double phase;
            double animationSpeed;
            double rotationSpeed;
            double speedX;
        };

        static constexpr intcs MAXMOVEOBJECT = 200;
        static constexpr intcs MAXQUART = 441;
        static constexpr intcs SCROLL_SPEED = 8;
        static constexpr intcs SCROLL_MARGX = 80;
        static constexpr intcs SCROLL_MARGY = 40;
        static constexpr intcs BLUPIFLOOR = 2;
        static constexpr intcs BLUPIOFFY = 4 + BLUPIFLOOR;
        static constexpr intcs BLUPISURF = 12;
        static constexpr intcs BLUPISUSPEND = 12;
        static constexpr intcs OVERHEIGHT = 80;
        ISound* m_sound;
        IPixmap* m_pixmap;
        GameData* m_gameData;
        Cellule m_decor[100][100]{};
        Cellule m_bigDecor[100][100]{};
        static constexpr int m_balleTrajLength = 1300;
        intcs m_balleTraj[m_balleTrajLength]{};
        static constexpr int m_moveTrajLength = 1300;
        intcs m_moveTraj[m_moveTrajLength]{};
        MoveObject m_moveObject[MAXMOVEOBJECT];
        intcs m_keyPress;
        intcs m_lastKeyPress;
        TinyPoint m_posDecor;
        TinyPoint m_dimDecor;

        intcs m_term;

        intcs m_music;

        intcs m_region;

        intcs m_time;

        intcs m_bPause;

        TinyRect m_drawBounds;

        intcs m_nbRankCaisse;

        static constexpr intcs m_rankCaisseLength = MAXMOVEOBJECT;

        intcs m_rankCaisse[m_rankCaisseLength]{};

        intcs m_nbLinkCaisse;

        static constexpr intcs m_linkCaisseLength = MAXMOVEOBJECT;

        intcs m_linkCaisse[m_linkCaisseLength];

        TinyPoint m_blupiPos;

        TinyPoint m_blupiLastPos;

        TinyPoint m_blupiValidPos;

        intcs m_blupiAction;

        intcs m_blupiDir;

        intcs m_blupiPhase;

        double m_blupiVitesseX;

        double m_blupiVitesseY;

        intcs m_blupiIcon;

        intcs m_blupiSec;

        PixmapChannel m_blupiChannel;

        TinyPoint m_blupiVector;

        intcs m_blupiTransport;

        bool m_blupiFocus;

        bool m_blupiAir;

        bool m_blupiHelico;

        bool m_blupiOver;

        bool m_blupiJeep;

        bool m_blupiTank;

        bool m_blupiSkate;

        bool m_blupiNage;

        bool m_blupiSurf;

        bool m_blupiVent;

        bool m_blupiSuspend;

        bool m_blupiJumpAie;

        bool m_blupiShield;

        bool m_blupiPower;

        bool m_blupiCloud;

        bool m_blupiHide;

        bool m_blupiInvert;

        bool m_blupiBalloon;

        bool m_blupiEcrase;

        bool m_blupiMotorHigh;

        SoundChannel m_blupiMotorSound;

        TinyPoint m_blupiPosHelico;

        TinyPoint m_blupiPosMagic;

        bool m_blupiRestart;

        bool m_blupiFront;

        intcs m_blupiBullet;

        intcs m_blupiCle;

        intcs m_blupiPerso;

        intcs m_blupiDynamite;

        intcs m_blupiNoBarre;

        intcs m_blupiTimeShield;

        intcs m_blupiTimeFire;

        intcs m_blupiTimeNoAsc;

        intcs m_blupiTimeMockery;

        intcs m_blupiTimeOuf;

        intcs m_blupiActionOuf;

        intcs m_blupiFifoNb;

        TinyPoint m_blupiFifoPos[10];

        TinyPoint m_blupiStartPos;

        intcs m_blupiStartDir;

        double m_blupiSpeedX;

        double m_blupiSpeedY;

        double m_blupiLastSpeedX;

        double m_blupiLastSpeedY;

        Jauge m_jauges[2];

        intcs m_blupiLevel;

        bool m_bFoundCle;

        bool m_bPrivate;

        bool m_bCheatDoors;

        bool m_bSuperBlupi;

        bool m_bDrawSecret;

        bool m_buildOfficialMissions;

        intcs m_mission;

        static constexpr intcs m_doorsLength = 200;

        intcs m_doors[m_doorsLength]{};

        intcs m_nbVies;

        intcs m_nbTresor;

        intcs m_totalTresor;

        intcs m_goalPhase;

        intcs m_detectIcon;

        TinyPoint m_sucettePos;

        ObjectType m_sucetteType;

        intcs m_blupiLogicRotation;

        intcs m_blupiRealRotation;

        intcs m_blupiOffsetY;

        TinyPoint m_scrollPoint;

        TinyPoint m_scrollAdd;

        intcs m_voyageIcon;

        PixmapChannel m_voyageChannel;

        intcs m_voyagePhase;

        intcs m_voyageTotal;

        TinyPoint m_voyageStart;

        TinyPoint m_voyageEnd;

        intcs m_decorAction;

        intcs m_decorPhase;

        intcs m_lastDecorIcon[200]{};

        double m_hotSpotFinalZoom;

        double m_hotSpotFinalX;

        double m_hotSpotFinalY;

        double m_hotSpotCurrentZoom;

        double m_hotSpotCurrentX;

        double m_hotSpotCurrentY;

        double m_hotSpotStepZoom;

        double m_hotSpotStepX;

        double m_hotSpotStepY;

        double m_hotSpotOutLag;

        std::unique_ptr<System::Random> m_random;

        std::vector<ByeByeObject> byeByeObjects;

    public:
        [[nodiscard]] TinyRect getDrawBoundsProperty() const;
        void setDrawBoundsProperty(const TinyRect v);
        DDATA(Def::ButtonGlyph, ButtonPressed)

    private:
        static void MoveObjectCopy(MoveObject& dst, const MoveObject& src);

    public:
        Decor();

    public:
        void Create(ISound* sound, IPixmap* pixmap, GameData* gameData);

    public:
        bool LoadImages();

    private:
        void InitDecor();

    public:
        void PlayPrepare(bool bTest);

    private:
        void BuildPrepare();

    public:
        int IsTerminated();

    public:
        void MoveStep();

    private:
        void ResetHotSpot();

    private:
        void MoveHotSpot();

    private:
        bool BlitzActif(intcs celx, intcs cely);

    public:
        void Build();

    private:
        void DrawInfo();

    private:
        bool IsDisplayInfo(int tableTresor);

    private:
        TinyPoint DecorNextAction();

    public:
        void SetSpeedX(double speed);

    public:
        void SetSpeedY(double speed);

    public:
        void KeyChange(int keyPress);

    private:
        void GetBlupiInfo(bool& bHelico, bool& bJeep, bool& bSkate, bool& bNage);

    private:
        SoundChannel SoundEnviron(SoundChannel sound, int obstacle);

    private:
        void PlaySound(SoundChannel sound, TinyPoint pos);

    public:
        void StopSound();

    public:
        void StartSound();

    private:
        void StopSound(SoundChannel sound);

    private:
        void AdaptMotorVehicleSound();

    private:
        void PosSound(TinyPoint pos);

    private:
        int GetRegion();

    private:
        void SetRegion(int region);

    private:
        int GetMusic();

    private:
        void SetMusic(int music);

    public:
        TinyPoint GetDim();

    public:
        void SetDim(TinyPoint dim);

    public:
        int GetMission();

    public:
        void SetMission(int mission);

    public:
        int GetNbVies();

    public:
        void SetNbVies(int nbVies);

    public:
        void InitializeDoors(GameData& gameData);

    public:
        void MemorizeDoors(GameData& gameData);

    public:
        static std::string GetCheatTinyText(Def::ButtonGlyph glyph);

    public:
        void CheatAction(Tables::CheatCodes cheat);

    private:
        void SetBuildOfficialMissions(bool bMode);

    private:
        void BlupiSearchIcon();

    private:
        bool BlupiIsGround();

    private:
        TinyRect BlupiRect(TinyPoint pos);

    private:
        void BlupiAdjust();

    private:
        bool BlupiBloque(TinyPoint pos, int dir);

    private:
        void BlupiStep();

    private:
        void BlupiDead(int action1, int action2);

    private:
        TinyPoint GetPosDecor(TinyPoint pos);

    private:
        void BlupiAddFifo(TinyPoint pos);

    private:
        bool DecorDetect(TinyRect rect);

    private:
        bool DecorDetect(TinyRect rect, bool bCaisse);

    private:
        bool TestPath(const TinyRect& rect, const TinyPoint& start, TinyPoint& end);

    private:
        void MoveObjectPollution();

    private:
        void MoveObjectPlouf(TinyPoint pos);

    private:
        void MoveObjectTiplouf(TinyPoint pos);

    private:
        void MoveObjectBlup(TinyPoint pos);

    private:
        int IsWorld(TinyPoint pos);

    private:
        void ActiveSwitch(bool bState, TinyPoint cel);

    private:
        int GetTypeBarre(TinyPoint pos);

    private:
        bool IsLave(TinyPoint pos);

    private:
        bool IsPiege(TinyPoint pos);

    private:
        bool IsGoutte(TinyPoint pos, bool bAlways);

    private:
        bool IsScie(TinyPoint pos);

    private:
        bool IsSwitch(TinyPoint pos, TinyPoint& celSwitch);

    private:
        bool IsEcraseur(TinyPoint pos);

    private:
        bool IsBlitz(TinyPoint pos, bool bAlways);

    private:
        bool IsRessort(TinyPoint pos);

    private:
        bool IsTemp(TinyPoint pos);

    private:
        bool IsBridge(TinyPoint pos, TinyPoint& celBridge);

    private:
        int IsDoor(TinyPoint pos, TinyPoint& celPorte);

    private:
        int IsTeleporte(TinyPoint pos);

    private:
        bool SearchTeleporte(TinyPoint pos, TinyPoint& newpos);

    private:
        bool IsNormalJump(TinyPoint pos);

    private:
        bool IsSurfWater(TinyPoint pos);

    private:
        bool IsDeepWater(TinyPoint pos);

    private:
        bool IsOutWater(TinyPoint pos);

    private:
        bool IsPassIcon(int icon);

    private:
        bool IsBlocIcon(int icon);

    private:
        void FlushBalleTraj();

    private:
        void SetBalleTraj(TinyPoint pos);

    private:
        bool IsBalleTraj(TinyPoint pos);

    private:
        void FlushMoveTraj();

    private:
        void SetMoveTraj(TinyPoint pos);

    private:
        bool IsMoveTraj(TinyPoint pos);

    private:
        int SearchDistRight(TinyPoint pos, TinyPoint dir, ObjectType type);

    private:
        bool IsVentillo(TinyPoint pos);

    private:
        void NetStopCloud(int rank);

    private:
        void StartSploutchGlu(TinyPoint pos);

    private:
        int ObjectStart(TinyPoint pos, ObjectType type, int speed);

    private:
        bool ObjectDelete(TinyPoint pos, ObjectType type);

    private:
        void ModifDecor(TinyPoint pos, int icon);

    private:
        void MoveObjectStep();

    private:
        void MoveObjectStepLine(int i);

    private:
        void MoveObjectStepIcon(int i);

    private:
        void DynamiteStart(int i, int dx, int dy);

    private:
        int AscenseurDetect(TinyRect rect, TinyPoint oldpos, TinyPoint newpos);

    private:
        void AscenseurVertigo(int i, bool& bVertigoLeft, bool& bVertigoRight);

    private:
        bool AscenseurShift(int i);

    private:
        void AscenseurSynchro(int i);

    private:
        void UpdateCaisse();

    private:
        bool TestPushCaisse(int i, TinyPoint pos, bool bPop);

    private:
        bool TestPushOneCaisse(int i, TinyPoint move, int b);

    private:
        void SearchLinkCaisse(int rank, bool bPop);

    private:
        bool AddLinkCaisse(int rank);

    private:
        int CaisseInFront();

    private:
        int CaisseGetMove(int max);

    private:
        int MockeryDetect(TinyPoint pos);

    private:
        bool BlupiElectro(TinyPoint pos);

    private:
        void MoveObjectFollow(TinyPoint pos);

    private:
        int MoveObjectDetect(TinyPoint pos, bool& bNear);

    private:
        int MoveAscenseurDetect(TinyPoint pos, int height);

    private:
        int MoveChargeDetect(TinyPoint pos);

    private:
        int MovePersoDetect(TinyPoint pos);

    private:
        int MoveObjectDelete(TinyPoint cel);

    private:
        int MoveObjectFree();

    private:
        int SortGetType(ObjectType type);

    private:
        void MoveObjectSort();

    private:
        void MoveObjectPriority(int i);

    private:
        int MoveObjectSearch(TinyPoint pos);

    private:
        int MoveObjectSearch(TinyPoint pos, std::optional<ObjectType> type);

    private:
        void ByeByeHelico();

    private:
        void ByeByeAdd(PixmapChannel channel, int icon, TinyPoint pos, double rotationSpeed, double animationSpeed);

    private:
        void ByeByeStep();

    private:
        void ByeByeDraw(TinyPoint posDecor);

    private:
        TinyPoint VoyageGetPosVie(int nbVies);

    private:
        void VoyageInit(TinyPoint start, TinyPoint end, int icon, PixmapChannel channel);

    private:
        void VoyageStep();

    private:
        void VoyageDraw();

    private:
        bool IsFloatingObject(int i);

    private:
        bool IsRightBorder(int x, int y, int dx, int dy);

    private:
        bool IsFromage(int x, int y);

    private:
        bool IsGrotte(int x, int y);

    private:
        void AdaptMidBorder(int x, int y);

    private:
        void AdaptBorder(TinyPoint cel);

    public:
        void CurrentDelete();

    public:
        bool CurrentWrite();

    public:
        bool CurrentRead();

    public:
        bool Read(int gamer, int rank, bool bUser);

    private:
        bool Delete(int gamer, int rank, bool bUser);

    private:
        bool FileExist(int gamer, int rank, bool bUser);

    private:
        bool SearchWorld(int world, TinyPoint& blupi, int& dir);

    private:
        bool SearchDoor(int n, TinyPoint& cel, TinyPoint& blupi);

    private:
        bool SearchGold(int n, TinyPoint& cel);

    public:
        void MainSwitchInitialize(int lastWorld);

    public:
        void AdaptDoors(bool bPrivate);

    private:
        void OpenDoorsTresor();

    private:
        void OpenDoor(TinyPoint cel);

    private:
        void OpenDoorsWin();

    private:
        void OpenGoldsWin();

    private:
        void DoorsLost();
    };
}
