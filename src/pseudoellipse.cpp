#include "pseudoellipse.hpp"
#include <cmath>
#include <numbers>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Vertex.hpp>

PseudoEllipse::PseudoEllipse(const Point& center, float radius, const sf::Color& color)
    : v_center(center), v_radius(radius), v_color(color)
{
}

void PseudoEllipse::Scale(const Point& factors)
{
    v_ops.push_back({Op::Kind::Scale, factors, 0.0f});
}

void PseudoEllipse::Rotate(const Point& axis, float angleRadians)
{
    Point k = axis;
    if (k.GetLength() == 0)
        return;
    k.Normalize();
    v_ops.push_back({Op::Kind::Rotate, k, angleRadians});
}

Point PseudoEllipse::Deform(const Point& v) const
{
    Point p = v;
    for (const Op& op : v_ops)
    {
        if (op.kind == Op::Kind::Scale)
        {
            p = Point(p.X() * op.vector.X(),
                      p.Y() * op.vector.Y(),
                      p.Z() * op.vector.Z());
        }
        else
        {
            // Rodrigues' rotation about the (already normalised) op axis.
            float c = std::cos(op.angle), s = std::sin(op.angle);
            const Point& k = op.vector;
            p = p * c + k.Cross(p) * s + k * (k.Dot(p) * (1.0f - c));
        }
    }
    return p;
}

void PseudoEllipse::Draw(sf::RenderTarget& target, const Camera& camera) const
{
    // The ellipsoid's three world-space semi-axes are the base sphere's axis
    // vectors after the full deformation sequence.
    Point ax = Deform(Point(v_radius, 0, 0));
    Point ay = Deform(Point(0, v_radius, 0));
    Point az = Deform(Point(0, 0, v_radius));

    std::optional<sf::Vector2f> center = camera.WorldToScreen(v_center);
    if (!center)
        return; // centre is behind the camera

    // Project the semi-axes with a single perspective scale taken at the centre's
    // depth (a local orthographic projection), rather than dividing each axis tip
    // by its own depth. Per-tip perspective division foreshortens the axis that
    // points towards the camera differently from the others, which warps a sphere
    // into an ellipse; projecting all three at the same scale keeps a sphere's
    // silhouette a true circle and an ellipsoid's a faithful ellipse.
    float depth = (v_center - camera.Position()).Dot(camera.Forward());
    if (depth <= 1e-4f)
        return;

    float halfFov = (Camera::Fov() * 0.5f) * (std::numbers::pi_v<float> / 180.0f);
    float focal = (camera.Viewport().x * 0.5f) / std::tan(halfFov);
    float scale = focal / depth;

    // Screen-space images of the three semi-axes. Screen +y points down, so the
    // up component is negated (matching Camera::WorldToScreen).
    auto project = [&](const Point& a)
    {
        return sf::Vector2f(a.Dot(camera.Right()) * scale, -a.Dot(camera.Up()) * scale);
    };
    sf::Vector2f u = project(ax);
    sf::Vector2f v = project(ay);
    sf::Vector2f w = project(az);

    // Trace the exact silhouette. The ellipsoid is the image of the unit sphere
    // under the 2x3 screen map A = [u v w]; the boundary point whose outward
    // normal is the screen direction d is (A At d) / |At d|. That needs only the
    // three 2D dot products below - no matrices.
    constexpr int segmentCount = 48;
    std::vector<sf::Vertex> fan;
    fan.reserve(segmentCount + 2);
    fan.push_back(sf::Vertex{*center, v_color});

    for (int i = 0; i <= segmentCount; ++i)
    {
        float theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / segmentCount;
        sf::Vector2f d(std::cos(theta), std::sin(theta));

        float du = u.x * d.x + u.y * d.y;
        float dv = v.x * d.x + v.y * d.y;
        float dw = w.x * d.x + w.y * d.y;
        float m = std::sqrt(du * du + dv * dv + dw * dw);

        sf::Vector2f boundary = *center;
        if (m > 0.0f)
            boundary += (u * du + v * dv + w * dw) / m;

        fan.push_back(sf::Vertex{boundary, v_color});
    }

    target.draw(fan.data(), fan.size(), sf::PrimitiveType::TriangleFan);
}
