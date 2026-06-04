#include <SFML/Graphics.hpp>
#include "camera.hpp"
#include "pointcloud.hpp"

int main()
{
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Point Cloud", sf::State::Fullscreen);
    window.setFramerateLimit(60);

    Camera camera(Point(0, 0, -12), sf::Vector2f(window.getSize()));
    camera.LookAt(Point(0, 0, 0));

    // 1000 points normally distributed in a 6x6x6 parallelepiped.
    PointCloud cloud(1000, Point(6, 6, 6), /*seed=*/42);

    // Right mouse = look around; middle mouse = drag (pan) the view.
    constexpr float rotateSpeed = 0.005f; // radians per pixel of mouse motion
    sf::Vector2i previousMouse = sf::Mouse::getPosition(window);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
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
        cloud.Draw(window, camera);
        window.display();
    }

    return 0;
}
