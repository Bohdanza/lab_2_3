#include "visualpoint.hpp"
#include <SFML/Graphics/CircleShape.hpp>

VisualPoint::VisualPoint(const Point& point, const sf::Color& color, float radius)
    : v_point(point), v_color(color), v_radius(radius)
{
}

void VisualPoint::Draw(sf::RenderTarget& target, const Camera& camera) const
{
    std::optional<sf::Vector2f> screen = camera.WorldToScreen(v_point);

    // Nothing to draw when the point sits behind the camera.
    if (!screen)
        return;

    sf::CircleShape disc(v_radius);
    disc.setFillColor(v_color);
    // setPosition places the top-left of the bounding box, so offset by the
    // radius to centre the disc on the projected point.
    disc.setOrigin({v_radius, v_radius});
    disc.setPosition(*screen);

    target.draw(disc);
}
