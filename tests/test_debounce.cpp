#include <gtest/gtest.h>
#include <vector>

extern "C" {
#include "debounce.h"
#include "fsm.h"
}


namespace {
std::vector<debounce_event_t> run(debounce_t &d, const std::vector<int> &samples)
{
    std::vector<debounce_event_t> out;
    for (int s : samples) {
        const auto e = debounce_update(&d, s != 0);

        if (e != DEBOUNCE_NONE) {
            out.emplace_back(e);
        }
    }
    return out;
}
} // namespace

TEST(Debounce, CleanPressAndRelease)
{
    debounce_t d;
    debounce_init(&d, 3);
    auto ev = run(d, {0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0});
    ASSERT_EQ(ev.size(), 2U);
    EXPECT_EQ(ev[0], DEBOUNCE_EVT_PRESSED);
    EXPECT_EQ(ev[1], DEBOUNCE_EVT_RELEASED);
    EXPECT_EQ(debounce_state(&d), DEBOUNCE_IDLE);
}

TEST(Debounce, PressedFiresExactlyOnThirdSample)
{
    debounce_t d;
    debounce_init(&d, 3);
    EXPECT_EQ(debounce_update(&d, true), DEBOUNCE_NONE);
    EXPECT_EQ(debounce_update(&d, true), DEBOUNCE_NONE);
    EXPECT_EQ(debounce_update(&d, true), DEBOUNCE_EVT_PRESSED);
    EXPECT_EQ(debounce_state(&d), DEBOUNCE_PRESSED);
    EXPECT_EQ(debounce_update(&d, true), DEBOUNCE_NONE);
}

TEST(Debounce, BounceOnPressIsIgnored)
{
    debounce_t d;
    debounce_init(&d, 3);
    auto ev = run(d, {1, 0, 1, 0, 1, 0, 0, 0});
    EXPECT_TRUE(ev.empty());
    EXPECT_EQ(debounce_state(&d), DEBOUNCE_IDLE);
}

TEST(Debounce, BounceThenStablePress)
{
    debounce_t d;
    debounce_init(&d, 3);
    auto ev = run(d, {1, 0, 1, 0, 1, 1, 1});
    ASSERT_EQ(ev.size(), 1U);
    EXPECT_EQ(ev[0], DEBOUNCE_EVT_PRESSED);
}

TEST(Debounce, GlitchWhilePressedDoesNotRelease)
{
    debounce_t d;
    debounce_init(&d, 3);
    auto ev = run(d, {1, 1, 1, 0, 1, 1, 1, 1});
    ASSERT_EQ(ev.size(), 1U);
    EXPECT_EQ(ev[0], DEBOUNCE_EVT_PRESSED);
    EXPECT_EQ(debounce_state(&d), DEBOUNCE_PRESSED);
}

TEST(Debounce, ThresholdOneReactsImmediately)
{
    debounce_t d;
    debounce_init(&d, 1);
    EXPECT_EQ(debounce_update(&d, true), DEBOUNCE_EVT_PRESSED);
    EXPECT_EQ(debounce_update(&d, false), DEBOUNCE_EVT_RELEASED);
}

TEST(Debounce, ZeroThresholdTreatedAsOne)
{
    debounce_t d;
    debounce_init(&d, 0);
    EXPECT_EQ(debounce_update(&d, true), DEBOUNCE_EVT_PRESSED);
}

TEST(Debounce, TwoPressesInRow)
{
    debounce_t d;
    debounce_init(&d, 2);
    auto ev = run(d, {1, 1, 0, 0, 1, 1, 0, 0});
    ASSERT_EQ(ev.size(), 4U);
    EXPECT_EQ(ev[0], DEBOUNCE_EVT_PRESSED);
    EXPECT_EQ(ev[1], DEBOUNCE_EVT_RELEASED);
    EXPECT_EQ(ev[2], DEBOUNCE_EVT_PRESSED);
    EXPECT_EQ(ev[3], DEBOUNCE_EVT_RELEASED);
}


namespace {
int g_calls = 0;
void count_call(void *ctx)
{
    (*static_cast<int *>(ctx))++;
    g_calls++;
}
} // namespace

TEST(Fsm, UnknownEventIsIgnored)
{
    static const fsm_transition_t table[] = {{0, 1, 1, nullptr}};
    fsm_t f;
    fsm_init(&f, table, 1, 0, nullptr);
    EXPECT_FALSE(fsm_dispatch(&f, 42));
    EXPECT_EQ(fsm_state(&f), 0);
    EXPECT_TRUE(fsm_dispatch(&f, 1));
    EXPECT_EQ(fsm_state(&f), 1);
    EXPECT_FALSE(fsm_dispatch(&f, 1));
}

TEST(Fsm, ActionRunsWithContextOnTransition)
{
    static const fsm_transition_t table[] = {{0, 1, 1, count_call}, {1, 1, 0, count_call}};
    int counter = 0;
    g_calls = 0;
    fsm_t f;
    fsm_init(&f, table, 2, 0, &counter);
    fsm_dispatch(&f, 1);
    fsm_dispatch(&f, 1);
    fsm_dispatch(&f, 7);
    EXPECT_EQ(counter, 2);
    EXPECT_EQ(g_calls, 2);
}
