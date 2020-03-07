/*
 * processor.h
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <cstdint>

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <utility>
#include <string>
#include <list>

#include "interrupt.h"
#include "timermodule.h"

class MemoryController;

class Processor {
public:
    struct Operation {
        std::string name;
        std::function<void()> handler;

        uint8_t length;
        uint8_t cycles;
    };

    struct Command {
        uint16_t pc;
        uint8_t opcode;
        std::array<uint8_t, 2> operands;
        const Operation *operation;

        void print() const;
        std::string str() const;
    };

    explicit Processor(MemoryController & memory);
    ~Processor() = default;

    void reset();
    uint8_t cycle();

    std::vector<Command> disassemble();

private:
#ifdef UNIT_TEST
    friend class TestCpu_Init_Test;
#endif

    static const uint16_t HISTORY_SIZE;

    static const uint8_t CB_PREFIX;

    static const uint16_t ROM_ENTRY_POINT;

    enum FlagMask {
        ZERO_FLAG_MASK       = 0x80,
        NEG_FLAG_MASK        = 0x40,
        HALF_CARRY_FLAG_MASK = 0x20,
        CARRY_FLAG_MASK      = 0x10,
    };

    std::unordered_map<uint8_t, Operation> OPCODES;
    std::unordered_map<uint8_t, Operation> CB_OPCODES;

    std::unordered_set<uint16_t> ILLEGAL_OPCODES;

    MemoryController & m_memory;

    uint8_t m_ticks;

    /** Program counter that holds the address of the next instruction to fetch */
    uint16_t m_pc;
    uint16_t m_instr;

    /** Stack pointer that holds the next available address in the stack memory space */
    uint16_t m_sp;

    /** Interrupt enable, mask, and status registers */
    Interrupts m_interrupts;

    bool m_halted;

    std::array<uint8_t, 2> m_operands;

    std::list<Command> m_executed;

    struct {
        union { struct { uint8_t f, a; }; uint16_t af; };
        union { struct { uint8_t c, b; }; uint16_t bc; };
        union { struct { uint8_t e, d; }; uint16_t de; };
        union { struct { uint8_t l, h; }; uint16_t hl; };
    } m_gpr;

    TimerModule m_timer;

    /** 8 bit flags register */
    uint8_t & m_flags;

    Operation *lookup(uint16_t & pc, uint8_t opcode);

    inline void setVBlankInterrupt() { Interrupts::set(m_memory, InterruptMask::VBLANK); }
    inline void setSerialInterrupt() { Interrupts::set(m_memory, InterruptMask::SERIAL); }
    inline void setLCDInterrupt()    { Interrupts::set(m_memory, InterruptMask::LCD);    }
    inline void setJoypadInterrupt() { Interrupts::set(m_memory, InterruptMask::JOYPAD); }

    inline bool isZeroFlagSet()      const { return (m_flags & ZERO_FLAG_MASK);       }
    inline bool isNegFlagSet()       const { return (m_flags & NEG_FLAG_MASK);        }
    inline bool isHalfCarryFlagSet() const { return (m_flags & HALF_CARRY_FLAG_MASK); }
    inline bool isCarryFlagSet()     const { return (m_flags & CARRY_FLAG_MASK);      }

    inline void setZeroFlag() { m_flags |= ZERO_FLAG_MASK;    }
    inline void clrZeroFlag() { m_flags &= (~ZERO_FLAG_MASK); }

    inline void setNegFlag() { m_flags |= NEG_FLAG_MASK;    }
    inline void clrNegFlag() { m_flags &= (~NEG_FLAG_MASK); }

    inline void setHalfCarryFlag() { m_flags |= HALF_CARRY_FLAG_MASK;    }
    inline void clrHalfCarryFlag() { m_flags &= (~HALF_CARRY_FLAG_MASK); }

    inline void setCarryFlag() { m_flags |= CARRY_FLAG_MASK;    }
    inline void clrCarryFlag() { m_flags &= (~CARRY_FLAG_MASK); }

    inline void inc(uint16_t & reg) { reg++; }
    inline void dec(uint16_t & reg) { reg--; }

    void and8(uint8_t value);
    void or8(uint8_t value);
    void xor8(uint8_t value);
    void add8(uint8_t value, bool carry);
    void sub8(uint8_t value, bool carry);
    void sbc(uint8_t value);
    void add16(uint16_t & reg, uint16_t value);
    void add16(uint16_t & dest, uint16_t reg, uint8_t value);
    void compare(uint8_t value);
    void swap(uint8_t & reg);
    void inc(uint8_t & reg);
    void dec(uint8_t & reg);
    void call();
    void jump();
    void jump(uint16_t address);
    void jumprel();
    void pop(uint16_t & reg);
    void push(uint16_t value);
    void ret(bool enable);
    void ccf();
    void compliment();
    void bit(uint8_t reg, uint8_t which);
    void res(uint8_t & reg, uint8_t which);
    void set(uint8_t & reg, uint8_t which);
    void rotatel(uint8_t & reg, bool wrap, bool ignoreZero);
    void rotater(uint8_t & reg, bool wrap, bool ignoreZero);
    void rlc(uint8_t & reg, bool ignoreZero);
    void rrc(uint8_t & reg, bool ignoreZero);
    void rst(uint8_t address);
    void loadMem(uint8_t value);
    void loadMem(uint16_t value);
    void loadMem(uint16_t ptr, uint8_t value);
    void loadMem(uint16_t ptr, uint16_t value);
    void load(uint8_t & reg, uint8_t value);
    void load(uint16_t & reg);
    void load(uint16_t & reg, uint16_t value);
    void sla(uint8_t & reg);
    void srl(uint8_t & reg);
    void sra(uint8_t & reg);
    void scf();
    void stop();
    void daa();

    bool interrupt();

    uint8_t execute();

    void history() const;

    void log(uint8_t opcode, const Operation *operation);
    void logRegisters() const;

    inline uint16_t args() const { return (uint16_t(m_operands[1]) << 8) | m_operands[0]; }
};



#endif /* PROCESSOR_H_ */
