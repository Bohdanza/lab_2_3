#pragma once
#include <SFML/Graphics/Color.hpp>
#include "drawable.hpp"
#include "point.hpp"

// A purely cosmetic coordinate grid: three mutually perpendicular axis vectors
// (X, Y, Z) drawn from a common origin.
//
// The direction of the X axis and the shared length of all three axes are given
// at construction; the Y and Z axes are derived so that the three form a
// right-handed orthogonal triple. The grid carries its own orientation, so it
// can be rotated to follow whatever object it annotates (see Rotate), and moved
// via SetOrigin.
class CoordGrid : public Drawable
{
    private:
        Point v_origin;
        // The three axis vectors, each already scaled to the requested length.
        Point v_axisX, v_axisY, v_axisZ;

        sf::Color v_colorX = sf::Color::Red;
        sf::Color v_colorY = sf::Color::Green;
        sf::Color v_colorZ = sf::Color(64, 128, 255); // blue

    public:
        // xDirection sets the direction of the X axis (need not be normalised);
        // length is the common length of all three axes. The Y and Z axes are
        // generated perpendicular to X and to each other.
        CoordGrid(const Point& xDirection, float length, const Point& origin = Point());

        const Point& Origin() const { return v_origin; }
        const Point& AxisX()  const { return v_axisX; }
        const Point& AxisY()  const { return v_axisY; }
        const Point& AxisZ()  const { return v_axisZ; }

        void SetOrigin(const Point& origin) { v_origin = origin; }

        // Rotate all three axes about an arbitrary axis (Rodrigues' formula; the
        // axis need not be normalised). The origin is left unchanged, so rotate
        // it separately if the grid should orbit a different pivot.
        void Rotate(const Point& axis, float angleRadians);

        void Draw(sf::RenderTarget& target, const Camera& camera) const override;
};
