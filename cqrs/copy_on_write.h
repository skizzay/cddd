#ifndef CDDD_CQRS_COPY_ON_WRITE_H__
#define CDDD_CQRS_COPY_ON_WRITE_H__

#include <memory>

namespace cddd {
namespace cqrs {

template<class T>
class copy_on_write {
public:
	explicit copy_on_write(T *t) :
		ptr(t)
	{
	}
   explicit copy_on_write(std::unique_ptr<T> t) :
      ptr(std::move(t))
   {
   }
	copy_on_write() = default;
	copy_on_write(const copy_on_write &) = default;
	copy_on_write(copy_on_write &&) = default;
	template<class U>
	explicit copy_on_write(U *u) :
		ptr(u)
	{
	}
	template<class U, class D>
	explicit copy_on_write(std::unique_ptr<U, D> u) :
		ptr(u)
	{
	}
	~copy_on_write() = default;

	copy_on_write & operator =(const copy_on_write &) = default;
	copy_on_write & operator =(copy_on_write &&) = default;

	std::size_t use_count() const
	{
		return ptr.use_count();
	}
	T& operator*()
	{
		return *get();
	}
	const T& operator*() const
	{
		return *get();
	}
	T* operator->()
	{
		return get();
	}
	const T* operator->() const
	{
		return get();
	}

	explicit operator bool() const
	{
		return static_cast<bool>(ptr);
	}

	void swap(copy_on_write &other)
	{
		ptr.swap(other.ptr);
	}
	void reset(T *pointer=nullptr)
	{
		ptr.reset(pointer);
	}

	T* get()
	{
		if (ptr && !ptr.unique()) {
			ptr = std::make_shared<T>(*ptr);
		}
		return ptr.get();
	}

	const T* get() const
	{
		return read();
	}

	const T* read() const
	{
		return ptr.get();
	}

private:
	std::shared_ptr<T> ptr;
};

}
}

namespace std
{

template<class T>
void swap(cddd::cqrs::copy_on_write<T> &l, cddd::cqrs::copy_on_write<T> &r)
{
	l.swap(r);
}

}

#endif
