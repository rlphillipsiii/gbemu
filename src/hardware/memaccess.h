#ifndef _MEM_ACCESS_H
#define _MEM_ACCESS_H

#include <cstdint>

#include "gbproc.h"

class MemAccessListener {
public:
    virtual void onMemoryAccess(GB::MemAccessType type, uint16_t address) = 0;
};

#endif
