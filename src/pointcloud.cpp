#include "pointcloud.hpp"
#include <algorithm>
#include <cmath>

namespace
{
    // Linear interpolation from white (t = 0, tightly packed) to green
    // (t = 1, isolated). Only the red and blue channels change; green stays 255.
    sf::Color CrowdingColor(float t)
    {
        auto channel = static_cast<std::uint8_t>(std::round(255.0f * (1.0f - t)));
        return sf::Color(channel, 255, channel);
    }
}

PointCloud::PointCloud(std::size_t pointCount, const Point& dimensions, unsigned seed, float pointRadius)
    // The grid's X axis starts along world X; its length is the longest side of
    // the parallelepiped. Origin is fixed up to the centroid once points exist.
    : v_grid(Point(1, 0, 0),
             std::max({dimensions.X(), dimensions.Y(), dimensions.Z()}))
{
    std::mt19937 generator(seed);

    // Dimension spans ~6 standard deviations (+-3 sigma), so sigma = dim / 6.
    std::normal_distribution<float> distX(0.0f, dimensions.X() / 6.0f);
    std::normal_distribution<float> distY(0.0f, dimensions.Y() / 6.0f);
    std::normal_distribution<float> distZ(0.0f, dimensions.Z() / 6.0f);

    v_points.reserve(pointCount);
    for (std::size_t i = 0; i < pointCount; ++i)
        v_points.emplace_back(Point(distX(generator), distY(generator), distZ(generator)),
                              sf::Color::White, pointRadius);

    v_grid.SetOrigin(Centroid());
    Recolor();
}

PointCloud::PointCloud(const PointCloud& other) : v_points(other.v_points), v_grid(other.v_grid)
{
}

Point PointCloud::Centroid() const
{
    if (v_points.empty())
        return Point();

    Point sum;
    for (const VisualPoint& vp : v_points)
        sum += vp.GetPoint();

    return sum / static_cast<float>(v_points.size());
}

void PointCloud::Recolor()
{
    const std::size_t n = v_points.size();

    // With fewer than two points there are no neighbours to measure against.
    if (n < 2)
    {
        for (VisualPoint& vp : v_points)
            vp.SetColor(CrowdingColor(0.0f));
        return;
    }

    const std::size_t k = std::min<std::size_t>(4, n - 1);

    std::vector<float> summary(n);
    std::vector<float> distances;
    distances.reserve(n - 1);

    for (std::size_t i = 0; i < n; ++i)
    {
        distances.clear();
        for (std::size_t j = 0; j < n; ++j)
            if (j != i)
                distances.push_back(v_points[i].GetPoint().DistanceToPoint(v_points[j].GetPoint()));

        // Pull the k smallest distances to the front and sum them.
        std::partial_sort(distances.begin(), distances.begin() + k, distances.end());
        float s = 0.0f;
        for (std::size_t m = 0; m < k; ++m)
            s += distances[m];
        summary[i] = s;
    }

    auto [minIt, maxIt] = std::minmax_element(summary.begin(), summary.end());
    float minS = *minIt;
    float range = *maxIt - minS;

    for (std::size_t i = 0; i < n; ++i)
    {
        // White for the smallest summary distance, green for the largest.
        float t = (range > 0.0f) ? (summary[i] - minS) / range : 0.0f;
        v_points[i].SetColor(CrowdingColor(t));
    }
}

void PointCloud::Rotate(const Point& axis, float angleRadians)
{
    Point k = axis;
    if (k.GetLength() == 0)
        return;
    k.Normalize();

    float cosT = std::cos(angleRadians);
    float sinT = std::sin(angleRadians);
    Point c = Centroid();

    for (VisualPoint& vp : v_points)
    {
        Point p = vp.GetPoint() - c;
        // Rodrigues' rotation formula.
        Point rotated = p * cosT + k.Cross(p) * sinT + k * (k.Dot(p) * (1.0f - cosT));
        vp.SetPoint(rotated + c);
    }

    // Keep the grid aligned with the cloud: its axes turn by the same rotation
    // while its origin stays pinned to the (rotation-invariant) centroid.
    v_grid.Rotate(k, angleRadians);
    v_grid.SetOrigin(c);

    // Rotation preserves distances, so the crowding colours stay valid.
}

void PointCloud::Scale(const Point& factors)
{
    Point c = Centroid();

    for (VisualPoint& vp : v_points)
    {
        Point p = vp.GetPoint() - c;
        vp.SetPoint(Point(p.X() * factors.X(), p.Y() * factors.Y(), p.Z() * factors.Z()) + c);
    }

    Recolor();
}

void PointCloud::Scale(float factor)
{
    Point c = Centroid();

    for (VisualPoint& vp : v_points)
        vp.SetPoint((vp.GetPoint() - c) * factor + c);

    // A uniform scale multiplies every distance equally, so the normalised
    // crowding values - and therefore the colours - are unchanged.
}

void PointCloud::ScaleAlongAxis(const Point& axis, float factor)
{
    Point u = axis;
    if (u.GetLength() == 0)
        return;
    u.Normalize();

    Point c = Centroid();

    for (VisualPoint& vp : v_points)
    {
        Point p = vp.GetPoint() - c;
        Point along = u * p.Dot(u);   // component parallel to the axis
        Point perp = p - along;       // component perpendicular to the axis
        vp.SetPoint(perp + along * factor + c);
    }

    Recolor();
}

void PointCloud::Draw(sf::RenderTarget& target, const Camera& camera) const
{
    v_grid.Draw(target, camera);

    for (const VisualPoint& vp : v_points)
        vp.Draw(target, camera);
}
