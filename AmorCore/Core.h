#pragma once
#include "Common.h"

namespace amor {
	namespace math {
		class Vec3f {
		public:
			Vec3f();
			Vec3f(const Vec3f& o);
			Vec3f(real u);
			Vec3f(real x, real y, real z = 0.0f);
			~Vec3f();

			bool operator==(const Vec3f& o) const;
			bool operator!=(const Vec3f& o) const;

			real len() const;
			real len_squared() const;
			Vec3f normalized() const;
			real angle2() const;

			Vec3f operator+(const Vec3f& o) const;
			Vec3f operator-(const Vec3f& o) const;

			Vec3f operator+(real val) const;
			Vec3f operator-(real val) const;

			real operator*(const Vec3f& o) const;

			Vec3f operator-() const;

			Vec3f operator*(real o) const;
			Vec3f operator/(real o) const;

			void operator=(const Vec3f& o);
			void operator+=(const Vec3f& o);
			void operator+=(real o);
			void operator-=(const Vec3f& o);
			void operator-=(real o);

			void operator*=(real o);
			void operator/=(real o);

		public:
			// extended functions
			Vec3f floor() const;
			Vec3f ceil() const;
			Vec3f round() const;

		public:
			real x, y, z;
		};

		class Rect {
		public:
			Rect();
			Rect(i32 width, i32 height);
			Rect(i32 x, i32 y, i32 width, i32 height);
			~Rect();

			bool contains(i32 x, i32 y) const;
			bool contains(const Vec3f& v) const;
			bool contains(const Rect& r) const;
			bool overlaps(const Rect& r) const;

			inline Vec3f origin() const { return { (real)x, (real)y, 0.0 }; }
			inline Vec3f size() const { return { (real)width, (real)height, 0.0 }; }

			inline i32 x2() const { return x + width; }
			inline i32 y2() const { return y + height; }

		public:
			i32 x, y, width, height;
		};
	}
}