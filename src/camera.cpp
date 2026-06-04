#include "camera.hpp"
#include <cmath>
#include <numbers>

namespace
{
    // The near plane: world points closer than this along the forward axis are
    // treated as being behind the camera.
    constexpr float c_nearPlane = 1e-4f;

    // Rotate v about a (unit-length) axis by angle, using Rodrigues' formula.
    Point RotateAroundAxis(const Point& v, const Point& axis, float angle)
    {
        float c = std::cos(angle), s = std::sin(angle);
        return v * c + axis.Cross(v) * s + axis * (axis.Dot(v) * (1.0f - c));
    }
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

namespace
{
    // Re-orthonormalise the basis from forward and up using Gram-Schmidt,
    // matching SetOrientation's conventions (right = up x forward) but keeping
    // the camera's *current* up rather than snapping back to world up. This is
    // what lets the camera roll through the poles and rotate freely.
    void ReorthonormalizeBasis(Point& right, Point& up, Point& forward)
    {
        forward.Normalize();
        right = up.Cross(forward);
        right.Normalize();
        up = forward.Cross(right);
        up.Normalize();
    }
}

void Camera::Rotate(float yawRadians, float pitchRadians)
{
    // Yaw and pitch about the camera's own up and right axes. Because the basis
    // turns with the view (no world-up reference), there are no poles to clamp
    // against and the camera can rotate freely through any angle.
    v_forward = RotateAroundAxis(v_forward, v_up, yawRadians);
    v_right   = RotateAroundAxis(v_right, v_up, yawRadians);

    v_forward = RotateAroundAxis(v_forward, v_right, pitchRadians);
    v_up      = RotateAroundAxis(v_up, v_right, pitchRadians);

    ReorthonormalizeBasis(v_right, v_up, v_forward);
}

void Camera::Orbit(const Point& pivot, float yawRadians, float pitchRadians)
{
    // Vector from the pivot out to the camera; rotating it moves the camera
    // around the pivot while the pivot itself stays put.
    Point offset = v_position - pivot;

    // Rotate the offset and the whole basis together about the camera's own up
    // (yaw) and right (pitch) axes. Rotating in the camera's local frame means
    // there is no world-up singularity, so the orbit is free in every direction.
    Point yawAxis = v_up;
    offset    = RotateAroundAxis(offset, yawAxis, yawRadians);
    v_forward = RotateAroundAxis(v_forward, yawAxis, yawRadians);
    v_right   = RotateAroundAxis(v_right, yawAxis, yawRadians);

    Point pitchAxis = v_right;
    offset    = RotateAroundAxis(offset, pitchAxis, pitchRadians);
    v_forward = RotateAroundAxis(v_forward, pitchAxis, pitchRadians);
    v_up      = RotateAroundAxis(v_up, pitchAxis, pitchRadians);

    v_position = pivot + offset;
    ReorthonormalizeBasis(v_right, v_up, v_forward);
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
