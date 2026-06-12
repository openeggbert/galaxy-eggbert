#include "GameData.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>

GameData::GameData() { Initialize(); }

bool GameData::Read(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    f.read(reinterpret_cast<char*>(data_), kTotalLength);
    return static_cast<int>(f.gcount()) == kTotalLength;
}

void GameData::Write(const std::string& path) const {
    std::filesystem::create_directories(
        std::filesystem::path(path).parent_path());
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (f.is_open())
        f.write(reinterpret_cast<const char*>(data_), kTotalLength);
}

void GameData::Reset() { Initialize(data_[2]); }

void GameData::GetDoors(int doors[]) const {
    int off = GamerOffset() + kGamerHeaderLength;
    for (int i = 0; i < kDoorsLength; ++i) doors[i] = data_[off + i];
}

void GameData::SetDoors(const int doors[]) {
    int off = GamerOffset() + kGamerHeaderLength;
    for (int i = 0; i < kDoorsLength; ++i)
        data_[off + i] = static_cast<uint8_t>(doors[i]);
}

void GameData::GetGamerInfo(int gamer, int& nbVies, int& mainDoors, int& secondaryDoors) const {
    int off = GamerOffset(gamer);
    nbVies = data_[off];
    secondaryDoors = 0;
    for (int i = 0;   i < 180; ++i)
        if (data_[off + kGamerHeaderLength + i] == 1) ++secondaryDoors;
    mainDoors = 0;
    for (int j = 180; j < 200; ++j)
        if (data_[off + kGamerHeaderLength + j] == 1) ++mainDoors;
}

void GameData::Initialize() {
    std::memset(data_, 0, kTotalLength);
    data_[0] = 1;   // version
    data_[1] = 1;   // reserved
    data_[2] = 0;   // selectedGamer
    data_[3] = 1;   // sounds on
    data_[4] = 1;   // jumpRight
    data_[5] = 1;   // autoZoom
    data_[6] = 0;   // accelActive off
    data_[7] = 50;  // accelSensitivity 50%
    for (int i = 0; i < kMaxGamer; ++i) Initialize(i);
}

void GameData::Initialize(int gamer) {
    int off = GamerOffset(gamer);
    data_[off]     = 3;  // nbVies
    data_[off + 1] = 1;  // lastWorld
    std::memset(data_ + off + kGamerHeaderLength, 0, kDoorsLength);
}
