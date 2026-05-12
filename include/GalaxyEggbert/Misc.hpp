#pragma once

#include "TinyPoint.hpp"
#include "TinyRect.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    using SharpRuntime::intcs;
    using GalaxyEggbert::TinyPoint;

    /**
     * @brief Provides miscellaneous helper methods used by the game.
     *
     * This class is a C++ port of the original C# static class Misc from
     * GalaxyEggbert.
     *
     * It contains utility methods for point rotation, angle conversion,
     * rectangle operations, and small numeric helpers.
     *
     * @note Status: Ported
     */
    class Misc
    {
    public:
        /**
         * @brief Deleted constructor because this is a static utility class.
         *
         * @note Status: Ported
         */
        Misc() = delete;

        /**
         * @brief Deleted destructor because this is a static utility class.
         *
         * @note Status: Ported
         */
        ~Misc() = delete;

        /**
         * @brief Adjusts a rectangle position after rotation around its center.
         *
         * The returned rectangle keeps the same width and height as the input
         * rectangle, but its top-left position is adjusted according to the
         * rotation of the rectangle center.
         *
         * @param rect Rectangle to adjust.
         * @param angle Rotation angle in radians.
         * @return Adjusted rectangle.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static Microsoft::Xna::Framework::Rectangle RotateAdjust(
            const Microsoft::Xna::Framework::Rectangle& rect,
            double angle);

        /**
         * @brief Rotates a point around the origin using an angle in radians.
         *
         * This overload rotates the point around (0, 0).
         *
         * @param angle Rotation angle in radians.
         * @param p Point to rotate.
         * @return Rotated point.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static TinyPoint RotatePointRad(double angle, const TinyPoint& p);

        /**
         * @brief Rotates a point around a specified center using an angle in radians.
         *
         * @param center Center of rotation.
         * @param angle Rotation angle in radians.
         * @param point Point to rotate.
         * @return Rotated point.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static TinyPoint RotatePointRad(
            const TinyPoint& center,
            double angle,
            const TinyPoint& point);

        /**
         * @brief Converts an angle from degrees to radians.
         *
         * @param angle Angle in degrees.
         * @return Angle in radians.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static double DegToRad(double angle);

        /**
         * @brief Moves an integer value toward a target value by a given step.
         *
         * If @p actual is smaller than @p final, it is increased by @p step
         * but not beyond @p final. If @p actual is greater than @p final,
         * it is decreased by @p step but not below @p final.
         *
         * @param actual Current value.
         * @param final Target value.
         * @param step Step size.
         * @return Adjusted value.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static intcs Approach(intcs actual, intcs final, intcs step);

        /**
         * @brief Converts a normalized speed factor into an integer speed.
         *
         * Positive speeds return at least 1, negative speeds return at most -1,
         * and zero returns 0.
         *
         * @param speed Speed factor.
         * @param max Maximum magnitude scaling factor.
         * @return Integer speed.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static intcs Speed(double speed, intcs max);

        /**
         * @brief Inflates a TinyRect by the specified value in all directions.
         *
         * @param rect Rectangle to inflate.
         * @param value Amount to subtract from left/top and add to right/bottom.
         * @return Inflated rectangle.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static TinyRect Inflate(const TinyRect& rect, intcs value);

        /**
         * @brief Returns true if a point lies inside or on the border of a rectangle.
         *
         * @param rect Rectangle to test.
         * @param p Point to test.
         * @return True if the point is inside the rectangle; otherwise false.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static bool IsInside(const TinyRect& rect, const TinyPoint& p);

        /**
         * @brief Computes the intersection of two rectangles.
         *
         * The destination rectangle is filled with the overlapping area of
         * @p src1 and @p src2.
         *
         * @param dst Output rectangle receiving the intersection.
         * @param src1 First source rectangle.
         * @param src2 Second source rectangle.
         * @return True if the resulting rectangle is non-empty; otherwise false.
         *
         * @note Status: Ported
         */
        static bool IntersectRect(TinyRect& dst, const TinyRect& src1, const TinyRect& src2);

        /**
         * @brief Computes the union of two rectangles.
         *
         * The destination rectangle is filled with the bounding rectangle
         * covering both @p src1 and @p src2.
         *
         * @param dst Output rectangle receiving the union.
         * @param src1 First source rectangle.
         * @param src2 Second source rectangle.
         * @return True if the resulting rectangle is non-empty; otherwise false.
         *
         * @note Status: Ported
         */
        static bool UnionRect(TinyRect& dst, const TinyRect& src1, const TinyRect& src2);

    private:
        /**
         * @brief Returns true if the specified rectangle is empty.
         *
         * A rectangle is considered empty if its width is less than or equal to zero
         * or its height is less than or equal to zero.
         *
         * @param rect Rectangle to test.
         * @return True if the rectangle is empty; otherwise false.
         *
         * @note Status: Ported
         */
        [[nodiscard]] static bool IsRectEmpty(const TinyRect& rect);
    };
}
