#include <gtest/gtest.h>

#include "gameboy.h"

TEST(TestCpu, Init)
{
    GameBoy gb;

    Processor & cpu = gb.cpu();

    EXPECT_EQ(cpu.m_gpr.a, 0);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
