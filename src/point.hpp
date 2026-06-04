#pragma once
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <SFML/System/Vector3.hpp>

class Point
{
    private:
        float v_x = 0, v_y = 0, v_z = 0;
        float v_length = -1;
    public:
        const float& X() const { return v_x; }
        const float& Y() const { return v_y; }
        const float& Z() const { return v_z; }

        Point() = default;
        Point(float x, float y, float z) : v_x(x), v_y(y), v_z(z) {}
        Point(const Point& point) = default;
        Point& operator=(const Point& point) = default;
        explicit Point(const sf::Vector3f& vector) : v_x(vector.x), v_y(vector.y), v_z(vector.z) {}
        explicit Point(const sf::Vector3i& vector) : v_x(vector.x), v_y(vector.y), v_z(vector.z) {}

        void SetX(float x)
        {
            SetCoords(x, v_y, v_z);
        }

        void SetY(float y)
        {
            SetCoords(v_x, y, v_z);
        }

        void SetZ(float z)
        {
            SetCoords(v_x, v_y, z);
        }

        void SetCoords(float x, float y, float z)
        {
            v_x = x;
            v_y = y;
            v_z = z;
            v_length = -1;
        }
        void SetCoords(const Point& point)
        {
            SetCoords(point.X(), point.Y(), point.Z());
        }

        // azimuth measured in the XY plane, inclination measured from the +Z axis.
        void SetSphericalCoords(float azimuth, float inclination, float distance)
        {
            float sinIncl = std::sin(inclination);
            SetCoords(distance * sinIncl * std::cos(azimuth),
                      distance * sinIncl * std::sin(azimuth),
                      distance * std::cos(inclination));
        }

        float GetLength()
        {
            if (v_length != -1)
                return v_length;

            return v_length = std::sqrt(v_x * v_x + v_y * v_y + v_z * v_z);
        }

        // angle in the XY plane, range [0, 2*pi).
        float GetAzimuth() const
        {
            if (v_x == 0 && v_y == 0)
                return 0;

            return std::numbers::pi_v<float> + std::atan2(v_y, v_x);
        }

        // angle from the +Z axis, range [0, pi].
        float GetInclination()
        {
            float ln = GetLength();

            if (ln == 0)
                return 0;

            return std::acos(v_z / ln);
        }

        float DistanceToPoint(const Point& point) const
        {
            Point diff = point - *this;
            return diff.GetLength();
        }

        float Dot(const Point& point) const
        {
            return v_x * point.X() + v_y * point.Y() + v_z * point.Z();
        }

        Point Cross(const Point& point) const
        {
            return Point(v_y * point.Z() - v_z * point.Y(),
                         v_z * point.X() - v_x * point.Z(),
                         v_x * point.Y() - v_y * point.X());
        }

        void RotateX(float rotation)
        {
            float c = std::cos(rotation), s = std::sin(rotation);
            SetCoords(v_x, v_y * c - v_z * s, v_y * s + v_z * c);
        }

        void RotateY(float rotation)
        {
            float c = std::cos(rotation), s = std::sin(rotation);
            SetCoords(v_x * c + v_z * s, v_y, -v_x * s + v_z * c);
        }

        void RotateZ(float rotation)
        {
            float c = std::cos(rotation), s = std::sin(rotation);
            SetCoords(v_x * c - v_y * s, v_x * s + v_y * c, v_z);
        }

        Point GetRotatedX(float rotation)
        {
            Point p1 = *this;
            p1.RotateX(rotation);
            return p1;
        }

        Point GetRotatedY(float rotation)
        {
            Point p1 = *this;
            p1.RotateY(rotation);
            return p1;
        }

        Point GetRotatedZ(float rotation)
        {
            Point p1 = *this;
            p1.RotateZ(rotation);
            return p1;
        }

        void Normalize()
        {
            double ln = GetLength();

            if (ln == 0)
                return;

            v_x /= ln;
            v_y /= ln;
            v_z /= ln;
            v_length = 1;
        }

        Point GetNormalized()
        {
            Point p1 = *this;
            p1.Normalize();
            return p1;
        }

        Point operator+(const Point& pointToAdd) const
        {
            return Point(v_x + pointToAdd.X(), v_y + pointToAdd.Y(), v_z + pointToAdd.Z());
        }
        Point& operator+=(const Point& pointToAdd)
        {
            SetCoords(v_x + pointToAdd.X(), v_y + pointToAdd.Y(), v_z + pointToAdd.Z());

            return *this;
        }

        Point operator-(const Point& pointToAdd) const
        {
            return Point(v_x - pointToAdd.X(), v_y - pointToAdd.Y(), v_z - pointToAdd.Z());
        }

        Point& operator-=(const Point& pointToAdd)
        {
            SetCoords(v_x - pointToAdd.X(), v_y - pointToAdd.Y(), v_z - pointToAdd.Z());

            return *this;
        }

        Point operator-()
        {
            return Point(-v_x, -v_y, -v_z);
        }

        Point operator*(const float& multiplicator) const
        {
            return Point(v_x * multiplicator, v_y * multiplicator, v_z * multiplicator);
        }

        Point& operator*=(const float& multiplicator)
        {
            SetCoords(v_x * multiplicator, v_y * multiplicator, v_z * multiplicator);

            return *this;
        }

        Point operator/(const float& divisor) const
        {
            return operator*(1 / divisor);
        }

        Point& operator/=(const float& divisor)
        {
            return operator*=(1 / divisor);
        }

        bool operator==(const Point& point) const
        {
            return v_x == point.X() && v_y == point.Y() && v_z == point.Z();
        }

        template<typename T>
        explicit operator sf::Vector3<T>() const
        {
            return sf::Vector3<T>(static_cast<T>(v_x), static_cast<T>(v_y), static_cast<T>(v_z));
        }

        uint64_t Hash() const
        {
            return Hash(0);
        }
        // roundPrecision is the inverse of max precision (e.g. 100 == 2 decimal places).
        uint64_t Hash(int roundPrecision) const
        {
            float x = v_x, y = v_y, z = v_z;

            if (roundPrecision > 0)
            {
                x = std::round(x * roundPrecision) / roundPrecision;
                y = std::round(y * roundPrecision) / roundPrecision;
                z = std::round(z * roundPrecision) / roundPrecision;
            }

            // Three 32-bit components don't fit in 64 bits, so mix them (FNV-style).
            uint64_t h = std::bit_cast<uint32_t>(x);
            h = (h * 0x100000001b3ULL) ^ std::bit_cast<uint32_t>(y);
            h = (h * 0x100000001b3ULL) ^ std::bit_cast<uint32_t>(z);
            return h;
        }
};
