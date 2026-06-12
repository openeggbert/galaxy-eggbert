#pragma once
#include <cstdint>
#include <string>

// Persistent save data in the original Speedy Blupi wire format (640 bytes).
// Binary layout is identical to mobile-eggbert's GameData save — files are
// cross-compatible between the two builds.
//
// Layout (canonical reference — never change field offsets):
//   [0]      version tag                  default 1
//   [1]      reserved                     default 1
//   [2]      selectedGamer  0..2          default 0
//   [3]      sounds         1=on 0=off    default 1
//   [4]      jumpRight      1=right       default 1
//   [5]      autoZoom       1=on          default 1
//   [6]      accelActive    1=on          default 0
//   [7]      accelSensitivity 0..100      default 50
//   [8-9]    reserved
//   [10 + g*210 .. +9]   per-gamer header (nbVies, lastWorld, 8 reserved)
//   [10 + g*210 + 10 .. +209]  door states [0..199]  (0=locked 1=opened)
class GameData {
public:
    static constexpr int kTotalLength       = 640;
    static constexpr int kSaveHeaderLength  = 10;
    static constexpr int kGamerHeaderLength = 10;
    static constexpr int kDoorsLength       = 200;
    static constexpr int kGamerLength       = 210;
    static constexpr int kMaxGamer          = 3;

    GameData();

    // Returns false if the file doesn't exist or is the wrong size (keeps defaults).
    bool Read(const std::string& path);
    void Write(const std::string& path) const;

    // Reset current gamer slot to defaults (lives=3, lastWorld=1, all doors locked).
    void Reset();

    int  GetSelectedGamer() const { return data_[2]; }
    void SetSelectedGamer(int v)  { data_[2] = static_cast<uint8_t>(v); }

    bool GetSounds() const        { return data_[3] == 1; }
    void SetSounds(bool v)        { data_[3] = v ? 1 : 0; }

    int  GetNbVies()    const     { return data_[GamerOffset()]; }
    void SetNbVies(int v)         { data_[GamerOffset()] = static_cast<uint8_t>(v); }

    int  GetLastWorld() const     { return data_[GamerOffset() + 1]; }
    void SetLastWorld(int v)      { data_[GamerOffset() + 1] = static_cast<uint8_t>(v); }

    void GetDoors(int doors[]) const;
    void SetDoors(const int doors[]);
    void GetGamerInfo(int gamer, int& nbVies, int& mainDoors, int& secondaryDoors) const;

private:
    int GamerOffset()          const { return kSaveHeaderLength + kGamerLength * data_[2]; }
    static int GamerOffset(int g)    { return kSaveHeaderLength + kGamerLength * g; }
    void Initialize();
    void Initialize(int gamer);

    uint8_t data_[kTotalLength]{};
};
