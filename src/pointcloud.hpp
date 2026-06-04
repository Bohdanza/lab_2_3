#pragma once
#include <cstddef>
#include <random>
#include <utility>
#include <vector>
#include "coordgrid.hpp"
#include "drawable.hpp"
#include "point.hpp"
#include "visualpoint.hpp"

// A cloud of VisualPoints, normally distributed inside a parallelepiped and
// drawn through a Camera.
//
// Each point is coloured by how crowded its local neighbourhood is: the colour
// ranges from white (the smallest summary distance to its 4 closest neighbours,
// i.e. the most tightly packed point) to green (the largest summary distance,
// i.e. the most isolated point).
//
// The cloud can be rotated about and scaled along arbitrary axes, and copy
// construction performs a full deep copy.
class PointCloud : public Drawable
{
    private:
        std::vector<VisualPoint> v_points;

        // A cosmetic coordinate grid drawn at the cloud's centroid. It rotates
        // together with the cloud (see Rotate) so it always shows the cloud's
        // current orientation.
        CoordGrid v_grid;

        // Centre of mass of all current points; the origin used by Rotate/Scale.
        Point Centroid() const;

        // Recompute every point's colour from the summary distance to its 4
        // closest neighbours. Called after the cloud is built and after any
        // operation that can change inter-point distances (i.e. scaling).
        void Recolor();

    public:
        // Generate pointCount points. Each coordinate is drawn from a normal
        // distribution centred on the origin, with the parallelepiped dimensions
        // spanning roughly +-3 standard deviations along each axis (so the vast
        // majority of points fall inside the box dimensions.X/Y/Z).
        PointCloud(std::size_t pointCount,
                   const Point& dimensions,
                   unsigned seed = std::random_device{}(),
                   float pointRadius = 3.0f);

        // Deep copy: every VisualPoint is copied by value.
        PointCloud(const PointCloud& other);
        PointCloud& operator=(const PointCloud& other) = default;

        std::size_t Size() const { return v_points.size(); }
        const std::vector<VisualPoint>& Points() const { return v_points; }
        const CoordGrid& Grid() const { return v_grid; }

        // Rotate the whole cloud by angleRadians about an axis through its
        // centroid (Rodrigues' rotation; axis need not be normalised).
        void Rotate(const Point& axis, float angleRadians);

        // Scale component-wise about the centroid (independent factors per axis).
        void Scale(const Point& factors);
        // Uniform scale about the centroid.
        void Scale(float factor);
        // Stretch the cloud by factor along an arbitrary axis direction, leaving
        // the perpendicular extent unchanged.
        void ScaleAlongAxis(const Point& axis, float factor);

        // Collapse the cloud onto the plane through its centroid perpendicular to
        // `normal` (i.e. project every point onto that plane). Equivalent to
        // scaling by zero along `normal`.
        void ProjectOntoPlane(const Point& normal) { ScaleAlongAxis(normal, 0.0f); }

        // Collapse the cloud onto the line through its centroid along
        // `direction` (i.e. keep only the component parallel to `direction`).
        void ProjectOntoLine(const Point& direction);

        // Brute-force search for the two points that are furthest apart. Returns
        // their indices into Points(); {0, 0} when fewer than two points exist.
        std::pair<std::size_t, std::size_t> FurthestPair() const;

        void Draw(sf::RenderTarget& target, const Camera& camera) const override;
};
