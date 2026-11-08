#include <gtest/gtest.h>

extern "C" {
#include "bits.h"
}

TEST(Crc16, KnownCheckValue)
{
    const uint8_t s[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(crc16(s, sizeof s), 0x29B1);
}

TEST(Crc16, EmptyIsInitValue)
{
    EXPECT_EQ(crc16(nullptr, 0), 0xFFFF);
}

TEST(Crc16, IncrementalMatchesBulk)
{
    const uint8_t d[] = {1, 2, 3, 4, 5};
    uint16_t c = 0xFFFF;
    for (uint8_t x : d) {
        c = crc16_update(c, x);
    }
    EXPECT_EQ(c, crc16(d, sizeof d));
}

TEST(Crc32, KnownCheckValue)
{
    const uint8_t s[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(crc32_compute(s, sizeof s), 0xCBF43926u);
}

TEST(Crc32, EmptyIsZero)
{
    EXPECT_EQ(crc32_compute(nullptr, 0), 0u);
}
