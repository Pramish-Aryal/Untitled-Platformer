template <typename T> struct ExitScope {
	T lambda;
	ExitScope(T lambda) : lambda(lambda) {}
	~ExitScope() { lambda(); }
};

struct ExitScopeHelper {
	template <typename T> ExitScope<T> operator+(T t) { return t; }
};

#define CONCAT_INT(a, b) a##b
#define CONCAT(a, b) CONCAT_INT(a, b)
#define DEFER auto &CONCAT(defer__, __LINE__) = ExitScopeHelper() + [&]()
