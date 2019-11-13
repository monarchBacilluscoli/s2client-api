/*! \file sc2_common.h
    \brief Common data types, including points, rectangles and colors.
*/
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sc2 {

//! 3D point.
//!< \sa Distance3D(const Point3D& a, const Point3D& b) DistanceSquared3D(const Point3D& a, const Point3D& b) Normalize3D(Point3D& a) Dot3D(const Point3D& a, const Point3D& b)
struct Point3D {
    float x;
    float y;
    float z;

    Point3D() :
        x(0.0f),
        y(0.0f),
        z(0.0f) {
    }

    Point3D(float in_x, float in_y, float in_z) :
        x(in_x),
        y(in_y),
        z(in_z) {
    }

    Point3D& operator+=(const Point3D& rhs);
    Point3D& operator-=(const Point3D& rhs);
    Point3D& operator*=(float rhs);
    Point3D& operator/=(float rhs);

    bool operator==(const Point3D& rhs);
    bool operator!=(const Point3D& rhs);
};

Point3D operator+(const Point3D& lhs, const Point3D& rhs);
Point3D operator-(const Point3D& lhs, const Point3D& rhs);
Point3D operator*(const Point3D& lhs, float rhs);
Point3D operator*(float lhs, const Point3D& rhs);
Point3D operator/(const Point3D& lhs, float rhs);
Point3D operator/(float lhs, const Point3D& rhs);

//! 2D point.
//!< \sa Distance2D(const Point2D& a, const Point2D& b) DistanceSquared2D(const Point2D& a, const Point2D& b) Normalize2D(Point2D& a) Dot2D(const Point2D& a, const Point2D& b)
struct Point2D {
    float x;
    float y;

    Point2D() :
        x(0.0f),
        y(0.0f) {
    }

    Point2D(Point3D a) :
        x(a.x),
        y(a.y) {

    }

    Point2D(float in_x, float in_y) :
        x(in_x),
        y(in_y) {
    }

    Point2D& operator+=(const Point2D& rhs);
    Point2D& operator-=(const Point2D& rhs);
    Point2D& operator*=(float rhs);
    Point2D& operator/=(float rhs);

    bool operator==(const Point2D& rhs);
    bool operator!=(const Point2D& rhs);

    float modulus(); 
};

Point2D operator+(const Point2D& lhs, const Point2D& rhs);
Point2D operator-(const Point2D& lhs, const Point2D& rhs);
Point2D operator*(const Point2D& lhs, float rhs);
Point2D operator*(float lhs, const Point2D& rhs);
Point2D operator/(const Point2D& lhs, float rhs);
Point2D operator/(float lhs, const Point2D& rhs);

typedef Point2D Vector2D;
typedef Point3D Vector3D;

//! 2D rectangle.
struct Rect2D {
    Point2D from;
    Point2D to;
};

//! 2D integer point.
struct Point2DI {
    int x;
    int y;

    Point2DI(int in_x = 0, int in_y = 0) :
        x(in_x),
        y(in_y) {
    }

    bool operator==(const Point2DI& rhs) const;
    bool operator!=(const Point2DI& rhs) const;
};

//! 2D integer rectangle.
struct Rect2DI {
    Point2DI from;
    Point2DI to;
};

//! 2D point represented in polar coordinator
struct Point2DP {
		Point2D point_rec;
		float r;
		float theta;

		Point2DP() :r(0.f), theta(0.f), point_rec(Point2D()) {};
		Point2DP(float in_r, float in_theta) :point_rec(Point2D(in_r*std::cos(in_theta), in_r*std::sin(in_theta))),r(in_r), theta(in_theta) { };
		Point2DP(const Point2D& p) :r(std::sqrt(p.x*p.x + p.y*p.y)), theta(std::atan2(p.y, p.x)), point_rec(p) {};

		Point2DP& operator+=(const Point2DP& rhs);
		Point2DP& operator-=(const Point2DP& rhs);
		Point2DP& operator*=(float rhs);
		Point2DP& operator/=(float rhs);

		bool operator==(const Point2DP& rhs);
		bool operator!=(const Point2DP& rhs);

		Point2D toPoint2D();
	};

    //todo undefined yet
	Point2DP operator+(const Point2DP& lhs, const Point2DP& rhs);
	Point2DP operator-(const Point2DP& lhs, const Point2DP& rhs);
	Point2DP operator*(const Point2DP& lhs, float rhs);
	Point2DP operator*(float lhs, const Point2DP& rhs);
	Point2DP operator/(const Point2DP& lhs, float rhs);
    Point2DP operator/(float lhs, const Point2DP& rhs);

//! RGB Color.
struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    Color() :
        r(255),
        g(255),
        b(255) {
    }

    Color(uint8_t in_r, uint8_t in_g, uint8_t in_b) :
        r(in_r),
        g(in_g),
        b(in_b) {
    }
};

namespace Colors {
    static const Color White = Color(255, 255, 255);
    static const Color Red = Color(255, 0, 0);
    static const Color Green = Color(0, 255, 0);
    static const Color Yellow = Color(255, 255, 0);
    static const Color Blue = Color(0, 0, 255);
    static const Color Teal = Color(0, 255, 255);
    static const Color Purple = Color(255, 0, 255);
    static const Color Black = Color(0, 0, 0);
    static const Color Gray = Color(128, 128, 128);
};

//! Gets a random floating-point number between -1.0 and 1.0.
//!< \return Random floating-point number between -1.0 and 1.0.
float GetRandomScalar();
//! Gets a random floating-point number between 0.0 and 1.0.
//!< \return Random floating-point number between 0.0 and 1.0.
float GetRandomFraction();
//! Gets a random integer between min and max inclusive.
//!< \param min Smallest value a random number could be.
//!< \param max Largest value a random number could be.
//!< \return Random integer between min and max.
int GetRandomInteger(int min, int max);

//! Gets a random entry from the container.
//!< \param container Array, list or whatever container.
//!< \return A random entry.
template <typename Container>
typename Container::value_type& GetRandomEntry(Container& container) {
    typename Container::iterator iter = container.begin();
    std::advance(iter, GetRandomInteger(0, static_cast<int>(container.size()) - 1));
    return *iter;
}

//! The distance between two 2D points.
//!< \param a First point.
//!< \param b Second point.
//!< \return Distance.
float Distance2D(const Point2D& a, const Point2D& b);
//! The distance squared between two 2D points, faster than Distance2D.
//!< \param a First point.
//!< \param b Second point.
//!< \return Distance^2.
float DistanceSquared2D(const Point2D& a, const Point2D& b);
//! Normalize a 2D point.
//!< \param a Point.
void Normalize2D(Point2D& a);
//! The dot product of two 2D vectors.
//!< \param a First vector.
//!< \param b Second vector.
//!< \return Dot product.
float Dot2D(const Point2D& a, const Point2D& b);

//! The distance between two 3D points.
//!< \param a First point.
//!< \param b Second point.
//!< \return Distance.
float Distance3D(const Point3D& a, const Point3D& b);
//! The distance squared between two 3D points, faster than Distance3D.
//!< \param a First point.
//!< \param b Second point.
//!< \return Distance^2.
float DistanceSquared3D(const Point3D& a, const Point3D& b);
//! Normalize a 3D point.
//!< \param a Point.
void Normalize3D(Point3D& a);
//! The dot product of two 3D vectors.
//!< \param a First vector.
//!< \param b Second vector.
//!< \return Dot product.
float Dot3D(const Point3D& a, const Point3D& b);



}
