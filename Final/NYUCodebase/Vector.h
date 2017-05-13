#pragma once

class Vector {
public:
	Vector() :x(0), y(0) {}
	Vector(float x, float y) :x(x), y(y) {}
	float length() const { return sqrt(x*x + y*y); }
	void normalize() {
		float len = length();
		if (len != 0) {
			x /= len;
			y /= len;
		}
	}
	float x;
	float y;
	float z;
};