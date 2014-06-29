#include "cddd/cqrs/object_id.h"
#include <gtest/gtest.h>

namespace {

using namespace cddd::cqrs;


TEST(given_an_objectid_type, when_default_constructed_then_it_should_be_null)
{
	// Given
	object_id target;

	// When & Then
	ASSERT_TRUE(target.is_null());
}


TEST(given_an_objectid_type, when_value_constructed_then_it_should_not_be_null)
{
	// Given
	object_id target = object_id::create(std::string("string"));

	// When & Then
	ASSERT_FALSE(target.is_null());
}


TEST(given_an_objectid_type, when_default_constructed_then_the_hash_should_be_zero)
{
	// Given
	object_id target;
   std::size_t expected = 0;

	// When
	std::size_t hashValue = target.hash();

	// ASSERT
	ASSERT_EQ(expected, hashValue);
}


TEST(given_an_objectid_type, when_default_constructed_then_the_string_should_be_empty)
{
	// Given
	object_id target;

	// When
	std::string stringValue = target.to_string();

	// Then
	ASSERT_TRUE(stringValue.empty());
}


TEST(given_an_objectid_type, when_value_constructed_then_the_string_should_represent_internal_value)
{
	// Given
	object_id target = object_id::create(1);

	// When
	std::string stringValue = target.to_string();

	// Then
	ASSERT_EQ("1", stringValue);
}


TEST(given_an_objectid_type, when_value_constructed_then_the_hash_should_should_not_be_zero)
{
	// Given
	object_id target = object_id::create(std::size_t(1));
   std::size_t expected = 1;

	// When
	std::size_t hashValue = target.hash();

	// ASSERT
	ASSERT_EQ(expected, hashValue);
}


TEST(given_an_objectid_type, when_move_constructed_then_the_previous_value_should_be_null)
{
	// Given
	object_id target = object_id::create(std::size_t(1));

	// When
	object_id newobject_id(std::move(target));

	// Then
	ASSERT_TRUE(target.is_null());
}


TEST(given_an_objectid_type, when_move_assigned_then_the_previous_value_should_be_null)
{
	// Given
	object_id target = object_id::create(std::size_t(1));
	object_id newobject_id;

	// When
	newobject_id = std::move(target);

	// Then
	ASSERT_TRUE(target.is_null());
}


TEST(given_an_objectid_type, when_copy_constructed_then_the_new_value_should_equal_the_existing_value)
{
	// Given
	object_id expected = object_id::create(std::size_t(1));

	// When
	object_id target(expected);

	// Then
	ASSERT_EQ(expected, target);
}


TEST(given_an_objectid_type, when_copy_assigned_then_the_new_value_should_equal_the_existing_value)
{
	// Given
	object_id expected = object_id::create(std::size_t(1));
	object_id target;

	// When
	target = expected;

	// Then
	ASSERT_EQ(expected, target);
}

}
