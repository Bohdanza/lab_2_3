#include <array>
#include <optional>
#include <vector>
#include <SFML/Graphics.hpp>
#include "camera.hpp"
#include "pointcloud.hpp"
#include "segment.hpp"
#include "visualpoint.hpp"

namespace
{
    // The interactive construction advances one step per Tab press. Each step is
    // computed in full the moment it is entered, so a step is always "finished"
    // before the next Tab can start the following one.
    enum class Step
    {
        Cloud,        // 1. the normally distributed cloud (built up front)
        Furthest3D,   // 2. furthest pair in 3D, joined by a red segment
        FlattenedPlane, // 3. cloud flattened onto a plane; furthest pair there
        FlattenedLine,  // 4. cloud collapsed onto a line; furthest pair there
        Result,       // 5. the original cloud with all marked points and segments
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
        std::array<Point, 3> directions{}; // segment directions per stage

        std::optional<PointCloud> planeCloud; // step 3: flattened onto a plane
        std::optional<PointCloud> lineCloud;  // step 4: collapsed onto a line

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
                segments = {Segment(a, b)};
                step = Step::FlattenedLine;
                break;
            }
            case Step::FlattenedLine:
            {
                // Step 5: back on the original cloud, mark every found point in
                // red and join each pair with a red segment.
                segments.clear();
                markers.clear();
                for (std::size_t s = 0; s < pairs.size(); ++s)
                {
                    auto [a, b] = Endpoints(s);
                    segments.emplace_back(a, b, sf::Color::Red);
                    markers.emplace_back(a, sf::Color::Red, 6.0f);
                    markers.emplace_back(b, sf::Color::Red, 6.0f);
                }
                step = Step::Result;
                break;
            }
            case Step::Result:
                break; // construction complete; further Tabs do nothing
        }
    }

    void Construction::Draw(sf::RenderTarget& target, const Camera& camera) const
    {
        // Steps 3 and 4 show their derived cloud; every other step shows the
        // original cloud.
        const PointCloud* shown = &cloud;
        if (step == Step::FlattenedPlane && planeCloud)
            shown = &*planeCloud;
        else if (step == Step::FlattenedLine && lineCloud)
            shown = &*lineCloud;

        shown->Draw(target, camera);

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
