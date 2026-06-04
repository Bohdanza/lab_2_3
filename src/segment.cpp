#include "segment.hpp"
#include <array>
#include <cmath>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Vertex.hpp>

Segment::Segment(const Point& a, const Point& b, const sf::Color& color)
    : v_a(a), v_b(b), v_color(color)
{
}

void Segment::Draw(sf::RenderTarget& target, const Camera& camera) const
{
    std::optional<sf::Vector2f> start = camera.WorldToScreen(v_a);
    std::optional<sf::Vector2f> end = camera.WorldToScreen(v_b);

    // Skip the segment if either endpoint lies behind the camera; clipping a
    // single line is not worth the complexity here.
    if (!start || !end)
        return;

    // GL line primitives are always one pixel wide, so draw the segment as a
    // thin quad instead: offset both endpoints by half the thickness along the
    // screen-space normal of the line.
    constexpr float thickness = 2.0f;

    sf::Vector2f dir = *end - *start;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    // A degenerate (zero-length) segment has no direction; pick an arbitrary one
    // so it still shows up as a small dot.
    sf::Vector2f unit = (len > 0.0f) ? dir / len : sf::Vector2f(1.0f, 0.0f);
    sf::Vector2f normal(-unit.y, unit.x);
    sf::Vector2f offset = normal * (thickness * 0.5f);

    std::array<sf::Vertex, 4> quad{
        sf::Vertex{*start + offset, v_color},
        sf::Vertex{*start - offset, v_color},
        sf::Vertex{*end + offset, v_color},
        sf::Vertex{*end - offset, v_color},
    };
    target.draw(quad.data(), quad.size(), sf::PrimitiveType::TriangleStrip);
}
