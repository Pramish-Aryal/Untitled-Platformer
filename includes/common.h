#define ArrayCount(a) (sizeof(a) / sizeof(*(a)))

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float r32;
typedef double r64;
typedef ptrdiff_t imem;

#ifndef CONCAT_INT
#define CONCAT_INT(a, b) a##b
#endif
#ifndef CONCAT
#define CONCAT(a, b) CONCAT_INT(a, b)
#endif

template <typename F>
struct DeferHelper {
	F f;
	DeferHelper(F f) : f(f) {}
	~DeferHelper() { f(); }
};

template <typename F>
DeferHelper<F> defer_func(F f) {
	return DeferHelper<F>(f);
}

#define DEFER_INT(x)    CONCAT(x, __COUNTER__)
#define Defer(code)   auto DEFER_INT(_defer_) = defer_func([&](){code;})


//Maybe templatize them?
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Clamp(a, x, b) (Min(Max((a), (x)), (b))