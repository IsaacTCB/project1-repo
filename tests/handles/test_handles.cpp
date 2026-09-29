#include "core/utils/handles.h"

#include <cstddef>
#include <limits>

#include <gtest/gtest.h>

/* TODO: HandlePool tests are missing too! */

struct TestTag;
using TestHandle = Handle<TestTag>;

/* Constructor test */

TEST(HandlesTest, DefaultConstructor) {
    TestHandle handle{};
    EXPECT_EQ(handle.index(), TestHandle::INDEX_INVALID);
    EXPECT_EQ(handle.gen(), 0);
}

TEST(HandlesTest, ConstructFromIndex) {
    for (size_t i = 0; i < 10; ++i) {
        TestHandle handle = TestHandle::from_index(i);
        EXPECT_EQ(handle.index(), i);
        EXPECT_EQ(handle.gen(), 0);
    }
}

TEST(HandlesTest, ConstructFromIndexAndGeneration) {
    for (size_t i = 0; i < 10; ++i) {
        TestHandle handle = TestHandle::from_index_gen(0, i);
        EXPECT_EQ(handle.index(), 0);
        EXPECT_EQ(handle.gen(), i);
    }
}

/* Explicit handle casting */

struct OtherTag;
using OtherHandle = Handle<OtherTag>;

static_assert(std::is_convertible_v<TestHandle, ErasedHandle>);
static_assert(!std::is_convertible_v<ErasedHandle, TestHandle>);
static_assert(!std::is_convertible_v<TestHandle, OtherHandle>);
static_assert(std::equality_comparable_with<TestHandle, ErasedHandle>);
static_assert(!std::equality_comparable_with<TestHandle, OtherTag>);

TEST(HandleCast, EnsureHandleCastReproducesSameValues) {
    auto t = TestHandle::from_index_gen(5, 3);
    auto s = handle_cast<OtherHandle>(t);
    EXPECT_EQ(s.index(), 5);
    EXPECT_EQ(s.gen(), 3);
    EXPECT_EQ(handle_cast<TestHandle>(s), t);
}

/* Handle validation */

TEST(HandlesTest, EnsureDefaultHandleIsInvalid) {
    TestHandle invalid{};
    EXPECT_FALSE(invalid);
    EXPECT_EQ(static_cast<bool>(invalid), invalid.valid());
}

TEST(HandlesTest, EnsureHandleFromIndexIsValid) {
    for (size_t i = 0; i < 10; ++i) {
        TestHandle handle = TestHandle::from_index(0);
        EXPECT_TRUE(handle);
    }
}

TEST(HandlesTest, EnsureHandleFromReservedIndexIsInvalid) {
    TestHandle handle = TestHandle::from_index(std::numeric_limits<size_t>::max());
    EXPECT_FALSE(handle);

    handle = TestHandle::from_index(TestHandle::INDEX_MAX);
    EXPECT_EQ(handle.index(), TestHandle::INDEX_MAX);
    EXPECT_TRUE(handle); /* index is in bounds */

    handle = TestHandle::from_index(TestHandle::INDEX_MAX + 1);
    EXPECT_EQ(handle.index(), TestHandle::INDEX_INVALID);
    EXPECT_FALSE(handle); /* index is outside bounds */
}

/* Increment operator */

TEST(HandlesTest, IncrementGeneration) {
    for (size_t i = 0; i < 10; ++i) {
        TestHandle handle = TestHandle::from_index(i);
        for (size_t j = 0; j < 10; ++j) {
            ASSERT_EQ(handle.index(), i);
            ASSERT_EQ(handle.gen(), j);
            handle = handle.inc_gen();
        }
    }
}

TEST(HandlesTest, IncrementGenerationWrapOnOverflow) {
    TestHandle handle = TestHandle::from_index_gen(0, TestHandle::GENERATION_MAX);
    handle = handle.inc_gen();
    EXPECT_EQ(handle.index(), 0);
    EXPECT_EQ(handle.gen(), 0);
}

/* Decrement operator */

TEST(HandlesTest, DecrementGeneration) {
    for (size_t i = 0; i < 10; ++i) {
        TestHandle handle = TestHandle::from_index_gen(i, 10);
        for (size_t j = 10; j-- > 0;) {
            handle = handle.dec_gen();
            ASSERT_EQ(handle.index(), i);
            ASSERT_EQ(handle.gen(), j);
        }
    }
}

TEST(HandlesTest, DecrementGenerationWrapOnUnderflow) {
    TestHandle handle = TestHandle::from_index_gen(0, 0);
    handle = handle.dec_gen();
    EXPECT_EQ(handle.gen(), TestHandle::GENERATION_MAX);
}
