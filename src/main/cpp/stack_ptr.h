#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace skizzay::cddd {
namespace stack_ptr_details_ {
template <typename T> struct move_construct final {
  static void call(T *const destination, T *const source) noexcept(
      std::is_nothrow_move_constructible_v<T>) requires
      std::move_constructible<T> {
    new (destination) T(std::move(*source));
  }

  static void call(T *const destination, T *const source) noexcept(
      std::is_nothrow_default_constructible_v<T>
          &&std::is_nothrow_move_assignable_v<T>) requires
      std::default_initializable<T> && std::is_move_assignable_v<T> &&
      (!std::move_constructible<T>) {
    new (destination) T();
    *destination = std::move(*source);
  }
};

template <typename T> struct move_assign final {
  static void
  call(T *const destination,
       T *const source) noexcept(std::is_nothrow_move_assignable_v<T>) requires
      std::is_move_assignable_v<T> {
    *destination = std::move(*source);
  }
};

template <typename T> struct destruct final {
  static void call(T *const destination) noexcept requires
      std::is_trivially_destructible_v<T> {}

  static void call(T *const destination) noexcept(
      std::is_nothrow_destructible_v<
          T>) requires(!std::is_trivially_destructible_v<T>) {
    std::destroy_at(destination);
  }
};

template <typename T, template <typename> typename Functor>
struct call_operations final {
  static void call_binary(void *const destination, void *const source) noexcept(
      noexcept(Functor<T>::call(static_cast<T *const>(destination),
                                static_cast<T *const>(source)))) {
    Functor<T>::call(static_cast<T *const>(destination),
                     static_cast<T *const>(source));
  }

  static void call_unary(void *const destination) noexcept(
      noexcept(Functor<T>::call(static_cast<T *const>(destination)))) {
    Functor<T>::call(static_cast<T *const>(destination));
  }
};

struct ptr_operations final {
  void (*move_construct_func)(void *const, void *const);
  void (*move_assign_func)(void *const, void *const);
  void (*destruct_func)(void *const) noexcept;
};

template <typename T>
inline constexpr ptr_operations operations = {
    .move_construct_func = &call_operations<T, move_construct>::call_binary,
    .move_assign_func = &call_operations<T, move_assign>::call_binary,
    .destruct_func = &call_operations<T, destruct>::call_unary};

template <std::size_t Size, std::size_t Align> struct impl_storage {
  impl_storage() noexcept = default;
  impl_storage(std::nullptr_t) noexcept = default;
  template <std::size_t Size2, std::size_t Align2>
  requires(Size2 <= Size) &&
      (0 == Align % Align2)
          impl_storage(impl_storage<Size2, Align2> &&other) noexcept
      : ops_{other.ops_} {
    copy_data(other.storage_);
    other.ops_ = nullptr;
  }
  impl_storage(ptr_operations const *ops, void *const data) noexcept
      : ops_{ops} {
    (ops_->move_construct_func)(data);
  }

  ~impl_storage() { reset(); }

  constexpr void reset() noexcept {
    if (ops_) {
      destruct();
      ops_ = nullptr;
    }
  }

  explicit operator bool() const noexcept { return nullptr != ops_; }

  constexpr auto operator<=>(std::nullptr_t const) noexcept {
    return std::compare_three_way{}(
        ops_, static_cast<ptr_operations const *>(nullptr));
  }

  constexpr auto operator<=>(impl_storage const &) noexcept = default;

protected:
  void destruct() noexcept { (ops_->destruct_func)(storage_.data()); }

  void *data() noexcept { return nullptr == ops_ ? nullptr : storage_.data(); }

  void const *data() const noexcept {
    return nullptr == ops_ ? nullptr : storage_.data();
  }

  template <std::size_t N>
  requires(N <= Size) void copy_data(
      std::array<std::byte, N> const &other) noexcept {
    std::copy(std::begin(other), std::end(other), std::begin(storage_));
  }

  ptr_operations *ops_ = nullptr;
  alignas(Align) std::array<std::byte, Size> storage_;
};

template <std::destructible T, std::size_t Size, std::size_t Align>
requires(!std::is_void_v<T>) && (0 != Size) &&
    (0 != Align) struct impl final : private impl_storage<Size, Align> {
  using impl_storage<Size, Align>::impl_storage;
  using impl_storage<Size, Align>::operator bool;
  using impl_storage<Size, Align>::operator<=>;
  using impl_storage<Size, Align>::reset;

  impl(T &&t) noexcept(std::is_nothrow_move_constructible_v<T>)
      : impl_storage{&operations<T>, &t} {}

  template <typename U, std::size_t Size2, std::size_t Align2>
  impl(impl<U, Size2, Align2> &&i)

      template <typename U, std::size_t Size2, std::size_t Align2>
  requires std::is_base_of_v<T, U> &&(Size2 <= Size) &&
      (0 == (Align % Align2)) impl &
      operator=(impl &&rhs) noexcept(std::is_nothrow_destructible_v<T>) {
    if (this != &rhs) {
      destruct();
      copy_data(std::begin(rhs.storage_), std::end(rhs.storage_));
      std::swap(ops_, rhs.ops_);
    }
  }

  T *operator->() noexcept { return get(); }

  T const *operator->() noexcept { return get(); }

  T &operator*() noexcept { return *get(); }

  T const &operator*() const noexcept { return *get(); }

  template <typename U>
  requires std::is_base_of_v<T, U>
  constexpr void swap(impl<U, Size, Align> &other) noexcept {
    using std::swap;
    swap(storage_, other.storage_);
    swap(ops_, other.ops_);
  }

  T *get() noexcept { return static_cast<T *>(this->data()); }

  T const *get() const noexcept { return static_cast<T const *>(this->data()); }
};
} // namespace stack_ptr_details_

template <typename T, std::size_t Size = sizeof(T),
          std::size_t Align = alignof(std::max_align_t)>
using stack_ptr = stack_ptr_details_::impl<T, Size, Align>;

} // namespace skizzay::cddd
