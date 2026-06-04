#pragma once
#include <optional>
#include <SFML/System/Vector2.hpp>
#include "point.hpp"

// A perspective camera with a fixed 90 degree horizontal field of view.
//
// It stores its position and orientation in world space and exposes the two
// projections that drawables rely on:
//   * WorldToScreen - project a world point onto the viewport (in pixels).
//   * ScreenToWorld - unproject a pixel back into world space at a chosen depth.
//
// Orientation is kept as an orthonormal basis (right / up / forward). Callers
// set it through SetOrientation or LookAt, which re-orthogonalise the vectors so
// the basis always stays valid.
class Camera
{
    private:
        // The horizontal field of view is fixed at 90 degrees.
        static constexpr float c_fovDegrees = 90.0f;

        Point v_position;
        // Orthonormal basis. forward is the viewing direction, right points to
        // the right of the screen, up points towards the top of the screen.
        Point v_right   {1, 0, 0};
        Point v_up      {0, 1, 0};
        Point v_forward {0, 0, 1};

        sf::Vector2f v_viewport {800, 600};

        // Distance, in pixels, from the eye to the projection plane. Derived
        // from the viewport width and the fixed field of view.
        float FocalLength() const;

    public:
        Camera() = default;
        Camera(const Point& position, const sf::Vector2f& viewport);

        const Point& Position() const { return v_position; }
        const Point& Right()    const { return v_right; }
        const Point& Up()       const { return v_up; }
        const Point& Forward()  const { return v_forward; }
        const sf::Vector2f& Viewport() const { return v_viewport; }

        static constexpr float Fov() { return c_fovDegrees; }

        void SetPosition(const Point& position) { v_position = position; }
        void SetViewport(const sf::Vector2f& viewport) { v_viewport = viewport; }
        void Move(const Point& offset) { v_position += offset; }

        // Build the basis from a desired forward direction and an approximate up
        // vector (which only needs to be non-parallel to forward).
        void SetOrientation(const Point& forward, const Point& up = Point(0, 1, 0));
        // Orient the camera so it faces target from the current position.
        void LookAt(const Point& target, const Point& up = Point(0, 1, 0));

        // Mouse-look: yaw and pitch about the camera's own up and right axes.
        // The basis turns with the view, so there is no pole to clamp against
        // and the camera can rotate freely through any angle.
        void Rotate(float yawRadians, float pitchRadians);

        // Orbit around a pivot: the camera circles the pivot (yaw and pitch
        // about its own up and right axes) and keeps looking at it, so the pivot
        // stays fixed at the centre of the view. Rotation is unconstrained.
        void Orbit(const Point& pivot, float yawRadians, float pitchRadians);

        // Project a world point onto the viewport. Returns std::nullopt when the
        // point lies behind the camera (and therefore cannot be drawn).
        std::optional<sf::Vector2f> WorldToScreen(const Point& world) const;

        // Unproject a screen pixel into world space. depth is the distance from
        // the camera along its forward axis at which the result lies.
        Point ScreenToWorld(const sf::Vector2f& screen, float depth) const;
};
