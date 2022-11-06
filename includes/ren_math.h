#pragma once

// TODO: Look into how to vectorize everything, or even just replace ren_math.h with an actual maths library
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

	T &operator[](int index) {
		assert(index >= 0 && index < 2);
		return d[index];
	}

	const T &operator[](int index) const {
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

	T &operator[](int index) {
		assert(index >= 0 && index < 3);
		return d[index];
	}

	const T &operator[](int index) const {
		assert(index >= 0 && index < 3);
		return d[index];
	}
};

typedef V3T<float> V3;
typedef V3T<int> V3i;

template<typename T> V2T<T> operator+(V2T<T> a, V2T<T> b) { return { a.x + b.x, a.y + b.y }; }
template<typename T> V2T<T> operator-(V2T<T> a, V2T<T> b) { return { a.x - b.x, a.y - b.y }; }
template<typename T> V2T<T> operator-(V2T<T> a) { return { -a.x, -a.y }; }
template<typename T> V2T<T> operator*(V2T<T> a, V2T<T> b) { return { a.x * b.x, a.y * b.y }; }
template<typename T> V2T<T> operator*(V2T<T> a, float b) { return { a.x * b, a.y * b }; }
template<typename T> V2T<T> operator*(float b, V2T<T> a) { return a * b; }
template<typename T> V2T<T> operator/(V2T<T> a, V2T<T> b) { return { a.x / b.x, a.y / b.y }; }
template<typename T> V2T<T> operator/(V2T<T> a, float b) { return { a.x / b, a.y / b }; }
template<typename T> V2T<T> &operator+=(V2T<T> &a, V2 b) { return a = a + b; }
template<typename T> V2T<T> &operator-=(V2T<T> &a, V2 b) { return a = a + -b; }
template<typename T> V2T<T> &operator*=(V2T<T> &a, float b) { return a = a * b; }
template<typename T> inline float dot(V2T<T> a, V2T<T> b) { return a.x * b.x + a.y * b.y; }
template<typename T> inline float length_squared(V2T<T> a) { return dot(a, a); }
template<typename T> inline float length(V2T<T> a) { return sqrtf(dot(a, a)); }

template<typename T> V3T<T> operator+(V3T<T> a, V3T<T> b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
template<typename T> V3T<T> operator-(V3T<T> a, V3T<T> b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
template<typename T> V3T<T> operator-(V3T<T> a) { return		{ -a.x, -a.y, -a.z }; }
template<typename T> V3T<T> operator*(V3T<T> a, V3T<T> b) { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
template<typename T> V3T<T> operator*(V3T<T> a, float b) { return { a.x * b, a.y * b,  a.z * b }; }
template<typename T> V3T<T> operator*(float b, V3T<T> a) { return a * b; }
template<typename T> V3T<T> operator/(V3T<T> a, V3T<T> b) { return { a.x / b.x, a.y / b.y, a.z / b.z }; }
template<typename T> V3T<T> operator/(V3T<T> a, float b) { return { a.x / b, a.y / b, a.z / b }; }
template<typename T> V3T<T> &operator+=(V3T<T> &a, V3T<T> b) { return a = a + b; }
template<typename T> V3T<T> &operator-=(V3T<T> &a, V3T<T> b) { return a = a + -b; }
template<typename T> V3T<T> &operator*=(V3T<T> &a, float b) { return a = a * b; }
template<typename T> inline float dot(V3T<T> a, V3T<T> b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
template<typename T> inline float length_squared(V3T<T> a) { return dot(a, a); }
template<typename T> inline float length(V3T<T> a) { return sqrtf(dot(a, a)); }
template<typename T> V3T<T> cross(V3T<T> a, V3T<T> b) { return { a.y * b.z - a.z * b.y, b.x * a.z - a.x * b.z, a.x * b.y - a.y * b.x }; }
template<typename T>
V3T<T> triple_prod(V3T<T> a, V3T<T> b, V3T<T> c) { return cross(cross(a, b), c); } // (A x B) x C

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


struct Rect {
	V2 min;
	V2 max;
};

struct Circle {
	V2 pos;
	float radius;
};

struct Capsule {
	V2 a, b;
	float radius;
};

constexpr int MAX_POINTS = 10;

struct Polygon {
	V2 points[MAX_POINTS];
	V2 pos;
	int size;
};

V2 center(Rect a) {
	return (a.min + a.max) / 2.f;
}

V2 center(Circle a) {
	return a.pos;
}

V2 center(Polygon a) {
	return a.pos;
}

V2 center(Capsule a) {
	return (a.a + a.b) / 2;
}

V2 support(Rect a, V2 dir) {
	return { dir.x > 0 ? a.max.x : a.min.x, dir.y > 0 ? a.max.y : a.min.y };
}

V2 support(Circle a, V2 dir) {
	return a.pos + a.radius * normalizez(dir);
}

V2 support(Polygon a, V2 dir) {
	int index = 0;
	float max_dot = dot(a.points[0], dir);
	for (int i = 1; i < a.size; ++i) {
		float dot_val = dot(a.points[i], dir);
		if (dot_val > max_dot) {
			max_dot = dot_val;
			index = i;
		}
	}
	return a.pos + a.points[index];
}

V2 support(Capsule a, V2 dir) {
	dir = normalizez(dir);
	float da = dot(a.a, dir);
	float db = dot(a.b, dir);
	if (da > db) return a.a + dir * a.radius;
	else return a.b + dir * a.radius;
}

template<typename ShapeA, typename ShapeB>
V2 support(ShapeA a, ShapeB b, V2 dir) {
	return support(a, dir) - support(b, -dir);
}

bool handle_simplex(V2 *simplex, int &simplex_size, V2 &dir) {
	if (simplex_size == 2) {
		V2 b = simplex[0], a = simplex[1];
		V2 ab = b - a, ao = -a;
		V2 ab_perp = triple_prod(V3(ab), V3(ao), V3(ab)).xy;
		dir = ab_perp;
		return false;
	} else {
		V2 c = simplex[0], b = simplex[1], a = simplex[2];
		V2 ab = b - a, ac = c - a, ao = -a;
		V2 ab_perp = triple_prod(V3(ab), V3(ao), V3(ab)).xy;
		V2 ac_perp = triple_prod(V3(ac), V3(ao), V3(ac)).xy;
		if (dot(ab_perp, dir) > 0) {
			simplex[0] = simplex[1];
			simplex[1] = simplex[2];
			simplex_size--;
			dir = ab_perp;
			return false;
		} else if (dot(ac_perp, dir) > 0) {
			simplex[1] = simplex[2];
			simplex_size--;
			dir = ac_perp;
			return false;
		}
	}
	return true;
}

template<typename ShapeA, typename ShapeB>
bool gjk(ShapeA s1, ShapeB s2, V2 *points = 0, int* size = 0)
{
	V2 dir = normalizez(center(s2) - center(s1));
	V2 simplex[3] = { support(s1, s2, dir) };
	int simplex_size = 1;
	dir = -simplex[0];
	while (true) {
		V2 a = support(s1, s2, dir);
		if (dot(a, dir) < 0)
			return false;
		simplex[simplex_size++] = a;
		if (handle_simplex(simplex, simplex_size, dir)) {
			if (points && size) {
				points[0] = simplex[0];
				points[1] = simplex[1];
				points[2] = simplex[2];
				*size = 3;
			}
			return true;
		}
	}
}

// TODO: Make proper array handling functions
constexpr int EPA_MAX_POINTS = 64;

template<typename T>
void insert(T *arr, int &size, int index, T val)
{
	assert(size + 1 <= EPA_MAX_POINTS);
	for (int i = size - 1; i >= index; i--) {
		arr[i + 1] = arr[i];
	}
	size++;
	arr[index] = val;
}

// TODO: make the insertion dynamic
template<typename ShapeA, typename ShapeB>
bool epa(ShapeA s1, ShapeB s2, V2 &dist) {
	V2 points[EPA_MAX_POINTS];
	int point_count = 0;
	if (!gjk(s1, s2, points, &point_count))
		return false;

	int min_index = 0;
	float min_distance = INFINITY;
	V2 min_normal = V2();

	while (min_distance == INFINITY) {
		for (int i = 0; i < point_count; ++i) {
			int j = (i + 1) % point_count;

			V2 v_i = points[i];
			V2 v_j = points[j];

			V2 ij = v_j - v_i;

			V2 normal = normalizez(V2(ij.y, -ij.x));
			float distance = dot(normal, v_i);

			if (distance < 0) {
				distance *= -1;
				normal *= -1;
			}

			if (distance < min_distance) {
				min_distance = distance;
				min_normal = normal;
				min_index = j;
			}
		}

		V2 sp = support(s1, s2, min_normal);
		float s_distance = dot(min_normal, sp);

		if (fabs(s_distance - min_distance) > 0.001f) {
			min_distance = INFINITY;
			insert(points, point_count, min_index, sp);
		}
	}
	dist = min_normal * (min_distance + 0.001f);
	return true;
}