#pragma once

#include <array>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>

namespace cddd {
namespace utils {

template<class T>
class array_view {
public:
   using value_type = T;
   using pointer = const T *;
   using const_pointer = const T *;
   using reference = const T &;
   using const_reference = const T &;
   using iterator = const T *;
   using const_iterator = const T *;
   using reverse_iterator = std::reverse_iterator<iterator>;
   using const_reverse_iterator = std::reverse_iterator<const_iterator>;
   using size_type = size_t;
   using difference_type = std::ptrdiff_t;

   constexpr array_view(const_pointer p, size_type l) noexcept :
      length{l},
      array{p}
   {
   }

   constexpr array_view() noexcept :
      array_view{nullptr, 0}
   {
   }

   constexpr array_view(const array_view &) noexcept = default;
   array_view &operator =(const array_view &) noexcept = default;

   constexpr array_view(std::initializer_list<value_type> l) noexcept :
      array_view{l.begin(), l.size()}
   {
   }

   template<class U, class Alloc, class=std::enable_if_t<std::is_convertible<U*, T*>::value>>
   constexpr array_view(const std::vector<U, Alloc> &v) noexcept :
      array_view{v.data(), v.size()}
   {
   }

   template<class U, size_t N, class=std::enable_if_t<std::is_convertible<U*, T*>::value>>
   constexpr array_view(const std::array<U, N> &a) noexcept :
      array_view{a.data(), a.size()}
   {
   }

   template<class U, size_t N, class=std::enable_if_t<std::is_convertible<U*, T*>::value>>
   constexpr array_view(const U a[N]) noexcept :
      array_view{&a[0], N}
   {
   }

   constexpr const_iterator begin() const noexcept {
      return this->array;
   }

   constexpr const_iterator end() const noexcept {
      return this->array + this->length;
   }

   constexpr const_iterator cbegin() const noexcept {
      return this->array;
   }

   constexpr const_iterator cend() const noexcept {
      return this->array + this->length;
   }

   inline const_reverse_iterator rbegin() const noexcept {
      return {this->end()};
   }

   inline const_reverse_iterator rend() const noexcept {
      return {this->begin()};
   }

   inline const_reverse_iterator crbegin() const noexcept {
      return {this->end()};
   }

   inline const_reverse_iterator crend() const noexcept {
      return {this->begin()};
   }

   constexpr size_type size() const noexcept {
      return this->length;
   }

   constexpr size_type max_size() const noexcept {
      return (std::numeric_limits<size_type>::max() - (sizeof(size_type) + sizeof(const T *))) / sizeof(const T) / 4;
   }

   constexpr bool empty() const noexcept {
      return this->length == 0;
   }

   constexpr const_reference operator[](size_type index) const noexcept {
      return *(this->array + index);
   }

   constexpr const_reference at(size_type index) const throw(std::out_of_range) {
      return index < this->length ?
             *(this->array + index) :
             throw std::out_of_range{"array_view::at"};
   }

   constexpr const_reference front() const noexcept {
      return *this->array;
   }

   constexpr const_reference back() const noexcept {
      return *(this->array + this->length - 1);
   }

   constexpr const_pointer data() const noexcept {
      return this->array;
   }

   inline void swap(array_view &other) noexcept {
      using std::swap;
      swap(this->length, other.length);
      swap(this->array, other.array);
   }

   template<typename Allocator_>
   explicit operator std::vector<T, Allocator_>() const {
      return std::vector<T, Allocator_>{begin(), end()};
   }

private:
   size_type length;
   const T *array;
};

}
}


namespace std {

template<typename T>
inline void swap(cddd::utils::array_view<T> &l, cddd::utils::array_view<T> &r) {
   l.swap(r);
}

}
