#pragma once

#include <string>
#include <sstream>
#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    using SharpRuntime::intcs;

    /**
     * @brief Represents a tiny 2D point with integer coordinates.
     *
     * This structure is a C++ port of the original C# TinyPoint struct
     * from GalaxyEggbert. It stores the horizontal and vertical
     * position in the fields @c X and @c Y.
     *
     * @note Status: Ported
     */
    struct TinyPoint
    {
        /**
         * @brief Horizontal coordinate.
         *
         * @note Status: Ported
         */
        intcs X;

        /**
         * @brief Vertical coordinate.
         *
         * @note Status: Ported
         */
        intcs Y;

        /**
         * @brief Creates a TinyPoint at coordinates (0, 0).
         *
         * This constructor does not exist explicitly in the original C# code,
         * but is a practical addition for C++ usage.
         *
         * @note Status: Ported
         */
        TinyPoint()
            : X(0), Y(0)
        {
        }

        /**
         * @brief Creates a TinyPoint with the specified coordinates.
         *
         * @param x Horizontal coordinate.
         * @param y Vertical coordinate.
         *
         * @note Status: Ported
         */
        TinyPoint(intcs x, intcs y)
            : X(x), Y(y)
        {
        }

        /**
         * @brief Returns the point as a string in the format "X;Y".
         *
         * Example: if X is 10 and Y is 20, the returned string is "10;20".
         *
         * @return String representation of the point.
         *
         * @note Status: Ported
         */
        [[nodiscard]] std::string ToString() const
        {
            std::ostringstream oss;
            oss << X << ";" << Y;
            return oss.str();
        }
    };
}
