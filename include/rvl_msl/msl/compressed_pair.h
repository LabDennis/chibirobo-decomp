#ifndef MSL_COMPRESSED_PAIR_H
#define MSL_COMPRESSED_PAIR_H

namespace std {
namespace detail {

template <typename T>
struct is_empty_helper : public T {
	int mValue;
};

struct is_empty_control {
	int mValue;
};

template <typename T, bool IsClass = (__builtin_ntype(T) & 0x2000) != 0>
struct is_empty {
	static const bool value = false;
};

template <typename T>
struct is_empty<T, true> {
	static const bool value = sizeof(is_empty_helper<T>) == sizeof(is_empty_control);
};

} // namespace detail
} // namespace std

namespace Metrowerks {

template <typename T>
struct call_traits {
	typedef const T& param_type;
};

template <>
struct call_traits<size_t> {
	typedef size_t param_type;
};

namespace detail {

template <typename T1, typename T2, bool T1Empty = std::detail::is_empty<T1>::value, bool T2Empty = std::detail::is_empty<T2>::value>
struct compressed_pair_selector {
	static const int value = 0;
};

template <typename T1, typename T2>
struct compressed_pair_selector<T1, T2, true, false> {
	static const int value = 1;
};

template <typename T1, typename T2>
struct compressed_pair_selector<T1, T2, false, true> {
	static const int value = 2;
};

template <typename T1, typename T2>
struct compressed_pair_selector<T1, T2, true, true> {
	static const int value = 3;
};

template <typename T1, typename T2, int Selector = compressed_pair_selector<T1, T2>::value>
class compressed_pair_imp {
public:
	typedef typename Metrowerks::call_traits<T1>::param_type first_param;
	typedef typename Metrowerks::call_traits<T2>::param_type second_param;

	compressed_pair_imp() { }
	compressed_pair_imp(first_param first, second_param second)
	    : m_First(first)
	    , m_Second(second)
	{
	}
	compressed_pair_imp(first_param first)
	    : m_First(first)
	{
	}
	compressed_pair_imp(second_param second)
	    : m_Second(second)
	{
	}

	T1& first() { return m_First; }
	const T1& first() const { return m_First; }
	T2& second() { return m_Second; }
	const T2& second() const { return m_Second; }

private:
	T1 m_First;
	T2 m_Second;
};

template <typename T1, typename T2>
class compressed_pair_imp<T1, T2, 2> : private T2 {
public:
	typedef typename Metrowerks::call_traits<T1>::param_type first_param;
	typedef typename Metrowerks::call_traits<T2>::param_type second_param;

	compressed_pair_imp() { }
	compressed_pair_imp(first_param first, second_param second)
	    : m_First(first)
	    , T2(second)
	{
	}
	compressed_pair_imp(first_param first)
	    : m_First(first)
	{
	}
	compressed_pair_imp(second_param second)
	    : T2(second)
	{
	}

	T1& first() { return m_First; }
	const T1& first() const { return m_First; }
	T2& second() { return *this; }
	const T2& second() const { return *this; }

private:
	T1 m_First;
};

} // namespace detail

template <typename T1, typename T2>
struct compressed_pair : public detail::compressed_pair_imp<T1, T2> {
	typedef detail::compressed_pair_imp<T1, T2> _Base;
	typedef typename _Base::first_param first_param;
	typedef typename _Base::second_param second_param;

	compressed_pair() { }
	compressed_pair(first_param first)
	    : _Base(first)
	{
	}
	compressed_pair(second_param second)
	    : _Base(second)
	{
	}
	compressed_pair(first_param first, second_param second)
	    : _Base(first, second)
	{
	}
};

} // namespace Metrowerks

#endif
