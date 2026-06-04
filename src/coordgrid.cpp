#include "coordgrid.hpp"
#include <array>
#include <cmath>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Vertex.hpp>

namespace
{
    // Rotate v about a (unit-length) axis by angle, using Rodrigues' formula.
    Point RotateAroundAxis(const Point& v, const Point& axis, float angle)
    {
        float c = std::cos(angle), s = std::sin(angle);
        return v * c + axis.Cross(v) * s + axis * (axis.Dot(v) * (1.0f - c));
    }
}

CoordGrid::CoordGrid(const Point& xDirection, float length, const Point& origin)
    : v_origin(origin)
{
    Point ax = xDirection;
    if (ax.GetLength() == 0)
        ax = Point(1, 0, 0); // fall back to the world X axis for a null direction
    ax.Normalize();

    // Pick a helper vector that is not parallel to ax, then cross to obtain a
    // perpendicular Y axis; Z follows as X x Y for a right-handed triple.
    Point helper = (std::abs(ax.X()) < 0.9f) ? Point(1, 0, 0) : Point(0, 1, 0);
    Point ay = ax.Cross(helper);
    ay.Normalize();
    Point az = ax.Cross(ay);
    az.Normalize();

    v_axisX = ax * length;
    v_axisY = ay * length;
    v_axisZ = az * length;
}

void CoordGrid::Rotate(const Point& axis, float angleRadians)
{
    Point k = axis;
    if (k.GetLength() == 0)
        return;
    k.Normalize();

    v_axisX = RotateAroundAxis(v_axisX, k, angleRadians);
    v_axisY = RotateAroundAxis(v_axisY, k, angleRadians);
    v_axisZ = RotateAroundAxis(v_axisZ, k, angleRadians);
}

void CoordGrid::Draw(sf::RenderTarget& target, const Camera& camera) const
{
    const std::array<std::pair<Point, sf::Color>, 3> axes{{
        {v_axisX, v_colorX},
        {v_axisY, v_colorY},
        {v_axisZ, v_colorZ},
    }};

    for (const auto& [axis, color] : axes)
    {
        // Draw each axis as a full line through the origin so it extends into
        // both positive and negative values.
        std::optional<sf::Vector2f> start = camera.WorldToScreen(v_origin - axis);
        std::optional<sf::Vector2f> end = camera.WorldToScreen(v_origin + axis);

        // Skip an axis whose either tip lies behind the camera; clipping a
        // single line segment is not worth the complexity for a cosmetic aid.
        if (!start || !end)
            continue;

        std::array<sf::Vertex, 2> line{
            sf::Vertex{*start, color},
            sf::Vertex{*end, color},
        };
        target.draw(line.data(), line.size(), sf::PrimitiveType::Lines);
    }
}
