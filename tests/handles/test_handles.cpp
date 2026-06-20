#include "core/handles.h"

#include <cstddef>
#include <limits>

#include <gtest/gtest.h>

/* TODO: HandlePool tests are missing too! */

/* Constructor test */

TEST(HandlesTest, DefaultConstructor) {
    Handle handle{};
    EXPECT_EQ(handle.index(), Handle::INDEX_INVALID);
    EXPECT_EQ(handle.gen(), 0);
}

TEST(HandlesTest, ConstructFromIndex) {
    for (size_t i = 0; i < 10; ++i) {
        Handle handle = Handle::from_index(i);
        EXPECT_EQ(handle.index(), i);
        EXPECT_EQ(handle.gen(), 0);
    }
}

TEST(HandlesTest, ConstructFromIndexAndGeneration) {
    for (size_t i = 0; i < 10; ++i) {
        Handle handle = Handle::from_index_gen(0, i);
        EXPECT_EQ(handle.index(), 0);
        EXPECT_EQ(handle.gen(), i);
    }
}

/* Handle validation */

TEST(HandlesTest, EnsureDefaultHandleIsInvalid) {
    Handle invalid{};
    EXPECT_FALSE(invalid);
    EXPECT_EQ(static_cast<bool>(invalid), invalid.is_valid());
}

TEST(HandlesTest, EnsureHandleFromIndexIsValid) {
    for (size_t i = 0; i < 10; ++i) {
        Handle handle = Handle::from_index(0);
        EXPECT_TRUE(handle);
    }
}

TEST(HandlesTest, EnsureHandleFromReservedIndexIsInvalid) {
    Handle handle = Handle::from_index(std::numeric_limits<size_t>::max());
    EXPECT_FALSE(handle);

    handle = Handle::from_index(Handle::INDEX_MAX);
    EXPECT_EQ(handle.index(), Handle::INDEX_MAX);
    EXPECT_TRUE(handle); /* index is in bounds */

    handle = Handle::from_index(Handle::INDEX_MAX + 1);
    EXPECT_EQ(handle.index(), Handle::INDEX_INVALID);
    EXPECT_FALSE(handle); /* index is outside bounds */
}

/* Increment operator */

TEST(HandlesTest, IncrementGeneration) {
    for (size_t i = 0; i < 10; ++i) {
        Handle handle = Handle::from_index(i);
        for (size_t j = 0; j < 10; ++j) {
            ASSERT_EQ(handle.index(), i);
            ASSERT_EQ(handle.gen(), j);
            handle = handle.inc_gen();
        }
    }
}

TEST(HandlesTest, IncrementGenerationWrapOnOverflow) {
    Handle handle = Handle::from_index_gen(0, Handle::GENERATION_MAX);
    handle = handle.inc_gen();
    EXPECT_EQ(handle.index(), 0);
    EXPECT_EQ(handle.gen(), 0);
}

/* Decrement operator */

TEST(HandlesTest, DecrementGeneration) {
    for (size_t i = 0; i < 10; ++i) {
        Handle handle = Handle::from_index_gen(i, 10);
        for (size_t j = 10; j-- > 0;) {
            handle = handle.dec_gen();
            ASSERT_EQ(handle.index(), i);
            ASSERT_EQ(handle.gen(), j);
        }
    }
}

TEST(HandlesTest, DecrementGenerationWrapOnUnderflow) {
    Handle handle = Handle::from_index_gen(0, 0);
    handle = handle.dec_gen();
    EXPECT_EQ(handle.gen(), Handle::GENERATION_MAX);
}
