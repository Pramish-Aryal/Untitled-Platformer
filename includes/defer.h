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