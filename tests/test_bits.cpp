#include <gtest/gtest.h>

extern "C" {
#include "bits.h"
}


TEST(Bits, SetClearToggleTest)
{
    EXPECT_EQ(bits_set(0u, 0), 0x1u);
    EXPECT_EQ(bits_set(0u, 31), 0x80000000u);
    EXPECT_EQ(bits_clear(0xFFFFFFFFu, 31), 0x7FFFFFFFu);
    EXPECT_EQ(bits_clear(0xFFFFFFFFu, 0), 0xFFFFFFFEu);
    EXPECT_EQ(bits_toggle(0u, 5), 0x20u);
    EXPECT_EQ(bits_toggle(0x20u, 5), 0u);
    EXPECT_TRUE(bits_test(0x80u, 7));
    EXPECT_FALSE(bits_test(0x80u, 6));
    EXPECT_TRUE(bits_test(0x80000000u, 31));
}

TEST(Bits, OutOfRangeBitIsHarmless)
{
    EXPECT_EQ(bits_set(0u, 32), 0u);
    EXPECT_EQ(bits_clear(0xFFu, 32), 0xFFu);
    EXPECT_EQ(bits_toggle(0xFFu, 100), 0xFFu);
    EXPECT_FALSE(bits_test(0xFFFFFFFFu, 32));
}

TEST(Bits, ExtractField)
{
    EXPECT_EQ(bits_extract(0xABCD1234u, 0, 4), 0x4u);
    EXPECT_EQ(bits_extract(0xABCD1234u, 4, 8), 0x23u);
    EXPECT_EQ(bits_extract(0xABCD1234u, 28, 4), 0xAu);
}

TEST(Bits, ExtractFullWidthDoesNotInvokeUB)
{
    EXPECT_EQ(bits_extract(0xDEADBEEFu, 0, 32), 0xDEADBEEFu);
}

TEST(Bits, ExtractInvalidArgsReturnZero)
{
    EXPECT_EQ(bits_extract(0xFFFFFFFFu, 0, 0), 0u);
    EXPECT_EQ(bits_extract(0xFFFFFFFFu, 1, 32), 0u);
    EXPECT_EQ(bits_extract(0xFFFFFFFFu, 32, 1), 0u);
    EXPECT_EQ(bits_extract(0xFFFFFFFFu, 30, 3), 0u);
}

TEST(Bits, InsertField)
{
    EXPECT_EQ(bits_insert(0u, 4, 4, 0xFu), 0xF0u);
    EXPECT_EQ(bits_insert(0xFFFFFFFFu, 8, 8, 0u), 0xFFFF00FFu);
    EXPECT_EQ(bits_insert(0u, 0, 4, 0xFFu), 0xFu);
    EXPECT_EQ(bits_insert(0xA5A5A5A5u, 8, 8, 0x3Cu), 0xA5A53CA5u);
    EXPECT_EQ(bits_insert(0x12345678u, 0, 32, 0xCAFEBABEu), 0xCAFEBABEu);
    EXPECT_EQ(bits_insert(0x1u, 31, 1, 1u), 0x80000001u);
}

TEST(Bits, InsertInvalidArgsLeaveValue)
{
    EXPECT_EQ(bits_insert(0x55u, 0, 0, 0xFFu), 0x55u);
    EXPECT_EQ(bits_insert(0x55u, 31, 2, 0xFFu), 0x55u);
}

TEST(Bits, InsertThenExtractRoundTrip)
{
    for (unsigned pos = 0; pos < 32; pos++) {
        for (unsigned w = 1; pos + w <= 32; w++) {
            const uint32_t v = bits_insert(0u, pos, w, 0xFFFFFFFFu);
            const uint32_t expect = (w == 32) ? 0xFFFFFFFFu : ((1u << w) - 1u);
            ASSERT_EQ(bits_extract(v, pos, w), expect) << "pos=" << pos << " w=" << w;
        }
    }
}

TEST(Bits, PackUnpackEndianness)
{
    uint8_t b[4];
    bits_pack_u32_le(b, 0x11223344u);
    EXPECT_EQ(b[0], 0x44);
    EXPECT_EQ(b[3], 0x11);
    EXPECT_EQ(bits_unpack_u32_le(b), 0x11223344u);

    bits_pack_u32_be(b, 0x11223344u);
    EXPECT_EQ(b[0], 0x11);
    EXPECT_EQ(b[3], 0x44);
    EXPECT_EQ(bits_unpack_u32_be(b), 0x11223344u);
}

TEST(Bits, PackUnpackBoundaryValues)
{
    uint8_t b[4];
    for (uint32_t v : {0u, 1u, 0x80000000u, 0xFFFFFFFFu}) {
        bits_pack_u32_le(b, v);
        EXPECT_EQ(bits_unpack_u32_le(b), v);
        bits_pack_u32_be(b, v);
        EXPECT_EQ(bits_unpack_u32_be(b), v);
    }
}

TEST(Bits, Popcount)
{
    EXPECT_EQ(bits_popcount32(0u), 0u);
    EXPECT_EQ(bits_popcount32(1u), 1u);
    EXPECT_EQ(bits_popcount32(0xFFFFFFFFu), 32u);
    EXPECT_EQ(bits_popcount32(0xA5A5A5A5u), 16u);
}

TEST(Bits, Reverse)
{
    EXPECT_EQ(bits_reverse32(0u), 0u);
    EXPECT_EQ(bits_reverse32(1u), 0x80000000u);
    EXPECT_EQ(bits_reverse32(0x80000000u), 1u);
    EXPECT_EQ(bits_reverse32(0x12345678u), 0x1E6A2C48u);
    EXPECT_EQ(bits_reverse32(bits_reverse32(0xDEADBEEFu)), 0xDEADBEEFu);
}

TEST(Crc8, KnownCheckValue)
{
    const uint8_t s[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(crc8(s, sizeof s), 0xF4);
}

TEST(Crc8, EmptyIsZero)
{
    EXPECT_EQ(crc8(nullptr, 0), 0);
}

TEST(Crc8, IncrementalMatchesBulk)
{
    const uint8_t d[] = {1, 2, 3, 4, 5};
    uint8_t c = 0;
    for (uint8_t x : d) {
        c = crc8_update(c, x);
    }
    EXPECT_EQ(c, crc8(d, sizeof d));
}
