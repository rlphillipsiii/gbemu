#include <gtest/gtest.h>

#include "memorycontroller.h"
#include "clock.h"
#include "memmap.h"
#include "processor.h"

class CpuTest : public ::testing::Test
{
public:
    CpuTest() : m_cpu(m_clock, m_memory) { }
    ~CpuTest() { }

protected:
    void testInit();
    void testSwap();

private:
    MemoryController m_memory;
    ClockStub m_clock;

    Processor m_cpu;
};

void CpuTest::testInit()
{
    EXPECT_EQ(m_cpu.m_gpr.a, 0);
}
TEST_F(CpuTest, Init) { testInit(); }

void CpuTest::testSwap()
{
    constexpr uint8_t original = 0xA5;
    constexpr uint8_t swapped  = 0x5A;

    uint8_t value = original;
    m_cpu.swap(value);
    EXPECT_EQ(swapped, value);

    m_memory.write(WORKING_RAM_OFFSET, original);

    m_cpu.swap(WORKING_RAM_OFFSET);
    EXPECT_EQ(swapped, m_memory.read(WORKING_RAM_OFFSET));
}
TEST_F(CpuTest, Swap) { testSwap(); }
