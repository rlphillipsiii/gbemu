/*
 * processor.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <iostream>
#include <cassert>

#include "processor.h"
#include "memorycontroller.h"

using std::map;
using std::function;
using std::pair;
using std::string;

const uint8_t Processor::CB_PREFIX = 0xCB;

void Processor::reset()
{
    m_pc = m_sp = 0x0000;

    m_flags = 0x00;

    m_gpr.af = m_gpr.bc = m_gpr.de = m_gpr.hl = 0x0000;

    m_interrupts = true;
}

Processor::Operation *Processor::lookup(uint8_t opcode)
{
    // Figure out which table we should be looking in for the next opcode.
    auto & table = (CB_PREFIX == opcode) ? CB_OPCODES : OPCODES;

    // If this isn't the CB prefix, we don't really care about our prefix.  If it
    // is then we need to grab the next by as that is our actual opcode, and then
    // save the prefix for debugging purposes.
    uint8_t prefix = 0x00;
    if (CB_PREFIX == opcode) {
        prefix = opcode;

        opcode = m_memory.read(m_pc++);
    }

    // Lookup the opcode in the table that we just selected.  If we couldn't find it,
    // then there's a bug and we have a fatal error.  Debug builds will assert and print
    // an error out.  Release builds will return the noop handler.
    auto iterator = table.find(opcode);
    if (iterator == table.end()) {
        printf("Unknown opcode: (0x%02x ==> 0x%02x)\n", prefix, opcode);
        assert(0);

        return &(OPCODES[0x00]);
    }

    return &(iterator->second);
}

void Processor::cycle()
{
    // We are actually doing everything in one cycle, so to maintain the same timing
    // as the processor, we are going to just return until we have burned the same
    // number of cycles as the opcode takes on the actual hardware.
    if (--m_ticks != 0) { return; }

    // Make sure that our operands are always in a known state.  We can take advantage
    // of this later on during our command execution.
    m_operands[0] = m_operands[1] = 0x00;

    // The program counter points to our next opcode.
    uint8_t opcode = m_memory.read(m_pc++);

    // Look up the handler in our opcode table.  If the length of our command is greater
    // than 1, then we also need to grab the next length - 1 bytes as they are the
    // arguments to the next operation that we are going to execute.
    Operation *operation = lookup(opcode);
    for (uint8_t i = 0; i < operation->length - 1; i++) {
        m_operands[i] = m_memory.read(m_pc++);
    }

    // Call the function pointer in our Operation struct.  Any arguments to the function
    // will have already been put in to the operands array.
    operation->handler();

    // Set the number of ticks so that the next pass through will burn additional cycles
    // if necessary.  We want to add to the ticks here because conditional commands will
    // set the base value based on whether or not the condition was hit while we were
    // executing the command (i.e. call if z will set ticks to 3 if the call was made
    // and leave it at 0 if it was not).
    m_ticks += operation->cycles;
}

void Processor::xor8(uint8_t value)
{
    m_flags = 0x00;

    m_gpr.a ^= value;
    if (!m_gpr.a) { setZeroFlag(); }

    setHalfCarryFlag();
}

void Processor::and8(uint8_t value)
{
    m_flags = 0x00;

    m_gpr.a &= value;
    if (!m_gpr.a) { setZeroFlag(); }

    setHalfCarryFlag();
}

void Processor::or8(uint8_t value)
{
    m_flags = 0x00;

    m_gpr.a |= value;
    if (!m_gpr.a) { setZeroFlag(); }
}

void Processor::inc(uint8_t & reg)
{
    (0xF == reg) ? setHalfCarryFlag() : clrHalfCarryFlag();

    reg++;
    (0x00 == reg) ? setZeroFlag() : clrZeroFlag();

    clrNegFlag();
}

void Processor::dec(uint8_t & reg)
{
    ((reg & 0x0F) == 0x00) ? setHalfCarryFlag() : clrHalfCarryFlag();

    reg--;
    (0x00 == reg) ? setZeroFlag() : clrZeroFlag();

    setNegFlag();
}

void Processor::add8(uint8_t value, bool carry)
{
    m_flags = 0x00;

    uint16_t result = uint16_t(m_gpr.a) + uint16_t(value);
    if (carry && isCarryFlagSet()) { result += 1; }

    if (result & 0xFF00) { setCarryFlag(); }

    uint8_t half = (m_gpr.a & 0xF) + (value & 0xF);
    if (carry && isCarryFlagSet()) { half += 1; }

    if (half & 0xF0) { setHalfCarryFlag(); }

    if (!(m_gpr.a = result & 0xFF)) {
        setZeroFlag();
    }
}

void Processor::sub8(uint8_t value, bool carry)
{
    m_flags = 0x00;

    if (carry) { value += 1; }

    if (m_gpr.a < value) { setCarryFlag(); }

    if ((m_gpr.a & 0x0F) < (value & 0x0F)) { setHalfCarryFlag(); }

    m_gpr.a -= value;

    if (0x00 == m_gpr.a) { setZeroFlag(); }

    setNegFlag();
}

void Processor::add16(uint16_t value)
{
    (void)value;
}

void Processor::swap(uint8_t & reg)
{
    m_flags = 0x00;

    uint8_t upper = (reg >> 4) & 0x0F;
    uint8_t lower = reg & 0x0F;

    reg = (lower << 4) | upper;

    if (0x00 == reg) { setZeroFlag(); }
}

void Processor::call()
{
    push(m_pc);

    m_pc = (m_operands[1] << 8) | m_operands[0];

    m_ticks = 3;
}

void Processor::jump()
{
    uint16_t address = (m_operands[1] << 8) | m_operands[0];
    jump(address);
}

void Processor::jumprel()
{
    uint16_t address = m_pc + uint16_t(m_operands[0]);
    jump(address);
}

void Processor::jump(uint16_t address)
{
    m_pc = address;

    m_ticks = 1;
}

void Processor::ccf()
{
    if (isCarryFlagSet()) {
        clrCarryFlag();
    } else {
        setCarryFlag();
    }

    clrHalfCarryFlag();
    clrNegFlag();
}

void Processor::compare(uint8_t value)
{
    m_flags = 0x00;

    setNegFlag();

    if (m_gpr.a == value) {
        setZeroFlag();
    }
    if (m_gpr.a < value) {
        setCarryFlag();
    }
    if ((m_gpr.a & 0xF) < (value & 0xF)) {
        setHalfCarryFlag();
    }
}

void Processor::compliment()
{
    uint8_t value = 0x00;
    for (uint8_t i = 0; i < 8; i++) {
        if (!(m_gpr.a & (1 << i))) {
            value |= (1 << i);
        }
    }

    m_gpr.a = value;
}

void Processor::bit(uint8_t reg, uint8_t which)
{
    if (reg & (1 << which)) {
        clrZeroFlag();
    } else {
        setZeroFlag();
    }

    clrNegFlag();
    setHalfCarryFlag();
}

void Processor::res(uint8_t & reg, uint8_t which)
{
    reg &= ~(1 << which);
}

void Processor::set(uint8_t & reg, uint8_t which)
{
    reg |= (1 << which);
}

void Processor::pop(uint16_t & reg)
{
    uint8_t lower = m_memory.read(m_sp++);
    uint8_t upper = m_memory.read(m_sp++);

    reg = (upper << 8) | lower;
}

void Processor::push(uint16_t value)
{
    uint8_t lower = value & 0xFF;
    uint8_t upper = (value >> 8) & 0xFF;

    m_memory.write(m_sp--, upper);
    m_memory.write(m_sp--, lower);
}

void Processor::ret(bool enable)
{
    pop(m_pc);

    if (enable) {
        m_interrupts = true;
    }
}

void Processor::rotatel(uint8_t & reg, bool wrap)
{
    uint8_t lsb = (isCarryFlagSet()) ? 0x01 : 0x00;

    (reg & (1 << 7)) ? setCarryFlag() : clrCarryFlag();

    reg <<= 1;
    if (wrap) { reg |= lsb; }

    (0x00 == reg) ? setZeroFlag() : clrZeroFlag();

    clrNegFlag();
    clrHalfCarryFlag();
}

void Processor::rotater(uint8_t & reg, bool wrap)
{
    uint8_t msb = (isCarryFlagSet()) ? 0x80 : 0x00;

    (reg & 0x01) ? setCarryFlag() : clrCarryFlag();

    reg >>= 1;
    if (wrap) { reg |= msb; }

    (0x00 == reg) ? setZeroFlag() : clrZeroFlag();

    clrNegFlag();
    clrHalfCarryFlag();
}

void Processor::rst(uint8_t address)
{
    push(m_pc);

    m_pc = 0x0000 + address;
}

void Processor::loadMem(uint8_t value)
{
    uint16_t ptr = (m_operands[1] << 8) | m_operands[0];
    loadMem(ptr, value);
}

void Processor::loadMem(uint16_t ptr, uint8_t value)
{
    m_memory.write(ptr, value);
}

void Processor::load(uint8_t & reg, uint8_t value)
{
    reg = value;
}

void Processor::load(uint16_t & reg)
{
    uint16_t value = (m_operands[1] << 8) | m_operands[0];
    load(reg, value);
}
void Processor::load(uint16_t & reg, uint16_t value)
{
    reg = value;
}

////////////////////////////////////////////////////////////////////////////////
Processor::Processor(MemoryController & memory)
    : m_memory(memory),
      m_ticks(1),
      m_halted(false),
      m_flags(m_gpr.f)
{
    reset();

    OPCODES = {
        // { opcode, { name, lambda function, length, cycles } }
        { 0x00, { "NOP", [this]() { }, 1, 1 } },

        { 0xCE, { "ADC_a_n",  [this]() { add8(m_operands[0], true); }, 2, 2 } },
        { 0x8F, { "ADC_a_a",  [this]() { add8(m_gpr.a, true);       }, 1, 1 } },
        { 0x88, { "ADC_a_b",  [this]() { add8(m_gpr.b, true);       }, 1, 1 } },
        { 0x89, { "ADC_a_c",  [this]() { add8(m_gpr.c, true);       }, 1, 1 } },
        { 0x8A, { "ADC_a_d",  [this]() { add8(m_gpr.d, true);       }, 1, 1 } },
        { 0x8B, { "ADC_a_e",  [this]() { add8(m_gpr.e, true);       }, 1, 1 } },
        { 0x8C, { "ADC_a_h",  [this]() { add8(m_gpr.h, true);       }, 1, 1 } },
        { 0x8D, { "ADC_a_l",  [this]() { add8(m_gpr.l, true);       }, 1, 1 } },

        { 0x8E, { "ADC_a_(hl)", [this]() { add8(m_memory.read(m_gpr.hl), true); }, 1, 2 } },

        { 0xC6, { "ADD_a_n",  [this]() { add8(m_operands[0], false); }, 2, 2 } },
        { 0x87, { "ADD_a_a",  [this]() { add8(m_gpr.a, false);       }, 1, 1 } },
        { 0x80, { "ADD_a_b",  [this]() { add8(m_gpr.b, false);       }, 1, 1 } },
        { 0x81, { "ADD_a_c",  [this]() { add8(m_gpr.c, false);       }, 1, 1 } },
        { 0x82, { "ADD_a_d",  [this]() { add8(m_gpr.d, false);       }, 1, 1 } },
        { 0x83, { "ADD_a_e",  [this]() { add8(m_gpr.e, false);       }, 1, 1 } },
        { 0x84, { "ADD_a_h",  [this]() { add8(m_gpr.h, false);       }, 1, 1 } },
        { 0x85, { "ADD_a_l",  [this]() { add8(m_gpr.l, false);       }, 1, 1 } },

        { 0x86, { "ADD_a_(hl)", [this]() { add8(m_memory.read(m_gpr.hl), false); }, 1, 2 } },

        { 0x09, { "ADD_hl_bc", [this]() { add16(m_gpr.bc); }, 1, 2 } },
        { 0x19, { "ADD_hl_de", [this]() { add16(m_gpr.de); }, 1, 2 } },
        { 0x29, { "ADD_hl_hl", [this]() { add16(m_gpr.hl); }, 1, 2 } },
        { 0x39, { "ADD_hl_sp", [this]() { add16(m_sp);     }, 1, 2 } },

        { 0xE8, { "ADD_sp_n", [this]() { add16(m_operands[0]); }, 2, 4 } },

        { 0xE6, { "AND_a_n",  [this]() { and8(m_operands[0]); }, 2, 2 } },
        { 0xA7, { "AND_a_a",  [this]() { and8(m_gpr.a);       }, 1, 1 } },
        { 0xA0, { "AND_a_b",  [this]() { and8(m_gpr.b);       }, 1, 1 } },
        { 0xA1, { "AND_a_c",  [this]() { and8(m_gpr.c);       }, 1, 1 } },
        { 0xA2, { "AND_a_d",  [this]() { and8(m_gpr.d);       }, 1, 1 } },
        { 0xA3, { "AND_a_e",  [this]() { and8(m_gpr.e);       }, 1, 1 } },
        { 0xA4, { "AND_a_h",  [this]() { and8(m_gpr.h);       }, 1, 1 } },
        { 0xA5, { "AND_a_l",  [this]() { and8(m_gpr.l);       }, 1, 1 } },

        { 0xA6, { "AND_a_(hl)", [this]() { and8(m_memory.read(m_gpr.hl)); }, 1, 2 } },

        { 0xCD, { "CALL",    [this]() { call();                           }, 3, 3 } },
        { 0xDC, { "CALL_C",  [this]() { if (isCarryFlagSet())  { call(); }}, 3, 3 } },
        { 0xD4, { "CALL_NC", [this]() { if (!isCarryFlagSet()) { call(); }}, 3, 3 } },
        { 0xCC, { "CALL_Z",  [this]() { if (isZeroFlagSet())   { call(); }}, 3, 3 } },
        { 0xC4, { "CALL_NZ", [this]() { if (!isZeroFlagSet())  { call(); }}, 3, 3 } },

        { 0x3F, { "CCF", [this]() { ccf(); }, 1, 1 } },

        { 0xFE, { "CP_n",  [this]() { compare(m_operands[0]); }, 2, 2 } },
        { 0xBF, { "CP_a",  [this]() { compare(m_gpr.a);       }, 1, 1 } },
        { 0xB8, { "CP_b",  [this]() { compare(m_gpr.b);       }, 1, 1 } },
        { 0xB9, { "CP_c",  [this]() { compare(m_gpr.c);       }, 1, 1 } },
        { 0xBA, { "CP_d",  [this]() { compare(m_gpr.d);       }, 1, 1 } },
        { 0xBB, { "CP_e",  [this]() { compare(m_gpr.e);       }, 1, 1 } },
        { 0xBC, { "CP_h",  [this]() { compare(m_gpr.h);       }, 1, 1 } },
        { 0xBD, { "CP_l",  [this]() { compare(m_gpr.l);       }, 1, 1 } },

        { 0xBE, { "CP_(hl)", [this]() { compare(m_memory.read(m_gpr.hl)); }, 1, 2 } },

        { 0x2F, { "CPL", [this]() { compliment(); }, 1, 1 } },

        { 0x27, { "DAA", [this]() { assert(0); }, 1, 1 } },

        { 0x35, { "DEC_(hl)", [this]() { dec(m_memory.read(m_gpr.hl)); }, 1, 3 } },
        { 0x3D, { "DEC_a",    [this]() { dec(m_gpr.a);                 }, 1, 1 } },
        { 0x05, { "DEC_b",    [this]() { dec(m_gpr.b);                 }, 1, 1 } },
        { 0x0B, { "DEC_bc",   [this]() { dec(m_gpr.bc);                }, 1, 2 } },
        { 0x0D, { "DEC_c",    [this]() { dec(m_gpr.c);                 }, 1, 1 } },
        { 0x15, { "DEC_d",    [this]() { dec(m_gpr.d);                 }, 1, 1 } },
        { 0x1B, { "DEC_de",   [this]() { dec(m_gpr.de);                }, 1, 2 } },
        { 0x1D, { "DEC_e",    [this]() { dec(m_gpr.e);                 }, 1, 1 } },
        { 0x25, { "DEC_h",    [this]() { dec(m_gpr.h);                 }, 1, 1 } },
        { 0x2B, { "DEC_hl",   [this]() { dec(m_gpr.hl);                }, 1, 2 } },
        { 0x2D, { "DEC_l",    [this]() { dec(m_gpr.l);                 }, 1, 1 } },
        { 0x3B, { "DEC_sp",   [this]() { dec(m_sp);                    }, 1, 2 } },

        { 0xF3, { "DI",   [this]() { m_interrupts = false; }, 1, 1 } },
        { 0xFB, { "EI",   [this]() { m_interrupts = true;  }, 1, 1 } },
        { 0x76, { "HALT", [this]() { m_halted = true;      }, 1, 1 } },

        { 0x34, { "INC_(hl)", [this]() { inc(m_memory.read(m_gpr.hl)); }, 1, 3 } },
        { 0x3C, { "INC_a",    [this]() { inc(m_gpr.a);                 }, 1, 1 } },
        { 0x04, { "INC_b",    [this]() { inc(m_gpr.b);                 }, 1, 1 } },
        { 0x03, { "INC_bc",   [this]() { inc(m_gpr.bc);                }, 1, 2 } },
        { 0x0C, { "INC_c",    [this]() { inc(m_gpr.c);                 }, 1, 1 } },
        { 0x14, { "INC_d",    [this]() { inc(m_gpr.d);                 }, 1, 1 } },
        { 0x13, { "INC_de",   [this]() { inc(m_gpr.de);                }, 1, 2 } },
        { 0x1C, { "INC_e",    [this]() { inc(m_gpr.e);                 }, 1, 1 } },
        { 0x24, { "INC_h",    [this]() { inc(m_gpr.h);                 }, 1, 1 } },
        { 0x23, { "INC_hl",   [this]() { inc(m_gpr.hl);                }, 1, 2 } },
        { 0x2C, { "INC_l",    [this]() { inc(m_gpr.l);                 }, 1, 1 } },
        { 0x33, { "INC_sp",   [this]() { inc(m_sp);                    }, 1, 2 } },

        { 0xC3, { "JP_n",    [this]() { jump();                        }, 3, 3 } },
        { 0xE9, { "JP_(hl)", [this]() { jump(m_memory.read(m_gpr.hl)); }, 1, 1 } },

        { 0xDA, { "JP_C",  [this]() { if (isCarryFlagSet())  { jump(); }}, 3, 3 } },
        { 0xD2, { "JP_NC", [this]() { if (!isCarryFlagSet()) { jump(); }}, 3, 3 } },
        { 0xC2, { "JP_NZ", [this]() { if (!isZeroFlagSet())  { jump(); }}, 3, 3 } },
        { 0xCA, { "JP_Z",  [this]() { if (isZeroFlagSet())   { jump(); }}, 3, 3 } },

        { 0x18, { "JR_n",  [this]() { jumprel();                           }, 2, 2 } },
        { 0x38, { "JR_C",  [this]() { if (isCarryFlagSet())  { jumprel(); }}, 2, 2 } },
        { 0x30, { "JR_NC", [this]() { if (!isCarryFlagSet()) { jumprel(); }}, 2, 2 } },
        { 0x20, { "JR_NZ", [this]() { if (!isZeroFlagSet())  { jumprel(); }}, 2, 2 } },
        { 0x28, { "JR_Z",  [this]() { if (isZeroFlagSet())   { jumprel(); }}, 2, 2 } },

        { 0x7F, { "LD_a_a", [this]() { m_gpr.a = m_gpr.a; }, 1, 1 } },
        { 0x78, { "LD_a_b", [this]() { m_gpr.a = m_gpr.b; }, 1, 1 } },
        { 0x79, { "LD_a_c", [this]() { m_gpr.a = m_gpr.c; }, 1, 1 } },
        { 0x7A, { "LD_a_d", [this]() { m_gpr.a = m_gpr.d; }, 1, 1 } },
        { 0x7B, { "LD_a_e", [this]() { m_gpr.a = m_gpr.e; }, 1, 1 } },
        { 0x7C, { "LD_a_h", [this]() { m_gpr.a = m_gpr.h; }, 1, 1 } },
        { 0x7D, { "LD_a_l", [this]() { m_gpr.a = m_gpr.l; }, 1, 1 } },

        { 0x32, { "LDD_(hl)_n", [this]() { loadMem(m_gpr.hl--, m_gpr.a); }, 1, 2 } },

        { 0x36, { "LD_(hl)_n", [this]() { loadMem(m_gpr.hl, m_operands[0]); }, 2, 3 } },
        { 0x77, { "LD_(hl)_a", [this]() { loadMem(m_gpr.hl, m_gpr.a);       }, 1, 2 } },
        { 0x70, { "LD_(hl)_b", [this]() { loadMem(m_gpr.hl, m_gpr.b);       }, 1, 2 } },
        { 0x71, { "LD_(hl)_c", [this]() { loadMem(m_gpr.hl, m_gpr.c);       }, 1, 2 } },
        { 0x72, { "LD_(hl)_d", [this]() { loadMem(m_gpr.hl, m_gpr.d);       }, 1, 2 } },
        { 0x73, { "LD_(hl)_e", [this]() { loadMem(m_gpr.hl, m_gpr.e);       }, 1, 2 } },
        { 0x74, { "LD_(hl)_h", [this]() { loadMem(m_gpr.hl, m_gpr.h);       }, 1, 2 } },
        { 0x75, { "LD_(hl)_l", [this]() { loadMem(m_gpr.hl, m_gpr.l);       }, 1, 2 } },

        { 0x21, { "LD_hl_n", [this]() { load(m_gpr.hl); }, 3, 3 } },
        { 0x31, { "LD_sp_n", [this]() { load(m_sp);     }, 3, 3 } },

        { 0xEE, { "XOR_a_n",  [this]() { xor8(m_operands[0]); }, 2, 2 } },
        { 0xAF, { "XOR_a_a",  [this]() { xor8(m_gpr.a);       }, 1, 1 } },
        { 0xA8, { "XOR_a_b",  [this]() { xor8(m_gpr.b);       }, 1, 1 } },
        { 0xA9, { "XOR_a_c",  [this]() { xor8(m_gpr.c);       }, 1, 1 } },
        { 0xAA, { "XOR_a_d",  [this]() { xor8(m_gpr.d);       }, 1, 1 } },
        { 0xAB, { "XOR_a_e",  [this]() { xor8(m_gpr.e);       }, 1, 1 } },
        { 0xAC, { "XOR_a_h",  [this]() { xor8(m_gpr.h);       }, 1, 1 } },
        { 0xAD, { "XOR_a_l",  [this]() { xor8(m_gpr.l);       }, 1, 1 } },

        { 0xAE, { "XOR_a_(hl)", [this]() { xor8(m_memory.read(m_gpr.hl)); }, 1, 2 } },

    };

    CB_OPCODES = {
        { 0x46, { "BIT_0_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 0); }, 2, 3 } },
        { 0x47, { "BIT_0_a", [this]() { bit(m_gpr.a, 0); }, 2, 2 } },
        { 0x40, { "BIT_0_b", [this]() { bit(m_gpr.b, 0); }, 2, 2 } },
        { 0x41, { "BIT_0_c", [this]() { bit(m_gpr.c, 0); }, 2, 2 } },
        { 0x42, { "BIT_0_d", [this]() { bit(m_gpr.d, 0); }, 2, 2 } },
        { 0x43, { "BIT_0_e", [this]() { bit(m_gpr.e, 0); }, 2, 2 } },
        { 0x44, { "BIT_0_h", [this]() { bit(m_gpr.h, 0); }, 2, 2 } },
        { 0x45, { "BIT_0_l", [this]() { bit(m_gpr.l, 0); }, 2, 2 } },
        { 0x4E, { "BIT_1_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 1); }, 2, 3 } },
        { 0x4F, { "BIT_1_a", [this]() { bit(m_gpr.a, 1); }, 2, 2 } },
        { 0x48, { "BIT_1_b", [this]() { bit(m_gpr.b, 1); }, 2, 2 } },
        { 0x49, { "BIT_1_c", [this]() { bit(m_gpr.c, 1); }, 2, 2 } },
        { 0x4A, { "BIT_1_d", [this]() { bit(m_gpr.d, 1); }, 2, 2 } },
        { 0x4B, { "BIT_1_e", [this]() { bit(m_gpr.e, 1); }, 2, 2 } },
        { 0x4C, { "BIT_1_h", [this]() { bit(m_gpr.h, 1); }, 2, 2 } },
        { 0x4D, { "BIT_1_l", [this]() { bit(m_gpr.l, 1); }, 2, 2 } },
        { 0x56, { "BIT_2_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 2); }, 2, 3 } },
        { 0x57, { "BIT_2_a", [this]() { bit(m_gpr.a, 2); }, 2, 2 } },
        { 0x50, { "BIT_2_b", [this]() { bit(m_gpr.b, 2); }, 2, 2 } },
        { 0x51, { "BIT_2_c", [this]() { bit(m_gpr.c, 2); }, 2, 2 } },
        { 0x52, { "BIT_2_d", [this]() { bit(m_gpr.d, 2); }, 2, 2 } },
        { 0x53, { "BIT_2_e", [this]() { bit(m_gpr.e, 2); }, 2, 2 } },
        { 0x54, { "BIT_2_h", [this]() { bit(m_gpr.h, 2); }, 2, 2 } },
        { 0x55, { "BIT_2_l", [this]() { bit(m_gpr.l, 2); }, 2, 2 } },
        { 0x5E, { "BIT_3_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 3); }, 2, 3 } },
        { 0x5F, { "BIT_3_a", [this]() { bit(m_gpr.a, 3); }, 2, 2 } },
        { 0x58, { "BIT_3_b", [this]() { bit(m_gpr.b, 3); }, 2, 2 } },
        { 0x59, { "BIT_3_c", [this]() { bit(m_gpr.c, 3); }, 2, 2 } },
        { 0x5A, { "BIT_3_d", [this]() { bit(m_gpr.d, 3); }, 2, 2 } },
        { 0x5B, { "BIT_3_e", [this]() { bit(m_gpr.e, 3); }, 2, 2 } },
        { 0x5C, { "BIT_3_h", [this]() { bit(m_gpr.h, 3); }, 2, 2 } },
        { 0x5D, { "BIT_3_l", [this]() { bit(m_gpr.l, 3); }, 2, 2 } },
        { 0x66, { "BIT_4_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 4); }, 2, 3 } },
        { 0x67, { "BIT_4_a", [this]() { bit(m_gpr.a, 4); }, 2, 2 } },
        { 0x60, { "BIT_4_b", [this]() { bit(m_gpr.b, 4); }, 2, 2 } },
        { 0x61, { "BIT_4_c", [this]() { bit(m_gpr.c, 4); }, 2, 2 } },
        { 0x62, { "BIT_4_d", [this]() { bit(m_gpr.d, 4); }, 2, 2 } },
        { 0x63, { "BIT_4_e", [this]() { bit(m_gpr.e, 4); }, 2, 2 } },
        { 0x64, { "BIT_4_h", [this]() { bit(m_gpr.h, 4); }, 2, 2 } },
        { 0x65, { "BIT_4_l", [this]() { bit(m_gpr.l, 4); }, 2, 2 } },
        { 0x6E, { "BIT_5_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 5); }, 2, 3 } },
        { 0x6F, { "BIT_5_a", [this]() { bit(m_gpr.a, 5); }, 2, 2 } },
        { 0x68, { "BIT_5_b", [this]() { bit(m_gpr.b, 5); }, 2, 2 } },
        { 0x69, { "BIT_5_c", [this]() { bit(m_gpr.c, 5); }, 2, 2 } },
        { 0x6A, { "BIT_5_d", [this]() { bit(m_gpr.d, 5); }, 2, 2 } },
        { 0x6B, { "BIT_5_e", [this]() { bit(m_gpr.e, 5); }, 2, 2 } },
        { 0x6C, { "BIT_5_h", [this]() { bit(m_gpr.h, 5); }, 2, 2 } },
        { 0x6D, { "BIT_5_l", [this]() { bit(m_gpr.l, 5); }, 2, 2 } },
        { 0x76, { "BIT_6_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 6); }, 2, 3 } },
        { 0x77, { "BIT_6_a", [this]() { bit(m_gpr.a, 6); }, 2, 2 } },
        { 0x70, { "BIT_6_b", [this]() { bit(m_gpr.b, 6); }, 2, 2 } },
        { 0x71, { "BIT_6_c", [this]() { bit(m_gpr.c, 6); }, 2, 2 } },
        { 0x72, { "BIT_6_d", [this]() { bit(m_gpr.d, 6); }, 2, 2 } },
        { 0x73, { "BIT_6_e", [this]() { bit(m_gpr.e, 6); }, 2, 2 } },
        { 0x74, { "BIT_6_h", [this]() { bit(m_gpr.h, 6); }, 2, 2 } },
        { 0x75, { "BIT_6_l", [this]() { bit(m_gpr.l, 6); }, 2, 2 } },
        { 0x7D, { "BIT_7_(hl)", [this]() { bit(m_memory.read(m_gpr.hl), 7); }, 2, 3 } },
        { 0x7F, { "BIT_7_a", [this]() { bit(m_gpr.a, 7); }, 2, 2 } },
        { 0x78, { "BIT_7_b", [this]() { bit(m_gpr.b, 7); }, 2, 2 } },
        { 0x79, { "BIT_7_c", [this]() { bit(m_gpr.c, 7); }, 2, 2 } },
        { 0x7A, { "BIT_7_d", [this]() { bit(m_gpr.d, 7); }, 2, 2 } },
        { 0x7B, { "BIT_7_e", [this]() { bit(m_gpr.e, 7); }, 2, 2 } },
        { 0x7C, { "BIT_7_h", [this]() { bit(m_gpr.h, 7); }, 2, 2 } },
    };
}

