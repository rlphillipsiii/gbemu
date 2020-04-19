#ifndef _GB_PROC_H
#define _GB_PROC_H

#include <functional>
#include <string>
#include <cstdint>
#include <array>

namespace GB {
    enum MemAccessType {
        MEM_ACCESS_READ,
        MEM_ACCESS_WRITE,
        MEM_ACCESS_EITHER,
    };

    enum FlagMask {
        ZERO_FLAG_MASK       = 0x80,
        NEG_FLAG_MASK        = 0x40,
        HALF_CARRY_FLAG_MASK = 0x20,
        CARRY_FLAG_MASK      = 0x10,
    };

    struct Operation {
        std::string name;
        std::function<void()> handler;

        uint8_t length;
        uint8_t cycles;
    };

    struct Command {
        uint16_t pc;
        uint16_t sp;
        uint8_t opcode;
        uint8_t flags;
        bool ints;
        uint8_t iMask;
        uint8_t iStatus;
        uint8_t a;
        uint16_t bc;
        uint16_t de;
        uint16_t hl;
        uint8_t scanline;
        uint8_t stat;
        uint8_t romBank;
        uint8_t ramBank;
        std::array<uint8_t, 2> operands;
        const Operation *operation;

        void print() const;
        std::string str() const;
        std::string status() const;
        std::string registers() const;
        std::string abbrev() const;
    };
}

#endif
