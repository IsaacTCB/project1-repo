#include "runtime/audio/spsc_queue.h"

#include <cstddef>
#include <gtest/gtest.h>

/* Constructor tests */

TEST(SpscQueueConstructorTest, DefaultConstructor) {
    SpscQueue<int, 16> queue;
    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(queue.empty(), true);

    int temp = 103;
    EXPECT_EQ(queue.try_pop(temp), false);
    EXPECT_EQ(temp, 103);
}

TEST(SpscQueueFifoTest, FifoCheck) {
    constexpr size_t QUEUE_SIZE = 12;
    constexpr size_t CHUNK_SIZE = 8;
    static_assert(CHUNK_SIZE < QUEUE_SIZE);

    SpscQueue<int, QUEUE_SIZE + 1> queue;

    int base = 0;
    for (size_t repeat = 0; repeat < 3; ++repeat) {
        /* push all elements into queue */
        for (size_t i = 0; i < 8; ++i) {
            bool result = queue.try_emplace(base + i);
            ASSERT_EQ(result, true) << "Failed to emplace element " << base + i;
        }

        /* read all elements from queue */
        int item;
        for (size_t i = 0; i < 8; ++i) {
            bool result = queue.try_pop(item);
            ASSERT_EQ(result, true) << "Failed to retrieve element " << base + i;
            ASSERT_EQ(item, base + i) << "FIFO check failure, expected " << base + i << " got " << item;
        }

        base += CHUNK_SIZE;
    }
}

TEST(SpscQueueFifoTest, SizeTest) {
    constexpr size_t QUEUE_SIZE = 16;
    SpscQueue<int, QUEUE_SIZE + 1> queue;

    EXPECT_EQ(0, queue.size());

    /* push all elements into queue */
    for (size_t i = 0; i < QUEUE_SIZE / 2; ++i) {
        queue.try_emplace(i);
        ASSERT_EQ(i + 1, queue.size());
    }

    /* clear spsc */
    int yes = 0;
    while (queue.try_pop(yes)) {
    }
    ASSERT_EQ(0, queue.size());

    /* fill out queue again */
    for (size_t i = 0; i < QUEUE_SIZE; ++i) {
        queue.try_emplace(i);
        ASSERT_EQ(i + 1, queue.size());
    }
}

TEST(SpscQueueFifoTest, EmplaceFillTest) {
    constexpr size_t QUEUE_SIZE = 16;
    SpscQueue<int, QUEUE_SIZE + 1> queue;

    EXPECT_EQ(0, queue.size());

    /* apply a tiny offset for the queue */
    for (size_t j = 0; j < 4; ++j) {
        bool success = queue.try_push(static_cast<int>(j));
        ASSERT_EQ(success, true);
    }

    int popped;
    while (queue.try_pop(popped)) {
    }

    /* fill thingy */
    for (size_t i = 0; i < QUEUE_SIZE; ++i) {
        ASSERT_EQ(queue.try_emplace(i), true);
    }

    /* okay, we should be full now. */
    ASSERT_EQ(queue.try_emplace(0), false);

    /* now that that's done, we can pop a value and it should work */
    ASSERT_EQ(queue.try_pop(popped), true);
    ASSERT_EQ(queue.try_emplace(0), true);
}

TEST(SpscQueueFifoTest, PushMoveTest) {
    constexpr size_t QUEUE_SIZE = 16;
    SpscQueue<std::shared_ptr<int>, QUEUE_SIZE + 1> queue;

    auto ptr = std::make_shared<int>(5);
    queue.try_push(ptr);
    EXPECT_EQ(ptr.use_count(), 2);

    auto ptr2 = ptr;
    queue.try_push(std::move(ptr));
    EXPECT_EQ(ptr2.use_count(), 3);
}
