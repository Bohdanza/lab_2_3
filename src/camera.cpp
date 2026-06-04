#include "camera.hpp"
#include <cmath>
#include <numbers>

namespace
{
    // The near plane: world points closer than this along the forward axis are
    // treated as being behind the camera.
    constexpr float c_nearPlane = 1e-4f;
}

Camera::Camera(const Point& position, const sf::Vector2f& viewport)
    : v_position(position), v_viewport(viewport)
{
}

float Camera::FocalLength() const
{
    // Half of the viewport width subtends half of the field of view, so:
    //   focal = (width / 2) / tan(fov / 2)
    float halfFov = (c_fovDegrees * 0.5f) * (std::numbers::pi_v<float> / 180.0f);
    return (v_viewport.x * 0.5f) / std::tan(halfFov);
}

void Camera::SetOrientation(const Point& forward, const Point& up)
{
    Point f = forward;
    f.Normalize();

    // right = up x forward, then re-derive a true up so the basis is orthonormal
    // even when the supplied up is not perpendicular to forward.
    Point r = up.Cross(f);
    r.Normalize();

    Point u = f.Cross(r);
    u.Normalize();

    v_forward = f;
    v_right   = r;
    v_up      = u;
}

void Camera::LookAt(const Point& target, const Point& up)
{
    SetOrientation(target - v_position, up);
}

std::optional<sf::Vector2f> Camera::WorldToScreen(const Point& world) const
{
    Point relative = world - v_position;

    // Coordinates in camera space.
    float cx = relative.Dot(v_right);
    float cy = relative.Dot(v_up);
    float cz = relative.Dot(v_forward);

    if (cz <= c_nearPlane)
        return std::nullopt;

    float focal = FocalLength();

    // Screen origin is the top-left corner; +x goes right, +y goes down, so the
    // camera-space up component is negated.
    float screenX = v_viewport.x * 0.5f + focal * (cx / cz);
    float screenY = v_viewport.y * 0.5f - focal * (cy / cz);

    return sf::Vector2f(screenX, screenY);
}

Point Camera::ScreenToWorld(const sf::Vector2f& screen, float depth) const
{
    float focal = FocalLength();

    // Convert the pixel back into camera-space directions on the projection
    // plane, then scale out to the requested depth.
    float cx = (screen.x - v_viewport.x * 0.5f) / focal * depth;
    float cy = -(screen.y - v_viewport.y * 0.5f) / focal * depth;

    return v_position + v_right * cx + v_up * cy + v_forward * depth;
}
