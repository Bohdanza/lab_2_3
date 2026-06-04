#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>
#include <SFML/Graphics.hpp>
#include "camera.hpp"
#include "coordgrid.hpp"
#include "pointcloud.hpp"
#include "pseudoellipse.hpp"
#include "segment.hpp"
#include "visualpoint.hpp"

namespace
{
    // Rotate vector v about a (unit-length) axis by angle, using Rodrigues' formula.
    Point RotateVector(const Point& v, const Point& axis, float angle)
    {
        float c = std::cos(angle), s = std::sin(angle);
        return v * c + axis.Cross(v) * s + axis * (axis.Dot(v) * (1.0f - c));
    }

    // A rotation that aligns the orthonormal frame (f0, f1, f0xf1) with the world
    // axes (X, Y, Z), expressed as a literal sequence of two elementary
    // rotations: first about `axisA` by `angleA` to bring f0 onto X, then about
    // X by `angleB` to bring the rotated f1 onto Y. f0 and f1 must be unit length
    // and perpendicular.
    struct Alignment
    {
        Point axisA{0, 0, 1};
        float angleA = 0.0f;
        float angleB = 0.0f;
    };

    Alignment ComputeAlignment(const Point& f0, const Point& f1)
    {
        const Point worldX(1, 0, 0), worldY(0, 1, 0), worldZ(0, 0, 1);
        Alignment a;

        // Step one: rotate f0 onto the X axis.
        float dot = std::clamp(f0.Dot(worldX), -1.0f, 1.0f);
        Point cross = f0.Cross(worldX);
        if (cross.GetLength() < 1e-6f)
        {
            // f0 is already parallel to X: either aligned (no rotation) or
            // anti-parallel (a half turn about any perpendicular axis).
            a.axisA = worldZ;
            a.angleA = (dot > 0.0f) ? 0.0f : std::numbers::pi_v<float>;
        }
        else
        {
            a.axisA = cross.GetNormalized();
            a.angleA = std::acos(dot);
        }

        // Step two: f1 is perpendicular to f0, so after the first rotation it lies
        // in the YZ plane. Rotate about X to bring it onto Y.
        Point f1r = RotateVector(f1, a.axisA, a.angleA);
        a.angleB = -std::atan2(f1r.Dot(worldZ), f1r.Dot(worldY));
        return a;
    }

    // The interactive construction advances one step per Tab press. Each step is
    // computed in full the moment it is entered, so a step is always "finished"
    // before the next Tab can start the following one.
    enum class Step
    {
        Cloud,          // 1. the normally distributed cloud (built up front)
        Furthest3D,     // 2. furthest pair in 3D, joined by a red segment
        FlattenedPlane, // 3. cloud flattened onto a plane; furthest pair there
        FlattenedLine,  // 4. cloud collapsed onto a line; furthest pair there
        Result,         // 5. the original cloud with all marked points and segments
        Rotated,        // 6. cloud rotated so the three segments parallel the axes
        Cube,           // 7. cloud scaled along each axis into a cube
        Spheres,        // 8. concentric spheres built from the cube's centre
        Ellipsoids,     // 9. spheres mapped back, drawn over the step-5 cloud
    };

    // The whole Petunin construction: the source cloud plus everything derived
    // from it as the user steps through. Derived clouds are optional because they
    // only come into existence once their step is reached.
    struct Construction
    {
        PointCloud cloud;
        Step step = Step::Cloud;

        // Pairs of original-point indices furthest apart at each stage. Because
        // the projected clouds are copies that preserve point order, an index
        // found in a projection refers to the same original point.
        std::array<std::pair<std::size_t, std::size_t>, 3> pairs{};
        std::array<Point, 3> directions{};  // segment directions per stage
        std::array<float, 3> lengths{};     // segment lengths per stage

        std::optional<PointCloud> planeCloud; // step 3: flattened onto a plane
        std::optional<PointCloud> lineCloud;  // step 4: collapsed onto a line

        // Step 6-8 working copy (rotated, then scaled into a cube) and a copy of
        // the initial coordinate grid parked outside the cloud as a fixed
        // reference against which the rotation is visible.
        std::optional<PointCloud> transformedCloud;
        std::optional<CoordGrid> referenceGrid;

        // The rotation (step 6) and per-axis scaling (step 7) that turn the cloud
        // into a cube, stored so step 9 can undo them on the ellipsoids.
        Point center;            // pivot of every transform (the centroid)
        Alignment alignment;     // step 6 rotation
        Point scale{1, 1, 1};    // step 7 per-axis cube-scaling factors

        std::vector<PseudoEllipse> ellipses; // steps 8-9

        // What is drawn on top of whichever cloud is current.
        std::vector<Segment> segments;
        std::vector<VisualPoint> markers;

        explicit Construction(PointCloud source) : cloud(std::move(source)) {}

        // Endpoints of the furthest pair `index`, taken from the original cloud.
        std::pair<Point, Point> Endpoints(std::size_t index) const
        {
            const auto& [i, j] = pairs[index];
            return {cloud.Points()[i].GetPoint(), cloud.Points()[j].GetPoint()};
        }

        // Step 5 / step 9 overlay: the six found points in red, joined in pairs.
        void BuildResultVisuals()
        {
            segments.clear();
            markers.clear();
            for (std::size_t s = 0; s < pairs.size(); ++s)
            {
                auto [a, b] = Endpoints(s);
                segments.emplace_back(a, b, sf::Color::Red);
                markers.emplace_back(a, sf::Color::Red, 6.0f);
                markers.emplace_back(b, sf::Color::Red, 6.0f);
            }
        }

        // The three orthogonal segments, axis-aligned and centred on `center`,
        // with the given half-extents along X, Y and Z. Used once the frame has
        // been rotated onto the world axes (steps 6-7).
        void BuildAxisSegments(const std::array<float, 3>& lens)
        {
            segments.clear();
            markers.clear();
            const std::array<Point, 3> axes{Point(1, 0, 0), Point(0, 1, 0), Point(0, 0, 1)};
            for (std::size_t k = 0; k < axes.size(); ++k)
            {
                Point half = axes[k] * (lens[k] * 0.5f);
                segments.emplace_back(center - half, center + half, sf::Color::Red);
            }
        }

        void Advance();
        void Draw(sf::RenderTarget& target, const Camera& camera) const;
    };

    void Construction::Advance()
    {
        switch (step)
        {
            case Step::Cloud:
            {
                // Step 2: the two furthest apart points in the 3D cloud.
                pairs[0] = cloud.FurthestPair();
                auto [a, b] = Endpoints(0);
                directions[0] = b - a;
                lengths[0] = directions[0].GetLength();
                segments = {Segment(a, b)};
                step = Step::Furthest3D;
                break;
            }
            case Step::Furthest3D:
            {
                // Step 3: copy the cloud and flatten it onto the plane
                // perpendicular to the first segment, then find the furthest pair
                // living in that plane.
                planeCloud = cloud;
                planeCloud->ProjectOntoPlane(directions[0]);
                pairs[1] = planeCloud->FurthestPair();

                const auto& [i, j] = pairs[1];
                Point a = planeCloud->Points()[i].GetPoint();
                Point b = planeCloud->Points()[j].GetPoint();
                directions[1] = b - a;
                lengths[1] = directions[1].GetLength();
                segments = {Segment(a, b)};
                step = Step::FlattenedPlane;
                break;
            }
            case Step::FlattenedPlane:
            {
                // Step 4: the line perpendicular to both previous segments is
                // their cross product. Collapse a fresh copy onto it and find the
                // furthest pair on that 1D cloud.
                directions[2] = directions[0].Cross(directions[1]);
                lineCloud = cloud;
                lineCloud->ProjectOntoLine(directions[2]);
                pairs[2] = lineCloud->FurthestPair();

                const auto& [i, j] = pairs[2];
                Point a = lineCloud->Points()[i].GetPoint();
                Point b = lineCloud->Points()[j].GetPoint();
                lengths[2] = a.DistanceToPoint(b);
                segments = {Segment(a, b)};
                step = Step::FlattenedLine;
                break;
            }
            case Step::FlattenedLine:
            {
                // Step 5: back on the original cloud, mark every found point in
                // red and join each pair with a red segment.
                BuildResultVisuals();
                step = Step::Result;
                break;
            }
            case Step::Result:
            {
                // Step 6: rotate a fresh copy so the orthogonal direction frame
                // (segment 1 -> X, segment 2 -> Y, segment 3 -> Z) lands on the
                // world axes. Keep a fixed copy of the initial grid outside the
                // cloud so the rotation is visible against it.
                center = cloud.Centroid();
                alignment = ComputeAlignment(directions[0].GetNormalized(),
                                             directions[1].GetNormalized());

                transformedCloud = cloud;
                transformedCloud->Rotate(alignment.axisA, alignment.angleA);
                transformedCloud->Rotate(Point(1, 0, 0), alignment.angleB);
                // Hide this copy's own (now tilted) grid; the red axis segments
                // already show the aligned axes and the reference grid shows the
                // original orientation.
                transformedCloud->SetDrawGrid(false);

                // A fixed, world-aligned reference grid pinned to the cloud's
                // centre (the point-cloud origin). It stays put while the cloud
                // rotates, so the rotation is visible as the red axis segments
                // turning away from this grid's axes.
                Point xAxis = cloud.Grid().AxisX();
                float gridLength = xAxis.GetLength();
                referenceGrid = CoordGrid(Point(1, 0, 0), gridLength, center);

                BuildAxisSegments(lengths); // now parallel to the axes
                step = Step::Rotated;
                break;
            }
            case Step::Rotated:
            {
                // Step 7: scale along each (now axis-aligned) direction so all
                // three segments shrink to the shortest one - a cube.
                float side = std::min({lengths[0], lengths[1], lengths[2]});
                scale = Point(side / lengths[0], side / lengths[1], side / lengths[2]);
                transformedCloud->Scale(scale);

                BuildAxisSegments({side, side, side});
                step = Step::Cube;
                break;
            }
            case Step::Cube:
            {
                // Step 8: from the cube's centre, build one sphere ("degenerate
                // ellipsoid") per point, with radius equal to that point's
                // distance to the centre.
                ellipses.clear();
                ellipses.reserve(transformedCloud->Size());
                for (const VisualPoint& vp : transformedCloud->Points())
                {
                    float radius = vp.GetPoint().DistanceToPoint(center);
                    ellipses.emplace_back(center, radius);
                }
                step = Step::Spheres;
                break;
            }
            case Step::Spheres:
            {
                // Step 9: undo steps 6-7 on every sphere in reverse order
                // (scale back, then rotate back), turning them into the Petunin
                // ellipsoids, and draw them over the step-5 cloud.
                for (PseudoEllipse& e : ellipses)
                {
                    e.Scale(Point(1.0f / scale.X(), 1.0f / scale.Y(), 1.0f / scale.Z()));
                    e.Rotate(Point(1, 0, 0), -alignment.angleB);
                    e.Rotate(alignment.axisA, -alignment.angleA);
                }
                BuildResultVisuals();
                step = Step::Ellipsoids;
                break;
            }
            case Step::Ellipsoids:
                break; // construction complete; further Tabs do nothing
        }
    }

    void Construction::Draw(sf::RenderTarget& target, const Camera& camera) const
    {
        // Pick the cloud this step shows: the plane/line projections (3-4), the
        // rotated/cube working copy (6-8), or the original cloud (everything else,
        // including the step-9 result drawn over the step-5 cloud).
        const PointCloud* shown = &cloud;
        if (step == Step::FlattenedPlane && planeCloud)
            shown = &*planeCloud;
        else if (step == Step::FlattenedLine && lineCloud)
            shown = &*lineCloud;
        else if ((step == Step::Rotated || step == Step::Cube || step == Step::Spheres) && transformedCloud)
            shown = &*transformedCloud;

        shown->Draw(target, camera);

        // The fixed reference grid accompanies the rotated/cube views.
        if ((step == Step::Rotated || step == Step::Cube || step == Step::Spheres) && referenceGrid)
            referenceGrid->Draw(target, camera);

        for (const PseudoEllipse& e : ellipses)
            e.Draw(target, camera);
        for (const Segment& seg : segments)
            seg.Draw(target, camera);
        for (const VisualPoint& marker : markers)
            marker.Draw(target, camera);
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Point Cloud", sf::State::Fullscreen);
    window.setFramerateLimit(60);

    Camera camera(Point(0, 0, -12), sf::Vector2f(window.getSize()));
    camera.LookAt(Point(0, 0, 0));

    // 1000 points normally distributed in a 6x6x6 parallelepiped.
    Construction construction(PointCloud(1000, Point(6, 6, 6), /*seed=*/42));

    // Right mouse = look around; middle mouse = drag (pan) the view.
    constexpr float rotateSpeed = 0.005f; // radians per pixel of mouse motion
    sf::Vector2i previousMouse = sf::Mouse::getPosition(window);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Tab)
                    construction.Advance(); // advance to the next construction step
                else if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
        }

        sf::Vector2i currentMouse = sf::Mouse::getPosition(window);
        sf::Vector2i delta = currentMouse - previousMouse;

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            // Orbit around the scene centre so it stays pinned to the middle of
            // the screen as the camera circles it.
            camera.Orbit(Point(), delta.x * rotateSpeed, delta.y * rotateSpeed);
        }
        else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
        {
            // Drag on the plane (parallel to the viewport) that passes through
            // the scene centre, so the grabbed point tracks the cursor exactly.
            float depth = (Point() - camera.Position()).Dot(camera.Forward());
            if (depth > 0)
            {
                Point before = camera.ScreenToWorld(sf::Vector2f(previousMouse), depth);
                Point after = camera.ScreenToWorld(sf::Vector2f(currentMouse), depth);
                camera.Move(before - after);
            }
        }

        previousMouse = currentMouse;

        window.clear(sf::Color::Black);
        construction.Draw(window, camera);
        window.display();
    }

    return 0;
}
