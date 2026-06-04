#pragma once
#include <vector>
#include <SFML/Graphics/Color.hpp>
#include "drawable.hpp"
#include "point.hpp"

// A "pseudo-ellipsoid": conceptually a sphere that has been deformed by an
// ordered sequence of axis-aligned scalings and rotations. The transform is kept
// as that literal sequence of elementary operations (no matrices); the ellipsoid
// is whatever the base sphere becomes once they are applied in order.
//
// The centre is the pivot of every operation, so it never moves; only the
// sphere's radius-vectors are deformed. The ellipsoid is drawn as a single
// translucent filled oval: the exact screen silhouette traced from its three
// projected semi-axes.
class PseudoEllipse : public Drawable
{
    public:
        struct Op
        {
            enum class Kind
            {
                Scale,  // vector holds per-axis factors
                Rotate, // vector holds the (unit) rotation axis, angle the radians
            };

            Kind kind;
            Point vector;
            float angle = 0.0f;
        };

    private:
        Point v_center;
        float v_radius = 1.0f;
        std::vector<Op> v_ops; // applied in order to each base radius-vector
        sf::Color v_color{0, 128, 0, 30};

        // Apply the whole op sequence to a radius-vector of the base sphere.
        Point Deform(const Point& v) const;

    public:
        PseudoEllipse() = default;
        PseudoEllipse(const Point& center, float radius,
                      const sf::Color& color = sf::Color(0, 128, 0, 30));

        // Append one more deformation to the sequence.
        void Scale(const Point& factors);
        void Rotate(const Point& axis, float angleRadians);

        const Point& Center() const { return v_center; }
        float Radius() const { return v_radius; }

        void Draw(sf::RenderTarget& target, const Camera& camera) const override;
};
