#include "pch.h"
#include "Core.h"

#include <cmath>

namespace amor {
    namespace math {
        Vec3f::Vec3f() : x{ 0 }, y{ 0 }, z{ 0 } {}
        Vec3f::Vec3f(const Vec3f& o) : x{ o.x }, y{ o.y }, z{ o.z } {}
        Vec3f::Vec3f(real u) : x{ u }, y{ u }, z{ u } {}
        Vec3f::Vec3f(real x, real y, real z) : x{ x }, y{ y }, z{ z } {}

        Vec3f::~Vec3f() {};

        bool Vec3f::operator==(const Vec3f& o) const {
            return x == o.x && y == o.y && z == o.z;
        }
        bool Vec3f::operator!=(const Vec3f& o) const {
            return x != o.x || y != o.y || z != o.z;
        }

        real Vec3f::len() const {
            return (real)sqrt(x * x + y * y + z * z);
        }
        real Vec3f::len_squared() const {
            return x * x + y * y + z * z;
        }
        real Vec3f::angle2() const {
            return (real)atan2(y, x);
        }

        Vec3f Vec3f::normalized() const {
            return (*this) / this->len();
        }

        Vec3f Vec3f::operator+(const Vec3f& o) const {
            return { x + o.x, y + o.y, z + o.z };
        }
        Vec3f Vec3f::operator-(const Vec3f& o) const {
            return { x - o.x, y - o.y, z - o.z };
        }

        Vec3f Vec3f::operator+(real val) const {
            return { x + val, y + val, z + val };
        }
        Vec3f Vec3f::operator-(real val) const {
            return { x - val, y - val, z - val };
        }

        real Vec3f::operator*(const Vec3f& o) const {
            return x * o.x + y * o.y + z * o.z;
        }

        Vec3f Vec3f::operator-() const {
            return { -x, -y, -z };
        }

        Vec3f Vec3f::operator*(real o) const {
            return { x * o, y * o, z * o };
        }
        Vec3f Vec3f::operator/(real o) const {
            return { x / o, y / o, z / o };
        }

        void Vec3f::operator=(const Vec3f& o) {
            x = o.x;
            y = o.y;
            z = o.z;
        }

        void Vec3f::operator+=(const Vec3f& o) {
            x += o.x;
            y += o.y;
            z += o.z;
        }
        void Vec3f::operator-=(const Vec3f& o) {
            x -= o.x;
            y -= o.y;
            z -= o.z;
        }
        void Vec3f::operator+=(real o) {
            x += o;
            y += o;
            z += o;
        }
        void Vec3f::operator-=(real o) {
            x -= o;
            y -= o;
            z -= o;
        }

        void Vec3f::operator*=(real o) {
            x *= o;
            y *= o;
            z *= o;
        }

        void Vec3f::operator/=(real o) {
            x /= o;
            y /= o;
            z /= o;
        }

        Vec3f Vec3f::floor() const {
            return { std::floor(x), std::floor(y), std::floor(z) };
        }
        Vec3f Vec3f::ceil() const {
            return { std::ceil(x), std::ceil(y), std::ceil(z) };
        }
        Vec3f Vec3f::round() const {
            return { std::round(x), std::round(y), std::round(z) };
        }

        Rect::Rect() : x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 } {}
        Rect::Rect(i32 width, i32 height) : x{ 0 }, y{ 0 }, width{ width }, height{ height } {}
        Rect::Rect(i32 x, i32 y, i32 width, i32 height) : x{ x }, y{ y }, width{ width }, height{ height } {}
        Rect::~Rect() {}

        bool Rect::contains(i32 xx, i32 yy) const {
            return (x <= xx && xx <= x + width) &&
                (y <= yy && yy <= y + height);
        }

        bool Rect::contains(const Vec3f& o) const {
            return contains((int)o.x, (int)o.y);
        }

        bool Rect::contains(const Rect& r) const {
            return (x <= r.x && r.x + r.width <= x + width) &&
                (y <= r.y && r.y + r.height <= y + height);
        }

        bool Rect::overlaps(const Rect& r) const {
            return (x < r.x + r.width && x + width > r.x) &&
                (y < r.y + r.height && y + height > r.y);
        }

    }
}

