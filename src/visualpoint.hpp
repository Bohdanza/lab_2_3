#pragma once
#include <SFML/Graphics/Color.hpp>
#include "drawable.hpp"
#include "point.hpp"

// A Point that can be rendered. It owns a world-space position and a colour, and
// draws itself as a small filled disc at the projected screen location supplied
// by the Camera.
class VisualPoint : public Drawable
{
    private:
        Point v_point;
        sf::Color v_color = sf::Color::White;
        // On-screen radius of the drawn disc, in pixels.
        float v_radius = 4.0f;

    public:
        VisualPoint() = default;
        explicit VisualPoint(const Point& point, const sf::Color& color = sf::Color::White, float radius = 4.0f);

        const Point& GetPoint() const { return v_point; }
        const sf::Color& GetColor() const { return v_color; }
        float GetRadius() const { return v_radius; }

        void SetPoint(const Point& point) { v_point = point; }
        void SetColor(const sf::Color& color) { v_color = color; }
        void SetRadius(float radius) { v_radius = radius; }

        void Draw(sf::RenderTarget& target, const Camera& camera) const override;
};
