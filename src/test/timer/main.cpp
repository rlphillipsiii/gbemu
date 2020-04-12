#include <gtest/gtest.h>
#include <vector>
#include <utility>

#include "memmap.h"
#include "memorycontroller.h"
#include "timermodule.h"
#include "interrupt.h"

using std::vector;
using std::pair;

class TimerTest : public ::testing::Test
{
public:
    TimerTest() : m_timer(m_memory) { }

protected:
    void SetUp() override;

    void testInit();
    void testTimeoutSetting();
    void testCounterTick();
    // TODO: void testRtcTick();
    void testTimerModulo();

    MemoryController m_memory;
    TimerModule m_timer;

private:
    static const vector<pair<uint8_t, uint16_t>> SETTINGS;
};

const vector<pair<uint8_t, uint16_t>> TimerTest::SETTINGS = {
    { 0x00, TimerModule::TIMEOUT_4K   },
    { 0x01, TimerModule::TIMEOUT_252K },
    { 0x02, TimerModule::TIMEOUT_65K  },
    { 0x03, TimerModule::TIMEOUT_16K  },
};

void TimerTest::SetUp()
{
    m_timer.reset();
}

void TimerTest::testInit()
{
    EXPECT_EQ(m_timer.m_ticks, 0);
    EXPECT_EQ(m_timer.m_rtc, 0);
    EXPECT_EQ(m_timer.m_divider, 0);
    EXPECT_EQ(m_timer.m_counter, 0);
    EXPECT_EQ(m_timer.m_modulo, 0);
    EXPECT_EQ(m_timer.m_control, 0);

    EXPECT_EQ(m_memory.read(CPU_TIMER_DIV_ADDRESS), 0);
    EXPECT_EQ(m_memory.read(CPU_TIMER_COUNTER_ADDRESS), 0);
    EXPECT_EQ(m_memory.read(CPU_TIMER_MODULO_ADDRESS), 0);
    EXPECT_EQ(m_memory.read(CPU_TIMER_CONTROL_ADDRESS), 0);
}
TEST_F(TimerTest, TimerInit) { testInit(); }

void TimerTest::testTimeoutSetting()
{
    for (const auto & setting : SETTINGS) {
        auto [frequency, timeout] = setting;

        m_memory.write(CPU_TIMER_CONTROL_ADDRESS, frequency | TimerModule::TIMER_ENABLE);
        m_timer.cycle(1);

        EXPECT_EQ(timeout, m_timer.m_timeout);
    }
}
TEST_F(TimerTest, TimeoutSetting) { testTimeoutSetting(); }

void TimerTest::testCounterTick()
{
    for (const auto & setting : SETTINGS) {
        auto [frequency, timeout] = setting;

        m_timer.reset();
        m_memory.write(CPU_TIMER_CONTROL_ADDRESS, frequency | TimerModule::TIMER_ENABLE);

        for (uint16_t i = 0; i < timeout; ++i) { m_timer.cycle(1); }

        EXPECT_EQ(1, m_timer.m_counter);
        EXPECT_EQ(1, m_memory.read(CPU_TIMER_COUNTER_ADDRESS));
        EXPECT_EQ(0, m_timer.m_ticks);

        for (uint8_t i = 0; i < 0xFF; ++i) {
            for (uint16_t j = 0; j < timeout; ++j) { m_timer.cycle(1); }

            if (0xFE != i) {
                ASSERT_EQ(i + 2, m_timer.m_counter);
                ASSERT_EQ(i + 2, m_memory.read(CPU_TIMER_COUNTER_ADDRESS));
            }
        }

        EXPECT_EQ(0, m_timer.m_counter);
        EXPECT_EQ(0, m_memory.read(CPU_TIMER_COUNTER_ADDRESS));

        uint8_t & ints = m_memory.read(INTERRUPT_FLAGS_ADDRESS);
        EXPECT_EQ(ints & uint8_t(InterruptMask::TIMER), uint8_t(InterruptMask::TIMER));

        ints = uint8_t(InterruptMask::NONE);
    }
}
TEST_F(TimerTest, CounterTick) { testCounterTick(); }

void TimerTest::testTimerModulo()
{
    constexpr uint8_t modulo = 0x32;

    m_memory.write(CPU_TIMER_MODULO_ADDRESS, modulo);

    EXPECT_EQ(modulo, m_timer.m_modulo);

    m_memory.write(CPU_TIMER_CONTROL_ADDRESS, TimerModule::TIMEOUT_4K | TimerModule::TIMER_ENABLE);

    m_timer.m_ticks   = TimerModule::TIMEOUT_4K - 1;
    m_timer.m_counter = 0xFF;

    m_timer.cycle(1);

    EXPECT_EQ(modulo, m_timer.m_counter);
}
TEST_F(TimerTest, Modulo) { testTimerModulo(); }
