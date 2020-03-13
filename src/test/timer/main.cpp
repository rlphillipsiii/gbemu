#include <gtest/gtest.h>
#include <functional>
#include <optional>
#include <vector>
#include <utility>

#include "gameboy.h"
#include "memmap.h"

using std::reference_wrapper;
using std::optional;
using std::vector;
using std::pair;

class TimerTest : public ::testing::Test
{
protected:
    void SetUp() override;

    void testInit(TimerModule & timer);
    void testTimeoutSetting(TimerModule & timer);
    void testCounterTick(TimerModule & timer);
    // TODO: void testRtcTick(TimerModule & timer);
    void testTimerModulo(TimerModule & timer);

    optional<reference_wrapper<TimerModule>> m_timer;

private:
    static const vector<pair<uint8_t, uint16_t>> SETTINGS;

    GameBoy m_gb;
};

const vector<pair<uint8_t, uint16_t>> TimerTest::SETTINGS = {
    { 0x00, TimerModule::TIMEOUT_4K   },
    { 0x01, TimerModule::TIMEOUT_252K },
    { 0x02, TimerModule::TIMEOUT_65K  },
    { 0x03, TimerModule::TIMEOUT_16K  },
};

void TimerTest::SetUp()
{
    m_timer = std::ref(m_gb.cpu().m_timer);
    m_timer->get().reset();
}

void TimerTest::testInit(TimerModule & timer)
{
    MemoryController & memory = m_gb.mmc();

    EXPECT_EQ(timer.m_ticks, 0);
    EXPECT_EQ(timer.m_rtc, 0);
    EXPECT_EQ(timer.m_divider, 0);
    EXPECT_EQ(timer.m_counter, 0);
    EXPECT_EQ(timer.m_modulo, 0);
    EXPECT_EQ(timer.m_control, 0);

    EXPECT_EQ(memory.read(CPU_TIMER_DIV_ADDRESS), 0);
    EXPECT_EQ(memory.read(CPU_TIMER_COUNTER_ADDRESS), 0);
    EXPECT_EQ(memory.read(CPU_TIMER_MODULO_ADDRESS), 0);
    EXPECT_EQ(memory.read(CPU_TIMER_CONTROL_ADDRESS), 0);
}
TEST_F(TimerTest, Init) { testInit(m_timer.value()); }

void TimerTest::testTimeoutSetting(TimerModule & timer)
{
    MemoryController & memory = m_gb.mmc();

    for (const auto & setting : SETTINGS) {
        auto [frequency, timeout] = setting;

        memory.write(CPU_TIMER_CONTROL_ADDRESS, frequency | TimerModule::TIMER_ENABLE);
        timer.cycle(1);

        EXPECT_EQ(timeout, timer.m_timeout);
    }
}
TEST_F(TimerTest, TimeoutSetting) { testTimeoutSetting(m_timer.value()); }

void TimerTest::testCounterTick(TimerModule & timer)
{
    MemoryController & memory = m_gb.mmc();

    for (const auto & setting : SETTINGS) {
        auto [frequency, timeout] = setting;

        timer.reset();
        memory.write(CPU_TIMER_CONTROL_ADDRESS, frequency | TimerModule::TIMER_ENABLE);

        for (uint16_t i = 0; i < timeout; ++i) { timer.cycle(1); }

        EXPECT_EQ(1, timer.m_counter);
        EXPECT_EQ(1, memory.read(CPU_TIMER_COUNTER_ADDRESS));
        EXPECT_EQ(0, timer.m_ticks);

        for (uint8_t i = 0; i < 0xFF; ++i) {
            for (uint16_t j = 0; j < timeout; ++j) { timer.cycle(1); }

            if (0xFE != i) {
                ASSERT_EQ(i + 2, timer.m_counter);
                ASSERT_EQ(i + 2, memory.read(CPU_TIMER_COUNTER_ADDRESS));
            }
        }

        EXPECT_EQ(0, timer.m_counter);
        EXPECT_EQ(0, memory.read(CPU_TIMER_COUNTER_ADDRESS));

        uint8_t & ints = memory.read(INTERRUPT_FLAGS_ADDRESS);
        EXPECT_EQ(ints & uint8_t(InterruptMask::TIMER), uint8_t(InterruptMask::TIMER));

        ints = uint8_t(InterruptMask::NONE);
    }
}
TEST_F(TimerTest, CounterTick) { testCounterTick(m_timer.value()); }

void TimerTest::testTimerModulo(TimerModule & timer)
{
    constexpr uint8_t modulo = 0x32;

    MemoryController & memory = m_gb.mmc();
    memory.write(CPU_TIMER_MODULO_ADDRESS, modulo);

    EXPECT_EQ(modulo, timer.m_modulo);

    memory.write(CPU_TIMER_CONTROL_ADDRESS, TimerModule::TIMEOUT_4K | TimerModule::TIMER_ENABLE);

    timer.m_ticks   = TimerModule::TIMEOUT_4K - 1;
    timer.m_counter = 0xFF;

    timer.cycle(1);

    EXPECT_EQ(modulo, timer.m_counter);
}
TEST_F(TimerTest, Modulo) { testTimerModulo(m_timer.value()); }
