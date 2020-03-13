#include <gtest/gtest.h>

#include "gameboy.h"
#include "memmap.h"

class CpuTest : public ::testing::Test
{
protected:
    void testInit();
    void testSwap();

private:
    GameBoy m_gb;
};

void CpuTest::testInit()
{
    Processor & cpu = m_gb.cpu();

    EXPECT_EQ(cpu.m_gpr.a, 0);
}
TEST_F(CpuTest, Init) { testInit(); }

void CpuTest::testSwap()
{
    constexpr uint8_t original = 0xA5;
    constexpr uint8_t swapped  = 0x5A;

    Processor & cpu = m_gb.cpu();

    uint8_t value = original;
    cpu.swap(value);
    EXPECT_EQ(swapped, value);

    cpu.m_memory.write(WORKING_RAM_OFFSET, original);

    cpu.swap(WORKING_RAM_OFFSET);
    EXPECT_EQ(swapped, cpu.m_memory.read(WORKING_RAM_OFFSET));
}
TEST_F(CpuTest, Swap) { testSwap(); }
