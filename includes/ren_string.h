#pragma once

#include <assert.h>

struct String {
	ptrdiff_t len;
	uint8_t *data;

	String() : data(0), len(0) {}
	template <ptrdiff_t _len> constexpr String(const char (&a)[_len]) : data((uint8_t *) a), len(_len - 1) {}
	String(const uint8_t *_Data, ptrdiff_t _Length) : data((uint8_t *) _Data), len(_Length) {}
	String(const char *_Data, ptrdiff_t _Length) : data((uint8_t *) _Data), len(_Length) {}
	const uint8_t &operator[](const ptrdiff_t index) const {
		assert(index < len);
		return data[index];
	}
	uint8_t &operator[](const ptrdiff_t index) {
		assert(index < len);
		return data[index];
	}
	inline uint8_t *begin() { return data; }
	inline uint8_t *end() { return data + len; }
	inline const uint8_t *begin() const { return data; }
	inline const uint8_t *end() const { return data + len; }
};

bool operator==(String a, String b)
{
	if (a.len != b.len)
		return false;
	for (int i = 0; i < a.len; ++i) {
		if (a[i] != b[i])
			return false;
	}
	return true;
}