/*
 * processor.h
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <cstdint>

#include <map>
#include <functional>
#include <utility>
#include <string>
#include <mutex>

class MemoryController;

class Processor {
public:
    Processor(MemoryController & memory);
    ~Processor() = default;

    void reset();
    void cycle();

    inline void setVBlankInterrupt() { setInterrupt(MASK_VBLANK); }
    inline void setSerialInterrupt() { setInterrupt(MASK_SERIAL); }
    
private:
    static const uint8_t CB_PREFIX;

    enum InterruptVector {
        ISR_VBLANK = 0x40,
        ISR_LCD    = 0x48,
        ISR_TIMER  = 0x50,
        ISR_SERIAL = 0x58,
        ISR_JOYPAD = 0x60,
    };

    enum InterruptMask {
        MASK_VBLANK = 0x01,
        MASK_LCD    = 0x02,
        MASK_TIMER  = 0x04,
        MASK_SERIAL = 0x08,
        MASK_JOYPAD = 0x10,
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
    std::map<uint8_t, Operation> OPCODES;
    std::map<uint8_t, Operation> CB_OPCODES;

    MemoryController & m_memory;

    uint8_t m_ticks;

    /** Program counter that holds the address of the next instruction to fetch */
    uint16_t m_pc;

    /** Stack pointer that holds the next available address in the stack memory space */
    uint16_t m_sp;

    /** Interrupt enable, mask, and status register */
    struct Interrupts {
        Interrupts(uint8_t & m, uint8_t & s)
            : mask(m), status(s) { }
        ~Interrupts() = default;

        bool enable;
        
        uint8_t & mask;
        uint8_t & status;
    };

    Interrupts m_interrupts;
    
    std::mutex m_iLock;
    
    bool m_halted;

    std::array<uint8_t, 2> m_operands;

#if 0
    struct {
        union { struct { uint8_t f, a; }; uint16_t af; };
        union { struct { uint8_t c, b; }; uint16_t bc; };
        union { struct { uint8_t e, d; }; uint16_t de; };
        union { struct { uint8_t l, h; }; uint16_t hl; };
    } m_gpr;
#endif

    struct {
        union { struct { uint8_t a, f; }; uint16_t af; };
        union { struct { uint8_t b, c; }; uint16_t bc; };
        union { struct { uint8_t d, e; }; uint16_t de; };
        union { struct { uint8_t h, l; }; uint16_t hl; };
    } m_gpr;

    /** 8 bit flags register */
    uint8_t m_flags;

    Operation *lookup(uint8_t opcode);

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
    void add16(uint16_t & reg, uint16_t value);
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
    void rotatel(uint8_t & reg, bool wrap);
    void rotater(uint8_t & reg, bool wrap);
    void rst(uint8_t address);
    void loadMem(uint8_t value);
    void loadMem(uint16_t value);
    void loadMem(uint16_t ptr, uint8_t value);
    void loadMem(uint16_t ptr, uint16_t value);
    void load(uint8_t & reg, uint8_t value);
    void load(uint16_t & reg);
    void load(uint16_t & reg, uint16_t value);

    bool interrupt();

    inline uint16_t args() const { return (m_operands[1] << 8) | m_operands[0]; }
    inline void setInterrupt(uint8_t mask)
        { if (m_interrupts.mask & mask) { m_interrupts.status |= mask; } }
};



#endif /* PROCESSOR_H_ */
