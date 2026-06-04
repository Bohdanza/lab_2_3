#pragma once
#include <SFML/Graphics/Color.hpp>
#include "drawable.hpp"
#include "point.hpp"

// A straight line segment between two world-space endpoints, drawn as a single
// coloured line through the camera. A zero-length (degenerate) segment simply
// projects to a single pixel.
class Segment : public Drawable
{
    private:
        Point v_a, v_b;
        sf::Color v_color = sf::Color::Red;

    public:
        Segment() = default;
        Segment(const Point& a, const Point& b, const sf::Color& color = sf::Color::Red);

        const Point& A() const { return v_a; }
        const Point& B() const { return v_b; }
        void SetColor(const sf::Color& color) { v_color = color; }

        void Draw(sf::RenderTarget& target, const Camera& camera) const override;
};
