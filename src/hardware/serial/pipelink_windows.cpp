#include <cassert>

#include "pipelink.h"
#include "memorycontrollerh"

PipeLink::PipeLink(MemoryController & memory)
    : ConsoleLink(memory)
{

}

PipeLink::~PipeLink()
{

}

void PipeLink::transfer(uint8_t)
{
    assert(0);
}
