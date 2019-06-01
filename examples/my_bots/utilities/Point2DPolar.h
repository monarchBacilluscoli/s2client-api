#ifndef POINT2DPOLAR_H
#define POINT2DPOLAR_H

#include<cmath>
#include<sc2api/sc2_common.h>

namespace sc2 {
	struct Point2DInPolar {
		Point2D point_rec;
		float r;
		float theta;

		Point2DInPolar() :r(0.f), theta(0.f), point_rec(Point2D()) {};
		Point2DInPolar(float in_r, float in_theta) :point_rec(Point2D(in_r*std::cos(in_theta), in_r*std::sin(in_theta))),r(in_r), theta(in_theta) { };
		Point2DInPolar(const Point2D& p) :r(std::sqrt(p.x*p.x + p.y*p.y)), theta(std::atan2(p.y, p.x)), point_rec(p) {};

		Point2DInPolar& operator+=(const Point2DInPolar& rhs);
		Point2DInPolar& operator-=(const Point2DInPolar& rhs);
		Point2DInPolar& operator*=(float rhs);
		Point2DInPolar& operator/=(float rhs);

		bool operator==(const Point2DInPolar& rhs);
		bool operator!=(const Point2DInPolar& rhs);

		Point2D toPoint2D();
	};

	Point2DInPolar operator+(const Point2DInPolar& lhs, const Point2DInPolar& rhs);
	Point2DInPolar operator-(const Point2DInPolar& lhs, const Point2DInPolar& rhs);
	Point2DInPolar operator*(const Point2DInPolar& lhs, float rhs);
	Point2DInPolar operator*(float lhs, const Point2DInPolar& rhs);
	Point2DInPolar operator/(const Point2DInPolar& lhs, float rhs);
	Point2DInPolar operator/(float lhs, const Point2DInPolar& rhs);
}
#endif // !POINT2DPOLAR_H