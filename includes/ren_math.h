#pragma once

#include <math.h>
#include <assert.h>

struct V2 {
	float x;
	float y;

	V2() : x(), y() {}
	V2(float a) : x(a), y(a) {}
	V2(float _x, float _y) : x(_x), y(_y) {}
};

V2 operator+(V2 a, V2 b) {
	return { a.x + b.x, a.y + b.y };
}

V2 operator-(V2 a, V2 b) {
	return { a.x - b.x, a.y - b.y };
}

V2 operator-(V2 a) {
	return V2(0) - a;
}

V2 operator*(V2 a, V2 b) {
	return { a.x * b.x, a.y * b.y };
}

V2 operator*(V2 a, float b) {
	return { a.x * b, a.y * b };
}

V2 operator*(float b, V2 a) {
	return a * b;
}

V2 operator/(V2 a, float b) {
	return { a.x / b, a.y / b };
}

V2 &operator+=(V2 &a, V2 b) {
	a = a + b;
	return a;
}

V2 &operator-=(V2 &a, V2 b) {
	a = a + -b;
	return a;
}

V2 &operator*=(V2 &a, float b) {
	a = a * b;
	return a;
}

inline float dot(V2 a, V2 b) {
	return a.x * b.x + a.y * b.y;
}

inline float length_squared(V2 a) {
	return dot(a, a);
}

inline float length(V2 a) {
	return sqrtf(dot(a, a));
}

inline V2 normalize(V2 a) {
	float len = length(a);
	assert(len != 0);
	return a / len;
}

inline V2 normalizez(V2 a) {
	float len = length(a);
	if (len) {
		return a / len;
	}
	return {};
}
