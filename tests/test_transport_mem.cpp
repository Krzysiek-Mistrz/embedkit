#include <gtest/gtest.h>
#include <cstring>

extern "C" {
    #include "transport.h"
    #include "transport_mem.h"
    #include "frame_parser.h"
}


namespace {
struct TransportMemTest : ::testing::Test {
    transport_mem_t mem;
    transport_t transport;
    void SetUp() override { ASSERT_EQ(0, transport_mem_init(&mem, &transport)); }
};

struct Received {
    uint8_t data[64];
    uint8_t len = 0;
    int count = 0;
};

void collect(const uint8_t *payload, uint8_t len, void *user) {
    auto *received = static_cast<Received *>(user);
    std::memcpy(received->data, payload, len);
    received->len = len;
    received->count++;
}
} // namespace

TEST_F(TransportMemTest, ReadFromEmptyReturnsZero)
{
    uint8_t buf[8];
    EXPECT_EQ(0, transport.read(transport.ctx, buf, sizeof buf));
}

TEST_F(TransportMemTest, WrittenBytesAreReadBackInOrder)
{
    const uint8_t data[] = {1, 2, 3, 4, 5};
    ASSERT_EQ(0, transport_write_all(&transport, data, sizeof data));

    uint8_t buf[8] = {};
    ASSERT_EQ(5, transport.read(transport.ctx, buf, sizeof buf));
    EXPECT_EQ(0, std::memcmp(data, buf, sizeof data));
}

TEST_F(TransportMemTest, ReadRespectsRequestedLength)
{
    const uint8_t data[] = {10, 20, 30, 40};
    ASSERT_EQ(0, transport_write_all(&transport, data, sizeof data));

    uint8_t buf[8] = {};
    EXPECT_EQ(2, transport.read(transport.ctx, buf, 2));
    EXPECT_EQ(10, buf[0]);
    EXPECT_EQ(20, buf[1]);
    EXPECT_EQ(2, transport.read(transport.ctx, buf, sizeof buf));
    EXPECT_EQ(30, buf[0]);
    EXPECT_EQ(40, buf[1]);
}

TEST_F(TransportMemTest, WriteStopsWhenFull)
{
    uint8_t data[TRANSPORT_MEM_CAPACITY + 10];
    std::memset(data, 0xAB, sizeof data);

    EXPECT_EQ(static_cast<int>(TRANSPORT_MEM_CAPACITY), transport.write(transport.ctx, data, sizeof data));
    EXPECT_EQ(0, transport.write(transport.ctx, data, 1));
}

TEST_F(TransportMemTest, WriteAllFailsWhenDataDoesNotFit)
{
    uint8_t data[TRANSPORT_MEM_CAPACITY + 1] = {};
    EXPECT_EQ(-1, transport_write_all(&transport, data, sizeof data));
}

TEST_F(TransportMemTest, FrameSurvivesRoundTripThroughTransport)
{
    const uint8_t payload[] = {'t', 'e', 'm', 'p'};
    uint8_t frame[32];
    size_t frame_len = frame_encode(payload, sizeof payload, frame, sizeof frame);
    ASSERT_GT(frame_len, 0u);
    ASSERT_EQ(0, transport_write_all(&transport, frame, frame_len));

    uint8_t parser_buf[32];
    frame_parser_t parser;
    Received received;
    frame_parser_init(&parser, parser_buf, sizeof parser_buf, collect, &received);

    uint8_t rx[8];
    int count;
    while ((count = transport.read(transport.ctx, rx, sizeof rx)) > 0) {
        for (int i = 0; i < count; i++) {
            frame_parser_feed(&parser, rx[i]);
        }
    }

    EXPECT_EQ(1, received.count);
    ASSERT_EQ(sizeof payload, received.len);
    EXPECT_EQ(0, std::memcmp(payload, received.data, sizeof payload));
    EXPECT_EQ(0u, parser.crc_errors);
}

TEST_F(TransportMemTest, CorruptedByteIsDetectedAsCrcError)
{
    const uint8_t payload[] = {'o', 'k'};
    uint8_t frame[16];
    size_t frame_len = frame_encode(payload, sizeof payload, frame, sizeof frame);
    frame[2] ^= 0xFF;
    ASSERT_EQ(0, transport_write_all(&transport, frame, frame_len));

    uint8_t parser_buf[16];
    frame_parser_t parser;
    Received received;
    frame_parser_init(&parser, parser_buf, sizeof parser_buf, collect, &received);

    uint8_t rx[16];
    int count = transport.read(transport.ctx, rx, sizeof rx);
    for (int i = 0; i < count; i++) {
        frame_parser_feed(&parser, rx[i]);
    }

    EXPECT_EQ(0, received.count);
    EXPECT_EQ(1u, parser.crc_errors);
}
