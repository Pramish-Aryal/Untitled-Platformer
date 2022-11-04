#pragma once

#include <math.h>
#include <assert.h>

#define PI32 3.14159265359f
#define PI64 3.14159265359

template<typename T>
struct V2T {
	union {
		struct { T x, y; };
		T d[2];
	};

	V2T<T>() : x(), y() {}
	V2T<T>(T a) : x(a), y(a) {}
	V2T<T>(T _x, T _y) : x(_x), y(_y) {}

	T& operator[](int index) {
		assert(index >= 0 && index < 2);
		return d[index];
	}

	const T& operator[](int index) const {
		assert(index >= 0 && index < 2);
		return d[index];
	}
};

typedef V2T<float> V2;
typedef V2T<int> V2i;

template<typename T>
struct V3T {
	union {
		struct { T x, y, z; };
		struct {
			V2T<T> xy;
			T z;
		};
		struct {
			T x;
			V2T<T> yz;
		};
		T d[3];
	};

	V3T<T>() : x(), y(), z() {}
	V3T<T>(float a) : x(a), y(a), z(a) {}
	V3T<T>(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	V3T<T>(V2T<T> a) : x(a.x), y(a.y), z() {}
	V3T<T>(V2T<T> a, float b) : x(a.x), y(a.y), z(b) {}
	V3T<T>(float a, V2T<T> b) : x(a), y(b.x), z(b.y) {}

	T& operator[](int index) {
		assert(index >= 0 && index < 3);
		return d[index];
	}

	const T& operator[](int index) const {
		assert(index >= 0 && index < 3);
		return d[index];
	}
};

typedef V3T<float> V3;
typedef V3T<int> V3i;

template<typename T> V2T<T> operator+(V2T<T> a, V2T<T> b)		{ return { a.x + b.x, a.y + b.y }; }
template<typename T> V2T<T> operator-(V2T<T> a, V2T<T> b)		{ return { a.x - b.x, a.y - b.y }; }
template<typename T> V2T<T> operator-(V2T<T> a)					{ return { -a.x, -a.y }; }
template<typename T> V2T<T> operator*(V2T<T> a, V2T<T> b)		{ return { a.x * b.x, a.y * b.y }; }
template<typename T> V2T<T> operator*(V2T<T> a, float b)		{ return { a.x * b, a.y * b }; }
template<typename T> V2T<T> operator*(float b, V2T<T> a)		{ return a * b; }
template<typename T> V2T<T> operator/(V2T<T> a, V2T<T> b)		{ return { a.x / b.x, a.y / b.y }; }
template<typename T> V2T<T> operator/(V2T<T> a, float b)		{ return { a.x / b, a.y / b }; }
template<typename T> V2T<T> &operator+=(V2T<T> &a, V2 b)		{ return a = a + b; }
template<typename T> V2T<T> &operator-=(V2T<T> &a, V2 b)		{ return a = a + -b; }
template<typename T> V2T<T> &operator*=(V2T<T> &a, float b)		{ return a = a * b; }
template<typename T> inline float dot(V2T<T> a, V2T<T> b)		{ return a.x * b.x + a.y * b.y; }
template<typename T> inline float length_squared(V2T<T> a)		{ return dot(a, a); }
template<typename T> inline float length(V2T<T> a)				{ return sqrtf(dot(a, a)); }

template<typename T> V3T<T> operator+(V3T<T> a, V3T<T> b)		{ return { a.x + b.x, a.y + b.y, a.z + b.z }; }
template<typename T> V3T<T> operator-(V3T<T> a, V3T<T> b)		{ return { a.x - b.x, a.y - b.y, a.z - b.z }; }
template<typename T> V3T<T> operator-(V3T<T> a) { return		{ -a.x, -a.y, -a.z }; }
template<typename T> V3T<T> operator*(V3T<T> a, V3T<T> b)		{ return { a.x * b.x, a.y * b.y, a.z * b.z }; }
template<typename T> V3T<T> operator*(V3T<T> a, float b)		{ return { a.x * b, a.y * b,  a.z * b }; }
template<typename T> V3T<T> operator*(float b, V3T<T> a)		{ return a * b; }
template<typename T> V3T<T> operator/(V3T<T> a, V3T<T> b)		{ return { a.x / b.x, a.y / b.y, a.z / b.z }; }
template<typename T> V3T<T> operator/(V3T<T> a, float b)		{ return { a.x / b, a.y / b, a.z / b }; }
template<typename T> V3T<T> &operator+=(V3T<T> &a, V3T<T> b)	{ return a = a + b; }
template<typename T> V3T<T> &operator-=(V3T<T> &a, V3T<T> b)	{ return a = a + -b; }
template<typename T> V3T<T> &operator*=(V3T<T> &a, float b)		{ return a = a * b; }
template<typename T> inline float dot(V3T<T> a, V3T<T> b)		{ return a.x * b.x + a.y * b.y + a.z * b.z; }
template<typename T> inline float length_squared(V3T<T> a)		{ return dot(a, a); }
template<typename T> inline float length(V3T<T> a)				{ return sqrtf(dot(a, a)); }
template<typename T> V3T<T> cross(V3T<T> a, V3T<T> b)			{ return {a.y * b.z - a.z * b.y, b.x * a.z - a.x * b.z, a.x * b.y - a.y * b.x}; }
template<typename T> 
V3T<T> triple_prod(V3T<T> a, V3T<T> b, V3T<T> c)				{ return cross(cross(a, b), c); } // (A x B) x C

template<typename T>
inline V2T<T> normalize(V2T<T> a) {
	float len = length(a);
	assert(len != 0);
	return a / len;
}

template<typename T>
inline  V2T<T> normalizez(V2T<T> a) {
	float len = length(a);
	if (len) {
		return a / len;
	}
	return {};
}

template<typename T>
inline V3T<T> normalize(V3T<T> a) {
	float len = length(a);
	assert(len != 0);
	return a / len;
}

template<typename T>
inline V3T<T> normalizez(V3T<T> a) {
	float len = length(a);
	if (len) {
		return a / len;
	}
	return {};
}

template<typename T>
T lerp(T a, float t, T b) {
	return a + (b - a) * t;
}

template<typename T>
T ilerp(T a, T x, T b) {
	return (x - a) / (b - a);
}

template<typename T>
T map_range(T r1, T r2, T val, T r3, T r4) {
	return r3 + (r4 - r3) * (val - r1) / (r2 - r1);
}