#ifndef MSL_DETAIL_SHARED_PTR_H
#define MSL_DETAIL_SHARED_PTR_H

#include <cstddef>
#include <msl/typeinfo>
#include <msl/compressed_pair.h>
#include <msl/detail/default_delete.h>

namespace std {
namespace tr1 {
namespace detail {

class shared_ptr_deleter_common {
public:
	// rb3 leaves the counters out; retail stores 1 into both after the new (0x800C8958)
	shared_ptr_deleter_common()
	    : m_RefCount(1)
	    , m_WeakCount(1)
	{
	}
	virtual ~shared_ptr_deleter_common() { }
	virtual const void* get_deleter(const std::type_info& info) const = 0;

protected:
	virtual void dispose() = 0;

public:
	void release();

	int m_RefCount;
	int m_WeakCount;
};

template <typename T, typename Deleter = std::detail::default_delete<T> >
class shared_ptr_deleter : public shared_ptr_deleter_common {
public:
	shared_ptr_deleter(T* ptr)
	    : m_Ptr(ptr)
	{
	}
	~shared_ptr_deleter() { }

	const void* get_deleter(const std::type_info& info) const
	{
		if (info == typeid(Deleter)) {
			return &m_Ptr.second();
		}
		return NULL;
	}

protected:
	void dispose() { m_Ptr.second()(m_Ptr.first()); }

private:
	Metrowerks::compressed_pair<T*, Deleter> m_Ptr;
};

template <typename T>
class shared_ptr_base {
protected:
	void __set(T* ptr, tr1::detail::shared_ptr_deleter_common* deleter)
	{
		m_Ptr     = ptr;
		m_Deleter = deleter;
	}

public:
	~shared_ptr_base() throw()
	{
		if (m_Deleter) {
			m_Deleter->release();
		}
	}

	T* get() const { return m_Ptr; }

protected:
	T* m_Ptr;
	tr1::detail::shared_ptr_deleter_common* m_Deleter;
};

} // namespace detail
} // namespace tr1

namespace detail {

template <typename T>
class shared_ptr : public tr1::detail::shared_ptr_base<T> {
public:
	shared_ptr();
	explicit shared_ptr(T* ptr) { this->__set(ptr, new tr1::detail::shared_ptr_deleter<T>(ptr)); }
};

template <typename T>
class shared_ptr<T[]> : public tr1::detail::shared_ptr_base<T> {
	struct __array_guard {
		T* ptr_;
		std::detail::default_delete<T[]>& del_;
		__array_guard(T* p, std::detail::default_delete<T[]>& d)
		    : ptr_(p)
		    , del_(d)
		{
		}
		~__array_guard()
		{
			if (ptr_) {
				del_(ptr_);
			}
		}
		void release() { ptr_ = NULL; }
	};

public:
	shared_ptr();
	explicit shared_ptr(T ptr[])
	{
		std::detail::default_delete<T[]> d;
		__array_guard hold(ptr, d);
		this->__set(ptr, new tr1::detail::shared_ptr_deleter<T, std::detail::default_delete<T[]> >(ptr));
		hold.release();
	}
};

} // namespace detail
} // namespace std

#endif
