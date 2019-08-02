#include "memorycontroller.h"
#include "processor.h"

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    MemoryController memory;
    
    Processor cpu(memory);

    printf("%d\n", cpu.m_gpr.a);
    
    return 0;
}
