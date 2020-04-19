/*
 * processor.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <iostream>
#include <cassert>
#include <sstream>
#include <vector>
#include <cstring>

#include "processor.h"
#include "memorycontroller.h"
#include "clockinterface.h"
#include "memmap.h"
#include "logging.h"

using std::string;
using std::stringstream;
using std::vector;

const uint8_t Processor::CB_PREFIX = 0xCB;

const uint16_t Processor::HISTORY_SIZE = 100;

const uint16_t Processor::ROM_ENTRY_POINT = 0x0100;

const uint8_t Processor::CYCLES_PER_TICK = 4;

void Processor::reset()
{
    m_pc = 0x0000;
    m_sp = 0xFFFE;

    m_flags = 0x00;

    m_gpr.a = 0x00;
    m_gpr.f = 0x00;
    m_gpr.b = 0x00;
    m_gpr.c = 0x00;
    m_gpr.d = 0x00;
    m_gpr.e = 0x00;
    m_gpr.h = 0x00;
    m_gpr.l = 0x00;

    m_interrupts.reset();
    m_timer.reset();
}

void Processor::history() const
{
    for (const GB::Command & cmd : m_executed) {
        cmd.print();
    }
}

vector<GB::Command> Processor::disassemble()
{
    vector<GB::Command> cmds;

    uint16_t pc = 0x100;
    while (pc < GPU_RAM_OFFSET) {
        GB::Command cmd;
        cmd.pc     = pc;
        cmd.opcode = m_memory.peek(pc++);

        if (ILLEGAL_OPCODES.find(cmd.opcode) != ILLEGAL_OPCODES.end()) {
            continue;
        }

        GB::Operation *operation = lookup(pc, cmd.opcode);
        if (!operation) { continue; }

        cmd.operation = operation;

        for (uint8_t i = 0; i < operation->length - 1; i++) {
            cmd.operands[i] = m_memory.peek(pc++);
        }

        cmds.push_back(cmd);
    }

    return cmds;
}

GB::Operation *Processor::lookup(uint16_t & pc, uint8_t opcode)
{
    // Figure out which table we should be looking in for the next opcode.
    auto & table = (CB_PREFIX == opcode) ? CB_OPCODES : OPCODES;

    // If this isn't the CB prefix, we don't really care about our prefix.  If it
    // is, then we need to grab the next by as that is our actual opcode, and then
    // save the prefix for debugging purposes.
    uint8_t prefix = 0x00;
    if (CB_PREFIX == opcode) {
        prefix = opcode;

        opcode = m_memory.peek(pc++);
    }

    // Lookup the opcode in the table that we just selected.  If we can't find it,
    // we are just going to print out a warning on debug builds and return a
    // nullptr to any upstream code.
    auto & entry = table[opcode];
    if (entry.name.empty()) {
        history();

        if (CB_PREFIX == prefix) {
            ERROR("Unknown opcode: 0x%04x - (0x%02x ==> 0x%02x)\n", m_instr, prefix, opcode);
        } else {
            ERROR("Unknown opcode: 0x%04x - (0x%02x)\n", m_instr, opcode);
        }
        return nullptr;
    }

    return &entry;
}

bool Processor::interrupt()
{
    if (!m_interrupts.enable && !m_halted) { return false; }

    InterruptVector vector = InterruptVector::INVALID;

    uint8_t status = m_interrupts.mask & m_interrupts.status;

    // Check our interrupt status to see if any of the interrupt bits are
    // set.  The if statements are in order of interrput priority.
    if (status & uint8_t(InterruptMask::VBLANK)) {
        vector = InterruptVector::VBLANK;
    } else if (status & uint8_t(InterruptMask::LCD)) {
        vector = InterruptVector::LCD;
    } else if (status & uint8_t(InterruptMask::TIMER)) {
        vector = InterruptVector::TIMER;
    } else if (status & uint8_t(InterruptMask::SERIAL)) {
        vector = InterruptVector::SERIAL;
    } else if (status & uint8_t(InterruptMask::JOYPAD)) {
        vector = InterruptVector::JOYPAD;
    }

    // If we found an interrupt that needs to be serviced, push the pc
    // on to the stack and then set the pc to the address of the interrupt
    // vector.
    if (InterruptVector::INVALID != vector) {
        if (m_interrupts.enable) {
            push(m_pc);

            m_pc = uint16_t(vector);

            // We are jumping to an ISR, so disable interrupts and clear the
            // flag in the interrupt status register.
            m_interrupts.status &= ~uint8_t(Interrupts::toMask(vector));
            m_interrupts.enable = false;
        }
        return true;
    }

    return false;
}

void Processor::tick(uint8_t ticks)
{
    uint8_t adjustment = CYCLES_PER_TICK >> uint8_t(m_timer.getSpeed());
    m_clock.tick(ticks * adjustment);
}

void Processor::execute(bool interrupted)
{
    // If we are halted, and we are not trying to execute an ISR, then all we need to
    // do is cycle the rest of the blocks and return.
    if (m_halted && !interrupted) { tick(1); return; }

    // If we've gotten to this point, then we were either never halted in the first
    // place, or we just executed an interrupt and were woken up.
    m_halted = false;

    // Make sure that our operands are always in a known state.  We can take advantage
    // of this later on during our command execution.
    m_operands[0] = m_operands[1] = 0x00;

    // The program counter points to our next opcode.
    m_instr = m_pc;

    // If our PC is set to the ROM's entry point, we need to swap out the BIOS memory
    // with the ROM memory because we would have already executed the BIOS if we've
    // gotten to this address.  This is a noop of the BIOS region is already disabled.
    if (ROM_ENTRY_POINT == m_pc) {
        NOTE("%s\n", "Starting ROM execution");
        m_memory.unlockBiosRegion();
    }

    uint8_t opcode = m_memory.peek(m_pc++);

    // Look up the handler in our opcode table.  If the length of our command is greater
    // than 1, then we also need to grab the next length - 1 bytes as they are the
    // arguments to the next operation that we are going to execute.
    GB::Operation *operation = lookup(m_pc, opcode);
    if (!operation) { FATAL("Unknown opcode: 0x%02x\n", opcode); }

    // If we have any operands, we need to read them in and increment the PC.
    for (uint8_t i = 0; i < operation->length - 1; i++) {
        m_operands[i] = m_memory.peek(m_pc++);
    }

#ifdef DEBUG
    log(opcode, operation);

    static bool trace = false;
    if (trace) {
        logRegisters();
    }
#endif

    // Let everything else sync with the clock before the CPU has a chance to modify
    // any memory.
    if (operation->cycles) { tick(operation->cycles); }

    // Call the function pointer in our Operation struct.  Any arguments to the function
    // will have already been put in to the operands array.
    operation->handler();

    // The flags register only uses the upper 4 bits, so let's go ahead and make sure
    // that the lower nibble is always 0.
    m_flags &= 0xF0;
}

void Processor::cycle()
{
    if (!m_memory.inBios() && !m_memory.isCartridgeValid()) { return; }

    // Cache the interrupt state so that we can log it and use it for debugging
    // if necessary.
    m_iCache.status = m_interrupts.status;
    m_iCache.mask   = m_interrupts.mask;

    execute(interrupt());
}

void Processor::log(uint8_t opcode, const GB::Operation *operation)
{
    if (m_memory.inBios()) { return; }

    if (m_executed.size() == HISTORY_SIZE) {
        m_executed.pop_front();
    }

    m_executed.push_back({
        m_instr,
        m_sp,
        opcode,
        m_flags,
        m_interrupts.enable,
        m_iCache.mask,
        m_iCache.status,
        m_gpr.a,
        m_gpr.bc,
        m_gpr.de,
        m_gpr.hl,
        m_memory.peek(GPU_SCANLINE_ADDRESS),
        m_memory.peek(GPU_STATUS_ADDRESS),
        m_memory.romBank(),
        m_memory.ramBank(),
        m_operands,
        operation
    });
}

void Processor::logRegisters() const
{
    if (!m_memory.inBios()) {
        m_executed.back().print();
    }
}

void Processor::xor8(uint8_t value)
{
    m_flags = 0x00;

    m_gpr.a ^= value;
    if (!m_gpr.a) { setZeroFlag(); }
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

void Processor::incP(uint16_t address)
{
    tick(2);
    uint8_t value = m_memory.peek(address);

    tick(1);

    inc(value);
    m_memory.write(address, value);
}

void Processor::inc(uint8_t & reg)
{
    const uint8_t mask = 0x0F;
    (mask == (reg & mask)) ? setHalfCarryFlag() : clrHalfCarryFlag();

    reg++;
    (!reg) ? setZeroFlag() : clrZeroFlag();

    clrNegFlag();
}

void Processor::decP(uint16_t address)
{
    tick(2);
    uint8_t value = m_memory.peek(address);

    tick(1);

    dec(value);
    m_memory.write(address, value);
}

void Processor::dec(uint8_t & reg)
{
    reg--;
    (!reg) ? setZeroFlag() : clrZeroFlag();

    const uint8_t mask = 0x0F;
    (mask == (reg & mask)) ? setHalfCarryFlag() : clrHalfCarryFlag();

    setNegFlag();
}

void Processor::add8(uint8_t value, bool carry)
{
    uint16_t adjust = (carry && isCarryFlagSet()) ? 0x01 : 0x00;

    m_flags = 0x00;

    uint16_t result = uint16_t(m_gpr.a) + uint16_t(value) + adjust;
    if (result & 0xFF00) { setCarryFlag(); }

    uint8_t half = (m_gpr.a & 0xF) + (value & 0xF) + adjust;
    if (half & 0xF0) { setHalfCarryFlag(); }

    m_gpr.a = result & 0xFF;
    if (!m_gpr.a) { setZeroFlag(); }
}

void Processor::sub8(uint8_t value, bool carry)
{
    uint8_t adjust = (carry && isCarryFlagSet()) ? 0x01 : 0x00;

    m_flags = 0x00;

    if (m_gpr.a < (value + adjust)) { setCarryFlag(); }

    if ((m_gpr.a & 0x0F) < ((value & 0x0F) + adjust)) {
        setHalfCarryFlag();
    }

    m_gpr.a -= (value + adjust);

    if (!m_gpr.a) { setZeroFlag(); }

    setNegFlag();
}

void Processor::sbc(uint8_t value)
{
    sub8(value, true);
}

void Processor::add16(uint16_t & reg, uint16_t value)
{
    clrNegFlag();

    uint32_t result = uint32_t(reg) + uint32_t(value);
    (result > 0xFFFF) ? setCarryFlag() : clrCarryFlag();

    constexpr const uint16_t mask = 0xFFF;
    if ((reg & mask) + (value & mask) > mask) {
        setHalfCarryFlag();
    } else {
        clrHalfCarryFlag();
    }

    reg = result & 0xFFFF;
}

void Processor::add16(uint16_t & dest, uint16_t reg, uint8_t value)
{
    m_flags = 0x00;

    union { int8_t sVal; uint8_t uVal; } input = { .uVal = value };

    constexpr const uint16_t cMask = 0xFF;
    if ((reg & cMask) + (input.sVal & cMask) > cMask) {
        setCarryFlag();
    } else {
        clrCarryFlag();
    }

    constexpr const uint16_t hMask = 0xF;
    if ((reg & hMask) + (input.sVal & hMask) > hMask) {
        setHalfCarryFlag();
    } else {
        clrHalfCarryFlag();
    }

    dest = reg + input.sVal;
}

void Processor::swap(uint16_t address)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    swap(value);
    m_memory.write(address, value);
}

void Processor::swap(uint8_t & reg)
{
    m_flags = 0x00;

    uint8_t upper = (reg >> 4) & 0x0F;
    uint8_t lower = reg & 0x0F;

    reg = (lower << 4) | upper;

    if (!reg) { setZeroFlag(); }
}

void Processor::call()
{
    push(m_pc);

    m_pc = args();

    tick(3);
}

void Processor::jump()
{
    jump(args());
}

void Processor::jumprel()
{
    union { uint8_t uVal; int8_t sVal; } temp = { .uVal = m_operands[0] };

    uint16_t address = m_pc + temp.sVal;
    jump(address);
}

void Processor::jump(uint16_t address)
{
    m_pc = address;

    tick(1);
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

    if (m_gpr.a == value) { setZeroFlag();  }
    if (m_gpr.a < value)  { setCarryFlag(); }

    const uint8_t mask = 0xF;
    if (((m_gpr.a - value) & mask) > (m_gpr.a & mask)) {
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

    setNegFlag();
    setHalfCarryFlag();
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

void Processor::res(uint16_t address, uint8_t which)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    res(value, which);
    m_memory.write(address, value);
}

void Processor::res(uint8_t & reg, uint8_t which)
{
    reg &= ~(1 << which);
}

void Processor::set(uint16_t address, uint8_t which)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    set(value, which);
    m_memory.write(address, value);
}

void Processor::set(uint8_t & reg, uint8_t which)
{
    reg |= (1 << which);
}

void Processor::pop(uint16_t & reg)
{
    uint8_t lower = m_memory.peek(m_sp++);
    uint8_t upper = m_memory.peek(m_sp++);

    reg = (upper << 8) | lower;
}

void Processor::push(uint16_t value)
{
    uint8_t lower = value & 0xFF;
    uint8_t upper = (value >> 8) & 0xFF;

    m_memory.write(--m_sp, upper);
    m_memory.write(--m_sp, lower);
}

void Processor::ret(bool enable)
{
    pop(m_pc);

    if (enable) {
        m_interrupts.enable = true;
    }

    tick(3);
}

void Processor::rotatel(uint16_t address, bool wrap, bool ignoreZero)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    rotatel(value, wrap, ignoreZero);
    m_memory.write(address, value);
}

void Processor::rotatel(uint8_t & reg, bool wrap, bool ignoreZero)
{
    uint8_t lsb = (isCarryFlagSet()) ? 0x01 : 0x00;

    (reg & 0x80) ? setCarryFlag() : clrCarryFlag();

    reg <<= 1;
    if (wrap) { reg |= lsb; }

    if (!ignoreZero) {
        (!reg) ? setZeroFlag() : clrZeroFlag();
    } else {
        clrZeroFlag();
    }

    clrNegFlag();
    clrHalfCarryFlag();
}

void Processor::rotater(uint16_t address, bool wrap, bool ignoreZero)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    rotater(value, wrap, ignoreZero);
    m_memory.write(address, value);
}

void Processor::rotater(uint8_t & reg, bool wrap, bool ignoreZero)
{
    uint8_t msb = (isCarryFlagSet()) ? 0x80 : 0x00;

    (reg & 0x01) ? setCarryFlag() : clrCarryFlag();

    reg >>= 1;
    if (wrap) { reg |= msb; }

    if (!ignoreZero) {
        (!reg) ? setZeroFlag() : clrZeroFlag();
    } else {
        clrZeroFlag();
    }

    clrNegFlag();
    clrHalfCarryFlag();
}

void Processor::rlc(uint16_t address, bool ignoreZero)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    rlc(value, ignoreZero);
    m_memory.write(address, value);
}

void Processor::rlc(uint8_t & reg, bool ignoreZero)
{
    uint8_t lsb = (reg & 0x80) ? 0x01 : 0x00;

    (lsb) ? setCarryFlag() : clrCarryFlag();

    reg = (reg << 1) | lsb;

    if (!ignoreZero) {
        (!reg) ? setZeroFlag() : clrZeroFlag();
    } else {
        clrZeroFlag();
    }

    clrNegFlag();
    clrHalfCarryFlag();
}

void Processor::rrc(uint16_t address, bool ignoreZero)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    rrc(value, ignoreZero);
    m_memory.write(address, value);
}

void Processor::rrc(uint8_t & reg, bool ignoreZero)
{
    uint8_t msb = (reg & 0x01) ? 0x80 : 0x00;

    (msb) ? setCarryFlag() : clrCarryFlag();

    reg = (reg >> 1) | msb;

    if (!ignoreZero) {
        (!reg) ? setZeroFlag() : clrZeroFlag();
    } else {
        clrZeroFlag();
    }

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
    loadMem(args(), value);
}

void Processor::loadMem(uint16_t ptr, uint8_t value)
{
    m_memory.write(ptr, value);
}

void Processor::loadMem(uint16_t value)
{
    loadMem(args(), value);
}

void Processor::loadMem(uint16_t ptr, uint16_t value)
{
    uint8_t lower = value & 0xFF;
    uint8_t upper = (value >> 8) & 0xFF;

    m_memory.write(ptr, lower);
    m_memory.write(ptr + 1, upper);
}

void Processor::load(uint16_t & reg)
{
    load(reg, args());
}
void Processor::load(uint16_t & reg, uint16_t value)
{
    reg = value;
}

void Processor::sla(uint16_t address)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    sla(value);
    m_memory.write(address, value);
}

void Processor::sla(uint8_t & reg)
{
    rotatel(reg, false, false);
}

void Processor::srl(uint16_t address)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    srl(value);
    m_memory.write(address, value);
}

void Processor::srl(uint8_t & reg)
{
    rotater(reg, false, false);
}

void Processor::sra(uint16_t address)
{
    tick(3);
    uint8_t value = m_memory.peek(address);

    tick(1);

    sra(value);
    m_memory.write(address, value);
}

void Processor::sra(uint8_t & reg)
{
    m_flags = 0x00;

    if (reg & 0x01) { setCarryFlag(); }

    uint8_t msb = reg & 0x80;
    reg = (reg >> 1) | msb;

    if (!reg) { setZeroFlag(); }
}

void Processor::scf()
{
    setCarryFlag();

    clrHalfCarryFlag();
    clrNegFlag();
}

void Processor::stop()
{
    if (!m_memory.isCGB()) { return; }

    // Check the speed switch register.  If the lowest bit is clear, then stop
    // was called without requesting a speed switch change, so we'll just bail
    // now.
    uint8_t current = m_memory.read(CGB_SPEED_SWITCH_ADDRESS);
    if (!(current & 0x01)) { return; }

    TimerModule::ClockSpeed speed =
        (m_timer.getSpeed() == TimerModule::SPEED_NORMAL)
        ? TimerModule::SPEED_DOUBLE : TimerModule::SPEED_NORMAL;

    // Update the MSB of the speed switch register and clear the LSB.
    m_memory.write(CGB_SPEED_SWITCH_ADDRESS, uint8_t(speed) << 7);

    m_timer.setSpeed(speed);
}

void Processor::daa()
{
    uint16_t value = m_gpr.a;

    if (isNegFlagSet()) {
        if (isHalfCarryFlagSet()) { value = (value - 0x06) & 0xFF; }
        if (isCarryFlagSet())     { value -= 0x60;                 }
    } else {
        if (isHalfCarryFlagSet() || ((value & 0xF) > 0x09)) { value += 0x06; }
        if (isCarryFlagSet() || (value > 0x9F))             { value += 0x60; }
    }

    clrHalfCarryFlag();
    if (value >= 0x100) { setCarryFlag(); }

    m_gpr.a = value;
    if (m_gpr.a) { clrZeroFlag(); }
    else         { setZeroFlag(); }
}
