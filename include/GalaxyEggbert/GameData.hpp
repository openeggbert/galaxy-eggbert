#pragma once

#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    using ushort = unsigned short;
    using SharpRuntime::bytecs;
    using SharpRuntime::intcs;

    class GameData
    {
        static constexpr intcs SaveHeaderLength = 10;

        static constexpr intcs GamerHeaderLength = 10;

        static constexpr intcs DoorsLength = 200;

        static constexpr intcs GamerLength = GamerHeaderLength + DoorsLength;

        static constexpr intcs MaxGamer = 3;

        static constexpr intcs TotalLength = SaveHeaderLength + GamerLength * MaxGamer;

        bytecs data[TotalLength]{};

    public:
        [[nodiscard]] intcs getSelectedGamerProperty() const;

        void setSelectedGamerProperty(const intcs v);

        [[nodiscard]] bool getSoundsProperty() const;

        void setSoundsProperty(const bool v);

        [[nodiscard]] bool getJumpRightProperty() const;

        void setJumpRightProperty(const bool v);
        [[nodiscard]] bool getAutoZoomProperty() const;


        void setAutoZoomProperty(const bool v);
        [[nodiscard]] bool getAccelActiveProperty() const;


        void setAccelActiveProperty(const bool v);
        [[nodiscard]] double getAccelSensitivityProperty() const;


        void setAccelSensitivityProperty(double v);

        [[nodiscard]] intcs getNbViesProperty() const;

        void setNbViesProperty(const intcs v);

        [[nodiscard]] intcs getLastWorldProperty() const;


        void setLastWorldProperty(const intcs v);
        [[nodiscard]] intcs getGamerOffsetProperty() const;

        GameData();

        void Read();

        void Write();

        void Reset();

        void GetDoors(intcs doors[]);

        void SetDoors(const intcs doors[]);

        void GetGamerInfo(intcs gamer, intcs& nbVies, intcs& mainDoors, intcs& secondaryDoors);

    private:
        void Initialize();

        void Initialize(intcs gamer);

        static intcs GetGamerOffset(intcs gamer);
    };
}
