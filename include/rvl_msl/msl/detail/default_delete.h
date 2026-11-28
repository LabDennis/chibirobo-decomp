#ifndef MSL_DETAIL_DEFAULT_DELETE_H
#define MSL_DETAIL_DEFAULT_DELETE_H

namespace std {
namespace detail {

template <typename T>
struct default_delete {
	void operator()(T* ptr) { delete ptr; }
};

template <typename T>
struct default_delete<T[]> {
	void operator()(T ptr[]) { delete[] ptr; }
};

} // namespace detail
} // namespace std

#endif
