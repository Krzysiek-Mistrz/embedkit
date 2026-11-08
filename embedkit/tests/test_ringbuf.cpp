#include <gtest/gtest.h>
#include <atomic>
#include <thread>

extern "C" {
    #include "ringbuf.h"
}


namespace {
struct Fixture : ::testing::Test {
    uint8_t storage[8];
    ringbuf_t rb;
    void SetUp() override { ASSERT_TRUE(ringbuf_init(&rb, storage, sizeof storage)); }
};
} // namespace

TEST(RingbufInit, RejectsBadArguments)
{
    uint8_t s[8];
    ringbuf_t rb;
    EXPECT_FALSE(ringbuf_init(&rb, nullptr, 8));
    EXPECT_FALSE(ringbuf_init(&rb, s, 0));
    EXPECT_FALSE(ringbuf_init(&rb, s, 6));
    EXPECT_TRUE(ringbuf_init(&rb, s, 1));
    EXPECT_TRUE(ringbuf_init(&rb, s, 8));
}

TEST_F(Fixture, StartsEmpty)
{
    uint8_t b;
    EXPECT_TRUE(ringbuf_is_empty(&rb));
    EXPECT_FALSE(ringbuf_is_full(&rb));
    EXPECT_EQ(ringbuf_count(&rb), 0U);
    EXPECT_EQ(ringbuf_capacity(&rb), 8U);
    EXPECT_FALSE(ringbuf_pop(&rb, &b));
}

TEST_F(Fixture, FifoOrder)
{
    for (uint8_t i = 1; i <= 5; i++) {
        ASSERT_TRUE(ringbuf_push(&rb, i));
    }

    EXPECT_EQ(ringbuf_count(&rb), 5u);
    
    for (uint8_t i = 1; i <= 5; i++) {
        uint8_t b = 0;
        ASSERT_TRUE(ringbuf_pop(&rb, &b));
        EXPECT_EQ(b, i);
    }
    EXPECT_TRUE(ringbuf_is_empty(&rb));
}

TEST_F(Fixture, UsesFullCapacityAndRejectsWhenFull)
{
    for (uint8_t i = 0; i < 8; i++) {
        ASSERT_TRUE(ringbuf_push(&rb, i)) << i;
    }
    
    EXPECT_TRUE(ringbuf_is_full(&rb));
    EXPECT_FALSE(ringbuf_push(&rb, 99));
    EXPECT_EQ(ringbuf_count(&rb), 8u);

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t b = 0;
        ASSERT_TRUE(ringbuf_pop(&rb, &b));
        EXPECT_EQ(b, i);
    }
}

TEST_F(Fixture, WrapsAroundManyTimes)
{
    uint8_t expected = 0;
    uint8_t next = 0;
    
    for (int round = 0; round < 100; round++) {
        for (int i = 0; i < 5; i++) {
            ASSERT_TRUE(ringbuf_push(&rb, next++));
        }
    
        for (int i = 0; i < 5; i++) {
            uint8_t b;
            ASSERT_TRUE(ringbuf_pop(&rb, &b));
            ASSERT_EQ(b, expected++);
        }
    }
}

TEST_F(Fixture, WorksAcrossUint32IndexOverflow)
{
    rb.head = rb.tail = 0xFFFFFFFCu;
    for (uint8_t i = 0; i < 8; i++) {
        ASSERT_TRUE(ringbuf_push(&rb, i));
    }
    
    EXPECT_TRUE(ringbuf_is_full(&rb));
    EXPECT_EQ(ringbuf_count(&rb), 8U);
    
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t b;
        ASSERT_TRUE(ringbuf_pop(&rb, &b));
        EXPECT_EQ(b, i);
    }
    
    EXPECT_TRUE(ringbuf_is_empty(&rb));
}

TEST(RingbufCap1, SingleSlot)
{
    uint8_t s[1];
    ringbuf_t rb;
    ASSERT_TRUE(ringbuf_init(&rb, s, 1));
    uint8_t b;
    EXPECT_TRUE(ringbuf_push(&rb, 7));
    EXPECT_FALSE(ringbuf_push(&rb, 8));
    EXPECT_TRUE(ringbuf_pop(&rb, &b));
    EXPECT_EQ(b, 7);
    EXPECT_FALSE(ringbuf_pop(&rb, &b));
}

TEST(RingbufSpsc, TwoThreadsKeepOrderAndLoseNothing)
{
    constexpr uint32_t kTotal = 500000;
    uint8_t storage[64];
    ringbuf_t rb;
    ASSERT_TRUE(ringbuf_init(&rb, storage, sizeof storage));

    std::thread producer([&] {
        for (uint32_t i = 0; i < kTotal; i++) {
            while (!ringbuf_push(&rb, static_cast<uint8_t>(i))) {
                std::this_thread::yield();
            }
        }
    });

    uint32_t received = 0;
    bool in_order = true;

    while (received < kTotal) {
        uint8_t b;
        
        if (ringbuf_pop(&rb, &b)) {
            if (b != static_cast<uint8_t>(received)) {
                in_order = false;
            }

            received++;
        } else {
            std::this_thread::yield();
        }
    }
    
    producer.join();

    EXPECT_TRUE(in_order);
    EXPECT_EQ(received, kTotal);
    EXPECT_TRUE(ringbuf_is_empty(&rb));
}
