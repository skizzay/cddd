#include "cqrs/copy_on_write.h"
#include <gtest/gtest.h>

using namespace cddd::cqrs;

template<class T>
class copy_on_write_test : public ::testing::Test {
public:
	copy_on_write<T> target;
};


TYPED_TEST_CASE_P(copy_on_write_test);


inline void modify(std::vector<float> &v) { v.push_back(1.0f); }
inline void modify(int &i) { i = 1; }
inline void modify(std::string &s) { s = "hello"; }


TYPED_TEST_P(copy_on_write_test, bool_conversion_returns_false_when_pointer_is_empty) {
	// Given

	// When
   bool actual = this->target;
   
   // Then
   ASSERT_FALSE(actual);
}


TYPED_TEST_P(copy_on_write_test, bool_conversion_returns_true_when_pointer_is_not_empty) {
   // Given
   this->target.reset(new TypeParam);

   // When
   bool actual = this->target;
   
   // Then
   ASSERT_TRUE(actual);
}


TYPED_TEST_P(copy_on_write_test, get_returns_nullptr_when_pointer_is_empty) {
   // Given
   
   // When
   auto *actual = this->target.get();
   
   // Then
   ASSERT_EQ(nullptr, actual);
}


TYPED_TEST_P(copy_on_write_test, get_returns_stored_pointer_when_pointer_is_not_empty) {
   // Given
   TypeParam *expected = new TypeParam;
   this->target.reset(expected);
   
   // When
   auto *actual = this->target.get();
   
   // Then
   ASSERT_EQ(expected, actual);
}


TYPED_TEST_P(copy_on_write_test, use_count_returns_0_when_pointer_is_empty) {
   // Given
   
   // When
   int actual = this->target.use_count();
   
   // Then
   ASSERT_EQ(0, actual);
}


TYPED_TEST_P(copy_on_write_test, use_count_returns_1_when_pointer_is_used_once) {
   // Given
   this->target.reset(new TypeParam);
   
   // When
   int actual = this->target.use_count();
   
   // Then
	ASSERT_EQ(1, actual);
}


TYPED_TEST_P(copy_on_write_test, use_count_returns_2_after_copy_constructed)
{
	// Given
	this->target.reset(new TypeParam);
   copy_on_write<TypeParam> other(this->target);

	// When
   int actual = this->target.use_count();

	// Then
	ASSERT_EQ(2, actual);
}


TYPED_TEST_P(copy_on_write_test, use_count_returns_1_after_being_modified)
{
	// Arrange
	this->target.reset(new TypeParam);
   copy_on_write<TypeParam> other(this->target);
   modify(*this->target);

	// Act
   int actual = this->target.use_count();

	// Assert
	ASSERT_EQ(1, actual);
}


REGISTER_TYPED_TEST_CASE_P(copy_on_write_test,
		bool_conversion_returns_false_when_pointer_is_empty,
		bool_conversion_returns_true_when_pointer_is_not_empty,
      get_returns_nullptr_when_pointer_is_empty,
		get_returns_stored_pointer_when_pointer_is_not_empty,
      use_count_returns_0_when_pointer_is_empty,
      use_count_returns_1_when_pointer_is_used_once,
      use_count_returns_2_after_copy_constructed,
      use_count_returns_1_after_being_modified);


typedef ::testing::Types<std::vector<float>, int, std::string> CopyOnWriteTypesToTest;
INSTANTIATE_TYPED_TEST_CASE_P(CopyOnWriteTests, copy_on_write_test, CopyOnWriteTypesToTest);
