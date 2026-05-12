#pragma once

#include <string>
#include <sstream>

#include "TinyPoint.hpp"
#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    using SharpRuntime::intcs;

    /**
     * @brief Represents a tiny rectangle using integer boundaries.
     *
     * This structure is a C++ port of the original C# TinyRect struct
     * from GalaxyEggbert.
     *
     * The rectangle is defined by its left, right, top, and bottom edges.
     *
     * @note Status: Ported
     */
    struct TinyRect
    {
        /**
         * @brief Left edge of the rectangle.
         *
         * @note Status: Ported
         */
        intcs Left;

        /**
         * @brief Right edge of the rectangle.
         *
         * @note Status: Ported
         */
        intcs Right;

        /**
         * @brief Top edge of the rectangle.
         *
         * @note Status: Ported
         */
        intcs Top;

        /**
         * @brief Bottom edge of the rectangle.
         *
         * @note Status: Ported
         */
        intcs Bottom;

        /**
         * @brief Creates a rectangle with all edges set to zero.
         *
         * This constructor is useful in C++ for default/value initialization,
         * for example when porting C# expressions such as default(TinyRect).
         *
         * @note Status: Ported
         */
        TinyRect()
            : Left(0), Right(0), Top(0), Bottom(0)
        {
        }

        /**
         * @brief Creates a rectangle with the specified edge coordinates.
         *
         * @warning The parameter order intentionally follows the original
         * GalaxyEggbert TinyRect layout: left, right, top, bottom.
         * This is different from the more common left, top, right, bottom order.
         *
         * @param left Left edge.
         * @param right Right edge.
         * @param top Top edge.
         * @param bottom Bottom edge.
         *
         * @note Status: IMPLEMENTED
         */
        TinyRect(const intcs left, const intcs right, const intcs top, const intcs bottom)
            : Left(left), Right(right), Top(top), Bottom(bottom)
        {
        }

        /**
 * @brief Creates a zero-size rectangle at the specified point.
 *
 * The point is used as both the left/right and top/bottom edge.
 * This is useful for icon drawing methods where a TinyRect with zero width
 * and height means "use the icon's default size".
 *
 * @param point Position of the zero-size rectangle.
 *
 * @note Status: IMPLEMENTED
 */
        explicit TinyRect(TinyPoint point);

        /**
         * @brief Gets the width of the rectangle.
         *
         * The width is computed as Right - Left.
         *
         * @return Rectangle width.
         *
         * @note Status: Ported
         */
        [[nodiscard]] intcs getWidthProperty() const;

        /**
         * @brief Gets the height of the rectangle.
         *
         * The height is computed as Bottom - Top.
         *
         * @return Rectangle height.
         *
         * @note Status: Ported
         */
        [[nodiscard]] intcs getHeightProperty() const;

        /**
         * @brief Returns the rectangle as a string in the format
         * "Left;Top;Right;Bottom".
         *
         * @return String representation of the rectangle.
         *
         * @note Status: Ported
         */
        [[nodiscard]] std::string ToString() const
        {
            std::ostringstream oss;
            oss << Left << ";" << Top << ";" << Right << ";" << Bottom;
            return oss.str();
        }
    };
}
