#pragma once
#include <SFML/Graphics/RenderTarget.hpp>
#include "camera.hpp"

// Base class for anything that lives in world space and is drawn through a
// Camera. Concrete drawables (VisualPoint and any future ones) implement Draw,
// using the camera's WorldToScreen to map their geometry onto the viewport.
class Drawable
{
    public:
        virtual ~Drawable() = default;
        virtual void Draw(sf::RenderTarget& target, const Camera& camera) const = 0;
};
