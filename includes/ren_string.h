#pragma once

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

// TODO: test all the string functions properly

// TODO: Call this string view and use it accordingly
struct String {
	imem len;
	u8 *data;

	String() : data(0), len(0) {}
	template <imem _len> constexpr String(const char (&a)[_len]) : data((u8 *) a), len(_len - 1) {}
	String(const u8 *_Data, imem _Length) : data((u8 *) _Data), len(_Length) {}
	String(const char *_Data, imem _Length) : data((u8 *) _Data), len(_Length) {}
	const u8 &operator[](const imem index) const {
		assert(index < len);
		return data[index];
	}
	u8 &operator[](const imem index) {
		assert(index < len);
		return data[index];
	}
	inline u8 *begin() { return data; }
	inline u8 *end() { return data + len; }
	inline const u8 *begin() const { return data; }
	inline const u8 *end() const { return data + len; }
};

// TODO: Add more string handling functions
bool operator==(String a, String b)
{
	if (a.len != b.len)
		return false;
	return SDL_memcmp(a.data, b.data, a.len) == 0;
}

String string_substring(String a, i32 start, u32 len)
{
	return { a.data + start, len };
}

// helpful utils

bool is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_digit(u8 c) {
	return c >= '0' && c <= '9';
}

bool is_alpha(u8 c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_alpha_numeric(u8 c) {
	return is_alpha(c) || is_digit(c);
}

bool is_valid_identifier_char(u8 c) {
	return is_alpha(c) || is_digit(c) || c == '_';
}

String string_trim_left(String string)
{
	while (string.len > 0 && is_space(*string.data)) {
		string.data++;
		string.len--;
	}
	return string;
}

String string_trim_right(String string)
{
	u32 i = 0;
	while (string.len > 0 && is_space(string.data[string.len - i - 1])) {
		i++;
	}
	string.len -= i;
	return string;
}

String string_trim(String string)
{
	return string_trim_right(string_trim_left(string));
}

String string_chop_left(String *string, u32 bytes)
{
	String result = *string;
	result.len = bytes;
	string->data += bytes;
	string->len -= bytes;
	return result;
}

String string_chop_left_while(String *string, bool (*predicate)(u8 a))
{
	u32 i = 0;
	for (; i < string->len && predicate(string->data[i]); ++i);
	return string_chop_left(string, i);
}

String string_chop_by_delim(String *string, u8 delim) {
	u32 i = 0;
	for (; i < string->len && string->data[i] != delim; ++i);

	String result = *string;
	result.len = i;
	
	string->len -= i + 1;
	string->data += i + 1;

	return result;
}

bool string_eq_sensitive(String a, String b)
{
	if (a.len != b.len) return false;
	return SDL_memcmp(a.data, b.data, a.len) == 0;
}

i32 string_parse_i32(String a)
{
	i32 result = SDL_strtol((const char *) a.data, 0, 10);
	if (errno == ERANGE) {
		log_error("Range Error Occured\n");
	}
	return result;
}

i64 string_parse_i64(String a)
{
	i64 result = strtoll((const char *) a.data, 0, 10);
	if (errno == ERANGE) {
		log_error("Range Error Occured\n");
	}
	return result;
}

r32 string_parse_r32(String a)
{
	r32 result = strtof((const char *) a.data, 0);
	if (errno == ERANGE) {
		log_error("Range Error Occured\n");
	}
	return result;
}

r64 string_parse_r64(String a)
{
	r64 result = strtod((const char *) a.data, 0);
	if (errno == ERANGE) {
		log_error("Range Error Occured\n");
	}
	return result;
}

