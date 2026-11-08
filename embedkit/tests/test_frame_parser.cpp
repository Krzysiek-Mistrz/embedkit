#include <gtest/gtest.h>
#include <vector>

extern "C" {
#include "bits.h"
#include "frame_parser.h"
}


namespace {
using Bytes = std::vector<uint8_t>;

struct Collector {
    std::vector<Bytes> frames;

    static void cb(const uint8_t *p, uint8_t len, void *user)
    {
        static_cast<Collector *>(user)->frames.emplace_back(p, p + len);
    }
};

struct ParserTest : ::testing::Test {
    uint8_t buf[16];
    Collector col;
    frame_parser_t p;

    void SetUp() override { frame_parser_init(&p, buf, sizeof buf, Collector::cb, &col); }

    void feed(const Bytes &b)
    {
        for (uint8_t x : b) {
            frame_parser_feed(&p, x);
        }
    }

    static Bytes encode(const Bytes &payload)
    {
        Bytes out(payload.size() + 3);
        const size_t n = frame_encode(payload.data(), static_cast<uint8_t>(payload.size()),
                                      out.data(), out.size());
        EXPECT_EQ(n, out.size());
        
        return out;
    }
};
} // namespace

TEST_F(ParserTest, ValidFrame)
{
    feed(encode({1, 2, 3}));
    ASSERT_EQ(col.frames.size(), 1u);
    EXPECT_EQ(col.frames[0], (Bytes{1, 2, 3}));
    EXPECT_EQ(p.frames_ok, 1u);
}

TEST_F(ParserTest, EncodeLayout)
{
    const uint8_t payload[] = {0xAA, 0xBB};
    uint8_t out[5];
    ASSERT_EQ(frame_encode(payload, 2, out, sizeof out), 5u);
    EXPECT_EQ(out[0], 0x7E);
    EXPECT_EQ(out[1], 2);
    EXPECT_EQ(out[2], 0xAA);
    EXPECT_EQ(out[3], 0xBB);
    const uint8_t covered[] = {2, 0xAA, 0xBB};
    EXPECT_EQ(out[4], crc8(covered, sizeof covered));
    EXPECT_EQ(frame_encode(payload, 2, out, 4), 0u);
}

TEST_F(ParserTest, ChunkedDelivery)
{
    const Bytes f = encode({10, 20, 30, 40});

    for (uint8_t x : f) {
        EXPECT_TRUE(col.frames.empty() || x == f.back());
        frame_parser_feed(&p, x);
    }

    ASSERT_EQ(col.frames.size(), 1u);
    EXPECT_EQ(col.frames[0], (Bytes{10, 20, 30, 40}));
}

TEST_F(ParserTest, BadCrcRejected)
{
    Bytes f = encode({1, 2, 3});
    f.back() ^= 0x01;
    feed(f);
    EXPECT_TRUE(col.frames.empty());
    EXPECT_EQ(p.crc_errors, 1u);
}

TEST_F(ParserTest, RecoversAfterBadCrc)
{
    Bytes bad = encode({1, 2, 3});
    bad.back() ^= 0xFF;
    feed(bad);
    feed(encode({9, 9}));
    ASSERT_EQ(col.frames.size(), 1u);
    EXPECT_EQ(col.frames[0], (Bytes{9, 9}));
}

TEST_F(ParserTest, GarbageBeforeStartIsSkipped)
{
    feed({0x00, 0xFF, 0x12, 0x34});
    feed(encode({5, 6, 7}));
    ASSERT_EQ(col.frames.size(), 1u);
    EXPECT_EQ(col.frames[0], (Bytes{5, 6, 7}));
}

TEST_F(ParserTest, StartByteInsidePayloadIsJustData)
{
    feed(encode({0x7E, 0x7E, 0x01}));
    ASSERT_EQ(col.frames.size(), 1u);
    EXPECT_EQ(col.frames[0], (Bytes{0x7E, 0x7E, 0x01}));
}

TEST_F(ParserTest, LengthLargerThanBufferIsRejectedWithoutOverflow)
{
    struct Guarded {
        uint8_t buf[4];
        uint8_t canary[8];
    } g;
 
    std::fill(std::begin(g.canary), std::end(g.canary), 0xCC);
    Collector c;
    frame_parser_t q;
    frame_parser_init(&q, g.buf, sizeof g.buf, Collector::cb, &c);
    frame_parser_feed(&q, 0x7E);
    frame_parser_feed(&q, 200);
    
    for (int i = 0; i < 100; i++) {
        frame_parser_feed(&q, 0xAB);
    }

    EXPECT_TRUE(c.frames.empty());
    EXPECT_EQ(q.too_long_errors, 1u);
    
    for (uint8_t x : g.canary) {
        EXPECT_EQ(x, 0xCC);
    }
}

TEST_F(ParserTest, ExactlyBufferSizeIsAccepted)
{
    Bytes payload(sizeof buf, 0x5A);
    feed(encode(payload));
    ASSERT_EQ(col.frames.size(), 1u);
    EXPECT_EQ(col.frames[0], payload);
}

TEST_F(ParserTest, ZeroLengthPayload)
{
    feed(encode({}));
    ASSERT_EQ(col.frames.size(), 1u);
    EXPECT_TRUE(col.frames[0].empty());
}

TEST_F(ParserTest, TwoBackToBackFrames)
{
    Bytes both = encode({1, 2});
    const Bytes second = encode({3, 4, 5});
    both.insert(both.end(), second.begin(), second.end());
    feed(both);
    ASSERT_EQ(col.frames.size(), 2u);
    EXPECT_EQ(col.frames[0], (Bytes{1, 2}));
    EXPECT_EQ(col.frames[1], (Bytes{3, 4, 5}));
}

TEST_F(ParserTest, NullCallbackStillCountsFrames)
{
    frame_parser_init(&p, buf, sizeof buf, nullptr, nullptr);
    feed(encode({1}));
    EXPECT_EQ(p.frames_ok, 1u);
}
