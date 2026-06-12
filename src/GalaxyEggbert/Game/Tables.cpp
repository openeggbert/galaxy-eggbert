#include "Tables.hpp"

using namespace GalaxyEggbert;

// Data derived from mobile-eggbert Tables.cpp table_blupi[2911].
// Format per entry: { action, frameCount, maxPhase, icons[] }
// maxPhase==0: loop forever.  maxPhase>0: freeze at that frame once reached.

int Tables::GetBlupiIcon(BlupiAction action, int scaledPhase) {
    // Stop(1): 330-frame idle cycle (standing still with occasional blink).
    static const int kStop[] = {
        0,0,0,0,0,23,23,23,0,0,0,0,0,0,0,
        0,0,23,23,23,23,23,23,0,0,0,0,0,0,0,
        0,23,23,23,23,0,0,0,0,0,0,0,0,0,0,
        23,23,23,23,23,0,0,0,23,23,23,0,0,0,0,
        0,0,133,133,0,0,0,133,133,0,0,0,0,0,0,
        0,133,133,0,0,0,0,23,23,23,23,0,0,0,0,
        0,0,0,0,0,0,0,23,23,0,0,0,0,0,23,
        23,23,0,0,0,135,135,136,136,137,137,137,137,137,137,
        137,137,138,138,137,137,137,138,138,137,137,137,138,138,137,
        137,137,138,138,137,137,137,137,137,137,136,136,135,135,135,
        0,0,0,0,0,23,23,23,0,0,133,133,0,0,0,
        23,23,23,23,0,0,0,0,0,0,0,23,23,0,0,
        0,0,0,0,0,0,133,133,0,0,0,0,0,23,23,
        23,23,0,0,0,135,135,136,136,137,137,137,137,137,137,
        137,137,138,138,137,137,137,138,138,137,137,137,138,138,137,
        137,137,138,138,137,137,137,138,138,137,137,137,138,138,137,
        137,137,138,138,137,137,137,138,138,137,137,137,138,138,137,
        137,137,138,138,137,137,137,138,138,137,137,137,138,138,137,
        137,137,137,137,137,136,136,135,135,135,0,0,0,0,0,
        23,23,23,0,0,133,133,0,0,0,23,23,23,23,0,
        0,0,0,0,0,0,23,23,0,0,0,0,0,0,0,
        0,133,133,0,0,0,0,0,23,23,23,23,0,0,0
    };
    static const int kMarch[] = {5,6,7,8,9,10};   // March(2): 6-frame walk cycle
    static const int kTurn[]  = {1,1,2,2,3,3};    // Turn(3):  6-frame turn
    static const int kJump[]  = {17,18,19};        // Jump(4):  3-frame jump start
    static const int kAir[]   = {169,26,170,170,27}; // Air(5): 5 frames, freezes at 4

    struct Entry {
        BlupiAction action;
        int         frameCount;
        int         maxPhase;
        const int*  icons;
    };
    static const Entry kEntries[] = {
        {BlupiAction::Stop,  330, 0, kStop },
        {BlupiAction::March,   6, 0, kMarch},
        {BlupiAction::Turn,    6, 0, kTurn },
        {BlupiAction::Jump,    3, 0, kJump },
        {BlupiAction::Air,     5, 4, kAir  },
    };

    for (const auto& e : kEntries) {
        if (e.action != action) continue;
        int frame = (e.maxPhase == 0 || scaledPhase <= e.maxPhase)
                        ? (scaledPhase % e.frameCount)
                        : e.maxPhase;
        return e.icons[frame];
    }
    return 0;
}
