#include <cassert>

#include "pipelink.h"
#include "memorycontroller.h"

PipeLink::PipeLink(MemoryController & memory)
    : ConsoleLink(memory)
{

}

PipeLink::~PipeLink()
{

}
