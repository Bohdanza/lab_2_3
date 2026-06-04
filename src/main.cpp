#include <SFML/Graphics.hpp>
#include "camera.hpp"
#include "pointcloud.hpp"

int main()
{
    sf::RenderWindow window(sf::VideoMode({800u, 600u}), "Point Cloud");
    window.setFramerateLimit(60);

    Camera camera(Point(0, 0, -12), sf::Vector2f(window.getSize()));
    camera.LookAt(Point(0, 0, 0));

    // 1000 points normally distributed in a 6x6x6 parallelepiped.
    PointCloud cloud(1000, Point(6, 6, 6), /*seed=*/42);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // Slowly spin the cloud about the vertical axis.
        cloud.Rotate(Point(0, 1, 0), 0.01f);

        window.clear(sf::Color::Black);
        cloud.Draw(window, camera);
        window.display();
    }

    return 0;
}
