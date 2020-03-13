#include "processor.h"
#include "memmap.h"
#include "memorycontroller.h"
#include "logging.h"

Processor::Processor(MemoryController & memory)
    : m_memory(memory),
      m_ticks(1),
      m_interrupts(m_memory.read(INTERRUPT_MASK_ADDRESS), m_memory.read(INTERRUPT_FLAGS_ADDRESS)),
      m_halted(false),
      m_timer(memory),
      m_flags(m_gpr.f)
{
    reset();

    //  opcode ==> { name, lambda function, length, cycles } }
    OPCODES[0x00] = { "NOP", [this]() { (void)this; }, 1, 1 };

    OPCODES[0xCE] = { "ADC_a_n",  [this]() { add8(m_operands[0], true); }, 2, 2 };
    OPCODES[0x8F] = { "ADC_a_a",  [this]() { add8(m_gpr.a, true);       }, 1, 1 };
    OPCODES[0x88] = { "ADC_a_b",  [this]() { add8(m_gpr.b, true);       }, 1, 1 };
    OPCODES[0x89] = { "ADC_a_c",  [this]() { add8(m_gpr.c, true);       }, 1, 1 };
    OPCODES[0x8A] = { "ADC_a_d",  [this]() { add8(m_gpr.d, true);       }, 1, 1 };
    OPCODES[0x8B] = { "ADC_a_e",  [this]() { add8(m_gpr.e, true);       }, 1, 1 };
    OPCODES[0x8C] = { "ADC_a_h",  [this]() { add8(m_gpr.h, true);       }, 1, 1 };
    OPCODES[0x8D] = { "ADC_a_l",  [this]() { add8(m_gpr.l, true);       }, 1, 1 };

    OPCODES[0x8E] = { "ADC_a_(hl)", [this]() { add8(m_memory.peek(m_gpr.hl), true); }, 1, 2 };

    OPCODES[0xC6] = { "ADD_a_n",  [this]() { add8(m_operands[0], false); }, 2, 2 };
    OPCODES[0x87] = { "ADD_a_a",  [this]() { add8(m_gpr.a, false);       }, 1, 1 };
    OPCODES[0x80] = { "ADD_a_b",  [this]() { add8(m_gpr.b, false);       }, 1, 1 };
    OPCODES[0x81] = { "ADD_a_c",  [this]() { add8(m_gpr.c, false);       }, 1, 1 };
    OPCODES[0x82] = { "ADD_a_d",  [this]() { add8(m_gpr.d, false);       }, 1, 1 };
    OPCODES[0x83] = { "ADD_a_e",  [this]() { add8(m_gpr.e, false);       }, 1, 1 };
    OPCODES[0x84] = { "ADD_a_h",  [this]() { add8(m_gpr.h, false);       }, 1, 1 };
    OPCODES[0x85] = { "ADD_a_l",  [this]() { add8(m_gpr.l, false);       }, 1, 1 };

    OPCODES[0x86] = { "ADD_a_(hl)", [this]() { add8(m_memory.peek(m_gpr.hl), false); }, 1, 2 };

    OPCODES[0x09] = { "ADD_hl_bc", [this]() { add16(m_gpr.hl, m_gpr.bc); }, 1, 2 };
    OPCODES[0x19] = { "ADD_hl_de", [this]() { add16(m_gpr.hl, m_gpr.de); }, 1, 2 };
    OPCODES[0x29] = { "ADD_hl_hl", [this]() { add16(m_gpr.hl, m_gpr.hl); }, 1, 2 };
    OPCODES[0x39] = { "ADD_hl_sp", [this]() { add16(m_gpr.hl, m_sp);     }, 1, 2 };

    OPCODES[0xE8] = { "ADD_sp_n", [this]() { add16(m_sp, m_sp, m_operands[0]); }, 2, 4 };

    OPCODES[0xE6] = { "AND_a_n",  [this]() { and8(m_operands[0]); }, 2, 2 };
    OPCODES[0xA7] = { "AND_a_a",  [this]() { and8(m_gpr.a);       }, 1, 1 };
    OPCODES[0xA0] = { "AND_a_b",  [this]() { and8(m_gpr.b);       }, 1, 1 };
    OPCODES[0xA1] = { "AND_a_c",  [this]() { and8(m_gpr.c);       }, 1, 1 };
    OPCODES[0xA2] = { "AND_a_d",  [this]() { and8(m_gpr.d);       }, 1, 1 };
    OPCODES[0xA3] = { "AND_a_e",  [this]() { and8(m_gpr.e);       }, 1, 1 };
    OPCODES[0xA4] = { "AND_a_h",  [this]() { and8(m_gpr.h);       }, 1, 1 };
    OPCODES[0xA5] = { "AND_a_l",  [this]() { and8(m_gpr.l);       }, 1, 1 };

    OPCODES[0xA6] = { "AND_a_(hl)", [this]() { and8(m_memory.peek(m_gpr.hl)); }, 1, 2 };

    OPCODES[0xCD] = { "CALL",    [this]() { call();                           }, 3, 3 };
    OPCODES[0xDC] = { "CALL_C",  [this]() { if (isCarryFlagSet())  { call(); }}, 3, 3 };
    OPCODES[0xD4] = { "CALL_NC", [this]() { if (!isCarryFlagSet()) { call(); }}, 3, 3 };
    OPCODES[0xCC] = { "CALL_Z",  [this]() { if (isZeroFlagSet())   { call(); }}, 3, 3 };
    OPCODES[0xC4] = { "CALL_NZ", [this]() { if (!isZeroFlagSet())  { call(); }}, 3, 3 };

    OPCODES[0x3F] = { "CCF", [this]() { ccf(); }, 1, 1 };

    OPCODES[0xFE] = { "CP_n",  [this]() { compare(m_operands[0]); }, 2, 2 };
    OPCODES[0xBF] = { "CP_a",  [this]() { compare(m_gpr.a);       }, 1, 1 };
    OPCODES[0xB8] = { "CP_b",  [this]() { compare(m_gpr.b);       }, 1, 1 };
    OPCODES[0xB9] = { "CP_c",  [this]() { compare(m_gpr.c);       }, 1, 1 };
    OPCODES[0xBA] = { "CP_d",  [this]() { compare(m_gpr.d);       }, 1, 1 };
    OPCODES[0xBB] = { "CP_e",  [this]() { compare(m_gpr.e);       }, 1, 1 };
    OPCODES[0xBC] = { "CP_h",  [this]() { compare(m_gpr.h);       }, 1, 1 };
    OPCODES[0xBD] = { "CP_l",  [this]() { compare(m_gpr.l);       }, 1, 1 };

    OPCODES[0xBE] = { "CP_(hl)", [this]() { compare(m_memory.peek(m_gpr.hl)); }, 1, 2 };

    OPCODES[0x2F] = { "CPL", [this]() { compliment(); }, 1, 1 };

    OPCODES[0x27] = { "DAA", [this]() { daa(); }, 1, 1 };

    OPCODES[0x35] = { "DEC_(hl)", [this]() { decP(m_gpr.hl); }, 1, 3 };
    OPCODES[0x3D] = { "DEC_a",    [this]() { dec(m_gpr.a);   }, 1, 1 };
    OPCODES[0x05] = { "DEC_b",    [this]() { dec(m_gpr.b);   }, 1, 1 };
    OPCODES[0x0B] = { "DEC_bc",   [this]() { dec(m_gpr.bc);  }, 1, 2 };
    OPCODES[0x0D] = { "DEC_c",    [this]() { dec(m_gpr.c);   }, 1, 1 };
    OPCODES[0x15] = { "DEC_d",    [this]() { dec(m_gpr.d);   }, 1, 1 };
    OPCODES[0x1B] = { "DEC_de",   [this]() { dec(m_gpr.de);  }, 1, 2 };
    OPCODES[0x1D] = { "DEC_e",    [this]() { dec(m_gpr.e);   }, 1, 1 };
    OPCODES[0x25] = { "DEC_h",    [this]() { dec(m_gpr.h);   }, 1, 1 };
    OPCODES[0x2B] = { "DEC_hl",   [this]() { dec(m_gpr.hl);  }, 1, 2 };
    OPCODES[0x2D] = { "DEC_l",    [this]() { dec(m_gpr.l);   }, 1, 1 };
    OPCODES[0x3B] = { "DEC_sp",   [this]() { dec(m_sp);      }, 1, 2 };

    OPCODES[0xF3] = { "DI",   [this]() { m_interrupts.enable = false; }, 1, 1 };
    OPCODES[0xFB] = { "EI",   [this]() { m_interrupts.enable = true;  }, 1, 1 };
    OPCODES[0x76] = { "HALT", [this]() { m_halted = true;             }, 1, 1 };

    OPCODES[0x34] = { "INC_(hl)", [this]() { incP(m_gpr.hl); }, 1, 3 };
    OPCODES[0x3C] = { "INC_a",    [this]() { inc(m_gpr.a);   }, 1, 1 };
    OPCODES[0x04] = { "INC_b",    [this]() { inc(m_gpr.b);   }, 1, 1 };
    OPCODES[0x03] = { "INC_bc",   [this]() { inc(m_gpr.bc);  }, 1, 2 };
    OPCODES[0x0C] = { "INC_c",    [this]() { inc(m_gpr.c);   }, 1, 1 };
    OPCODES[0x14] = { "INC_d",    [this]() { inc(m_gpr.d);   }, 1, 1 };
    OPCODES[0x13] = { "INC_de",   [this]() { inc(m_gpr.de);  }, 1, 2 };
    OPCODES[0x1C] = { "INC_e",    [this]() { inc(m_gpr.e);   }, 1, 1 };
    OPCODES[0x24] = { "INC_h",    [this]() { inc(m_gpr.h);   }, 1, 1 };
    OPCODES[0x23] = { "INC_hl",   [this]() { inc(m_gpr.hl);  }, 1, 2 };
    OPCODES[0x2C] = { "INC_l",    [this]() { inc(m_gpr.l);   }, 1, 1 };
    OPCODES[0x33] = { "INC_sp",   [this]() { inc(m_sp);      }, 1, 2 };

    OPCODES[0xC3] = { "JP_n",  [this]() { jump();         }, 3, 3 };
    OPCODES[0xE9] = { "JP_hl", [this]() { jump(m_gpr.hl); }, 1, 0 };

    OPCODES[0xDA] = { "JP_C",  [this]() { if (isCarryFlagSet())  { jump(); }}, 3, 3 };
    OPCODES[0xD2] = { "JP_NC", [this]() { if (!isCarryFlagSet()) { jump(); }}, 3, 3 };
    OPCODES[0xC2] = { "JP_NZ", [this]() { if (!isZeroFlagSet())  { jump(); }}, 3, 3 };
    OPCODES[0xCA] = { "JP_Z",  [this]() { if (isZeroFlagSet())   { jump(); }}, 3, 3 };

    OPCODES[0x18] = { "JR_n",  [this]() { jumprel();                           }, 2, 2 };
    OPCODES[0x38] = { "JR_C",  [this]() { if (isCarryFlagSet())  { jumprel(); }}, 2, 2 };
    OPCODES[0x30] = { "JR_NC", [this]() { if (!isCarryFlagSet()) { jumprel(); }}, 2, 2 };
    OPCODES[0x20] = { "JR_NZ", [this]() { if (!isZeroFlagSet())  { jumprel(); }}, 2, 2 };
    OPCODES[0x28] = { "JR_Z",  [this]() { if (isZeroFlagSet())   { jumprel(); }}, 2, 2 };

    OPCODES[0x32] = { "LDD_(hl)_a", [this]() { loadMem(m_gpr.hl--, m_gpr.a); }, 1, 2 };
    OPCODES[0x22] = { "LDI_(hl)_a", [this]() { loadMem(m_gpr.hl++, m_gpr.a); }, 1, 2 };

    OPCODES[0xEA] = { "LD_(nn)_a",  [this]() { loadMem(m_gpr.a); }, 3, 4 };
    OPCODES[0x08] = { "LD_(nn)_sp", [this]() { loadMem(m_sp);    }, 3, 5 };
    OPCODES[0xE0] = { "LD_(n)_a",   [this]() { loadMem(uint16_t(0xFF00 + m_operands[0]), m_gpr.a); }, 2, 3 };
    OPCODES[0x02] = { "LD_(bc)_a",  [this]() { loadMem(m_gpr.bc, m_gpr.a); }, 1, 2 };
    OPCODES[0xE2] = { "LD_(c)_a",   [this]() { loadMem(uint16_t(0xFF00 + m_gpr.c), m_gpr.a); }, 1, 2 };
    OPCODES[0x12] = { "LD_(de)_a",  [this]() { loadMem(m_gpr.de, m_gpr.a); }, 1, 2 };

    OPCODES[0xFA] = { "LD_a_(nn)",  [this]() { m_gpr.a = m_memory.peek(args());           }, 3, 4 };
    OPCODES[0x0A] = { "LD_a_(bc)",  [this]() { m_gpr.a = m_memory.peek(m_gpr.bc);         }, 1, 2 };
    OPCODES[0xF2] = { "LD_a_(c)",   [this]() { m_gpr.a = m_memory.peek(0xFF00 + m_gpr.c); }, 1, 2 };
    OPCODES[0x1A] = { "LD_a_(de)",  [this]() { m_gpr.a = m_memory.peek(m_gpr.de);         }, 1, 2 };
    OPCODES[0x3A] = { "LDD_a_(hl)", [this]() { m_gpr.a = m_memory.peek(m_gpr.hl--);       }, 1, 2 };
    OPCODES[0x2A] = { "LDI_a_(hl)", [this]() { m_gpr.a = m_memory.peek(m_gpr.hl++);       }, 1, 2 };

    OPCODES[0xF0] = { "LDH_a_(n)", [this]() { m_gpr.a = m_memory.peek(0xFF00 + m_operands[0]); }, 2, 3};

    OPCODES[0x3E] = { "LD_a_n",   [this]() { m_gpr.a = m_operands[0]; }, 2, 2 };
    OPCODES[0x7E] = { "LD_a_(hl)", [this]() { m_gpr.a = m_memory.peek(m_gpr.hl); }, 1, 2 };
    OPCODES[0x7F] = { "LD_a_a",   [this]() { m_gpr.a = m_gpr.a;       }, 1, 1 };
    OPCODES[0x78] = { "LD_a_b",   [this]() { m_gpr.a = m_gpr.b;       }, 1, 1 };
    OPCODES[0x79] = { "LD_a_c",   [this]() { m_gpr.a = m_gpr.c;       }, 1, 1 };
    OPCODES[0x7A] = { "LD_a_d",   [this]() { m_gpr.a = m_gpr.d;       }, 1, 1 };
    OPCODES[0x7B] = { "LD_a_e",   [this]() { m_gpr.a = m_gpr.e;       }, 1, 1 };
    OPCODES[0x7C] = { "LD_a_h",   [this]() { m_gpr.a = m_gpr.h;       }, 1, 1 };
    OPCODES[0x7D] = { "LD_a_l",   [this]() { m_gpr.a = m_gpr.l;       }, 1, 1 };
    OPCODES[0x06] = { "LD_b_n",   [this]() { m_gpr.b = m_operands[0]; }, 2, 2 };
    OPCODES[0x46] = { "LD_b_(hl)", [this]() { m_gpr.b = m_memory.peek(m_gpr.hl); }, 1, 2 };
    OPCODES[0x47] = { "LD_b_a",   [this]() { m_gpr.b = m_gpr.a;       }, 1, 1 };
    OPCODES[0x40] = { "LD_b_b",   [this]() { m_gpr.b = m_gpr.b;       }, 1, 1 };
    OPCODES[0x41] = { "LD_b_c",   [this]() { m_gpr.b = m_gpr.c;       }, 1, 1 };
    OPCODES[0x42] = { "LD_b_d",   [this]() { m_gpr.b = m_gpr.d;       }, 1, 1 };
    OPCODES[0x43] = { "LD_b_e",   [this]() { m_gpr.b = m_gpr.e;       }, 1, 1 };
    OPCODES[0x44] = { "LD_b_h",   [this]() { m_gpr.b = m_gpr.h;       }, 1, 1 };
    OPCODES[0x45] = { "LD_b_l",   [this]() { m_gpr.b = m_gpr.l;       }, 1, 1 };
    OPCODES[0x01] = { "LD_bc_nn", [this]() { m_gpr.bc = args();       }, 3, 3 };
    OPCODES[0x0E] = { "LD_c_n",   [this]() { m_gpr.c = m_operands[0]; }, 2, 2 };
    OPCODES[0x4E] = { "LD_c_(hl)", [this]() { m_gpr.c = m_memory.peek(m_gpr.hl); }, 1, 2 };
    OPCODES[0x4F] = { "LD_c_a",   [this]() { m_gpr.c = m_gpr.a;       }, 1, 1 };
    OPCODES[0x48] = { "LD_c_b",   [this]() { m_gpr.c = m_gpr.b;       }, 1, 1 };
    OPCODES[0x49] = { "LD_c_c",   [this]() { m_gpr.c = m_gpr.c;       }, 1, 1 };
    OPCODES[0x4A] = { "LD_c_d",   [this]() { m_gpr.c = m_gpr.d;       }, 1, 1 };
    OPCODES[0x4B] = { "LD_c_e",   [this]() { m_gpr.c = m_gpr.e;       }, 1, 1 };
    OPCODES[0x4C] = { "LD_c_h",   [this]() { m_gpr.c = m_gpr.h;       }, 1, 1 };
    OPCODES[0x4D] = { "LD_c_l",   [this]() { m_gpr.c = m_gpr.l;       }, 1, 1 };
    OPCODES[0x16] = { "LD_d_n",   [this]() { m_gpr.d = m_operands[0]; }, 2, 2 };
    OPCODES[0x56] = { "LD_d_(hl)", [this]() { m_gpr.d = m_memory.peek(m_gpr.hl); }, 1, 2 };
    OPCODES[0x57] = { "LD_d_a",   [this]() { m_gpr.d = m_gpr.a;       }, 1, 1 };
    OPCODES[0x50] = { "LD_d_b",   [this]() { m_gpr.d = m_gpr.b;       }, 1, 1 };
    OPCODES[0x51] = { "LD_d_c",   [this]() { m_gpr.d = m_gpr.c;       }, 1, 1 };
    OPCODES[0x52] = { "LD_d_d",   [this]() { m_gpr.d = m_gpr.d;       }, 1, 1 };
    OPCODES[0x53] = { "LD_d_e",   [this]() { m_gpr.d = m_gpr.e;       }, 1, 1 };
    OPCODES[0x54] = { "LD_d_h",   [this]() { m_gpr.d = m_gpr.h;       }, 1, 1 };
    OPCODES[0x55] = { "LD_d_l",   [this]() { m_gpr.d = m_gpr.l;       }, 1, 1 };
    OPCODES[0x11] = { "LD_de_nn", [this]() { m_gpr.de = args();       }, 3, 3 };
    OPCODES[0x1E] = { "LD_e_n",   [this]() { m_gpr.e = m_operands[0]; }, 2, 2 };
    OPCODES[0x5E] = { "LD_e_(hl)", [this]() { m_gpr.e = m_memory.peek(m_gpr.hl); }, 1, 2 };
    OPCODES[0x5F] = { "LD_e_a",   [this]() { m_gpr.e = m_gpr.a;       }, 1, 1 };
    OPCODES[0x58] = { "LD_e_b",   [this]() { m_gpr.e = m_gpr.b;       }, 1, 1 };
    OPCODES[0x59] = { "LD_e_c",   [this]() { m_gpr.e = m_gpr.c;       }, 1, 1 };
    OPCODES[0x5A] = { "LD_e_d",   [this]() { m_gpr.e = m_gpr.d;       }, 1, 1 };
    OPCODES[0x5B] = { "LD_e_e",   [this]() { m_gpr.e = m_gpr.e;       }, 1, 1 };
    OPCODES[0x5C] = { "LD_e_h",   [this]() { m_gpr.e = m_gpr.h;       }, 1, 1 };
    OPCODES[0x5D] = { "LD_e_l",   [this]() { m_gpr.e = m_gpr.l;       }, 1, 1 };
    OPCODES[0x26] = { "LD_h_n",   [this]() { m_gpr.h = m_operands[0]; }, 2, 2 };
    OPCODES[0x66] = { "LD_h_(hl)", [this]() { m_gpr.h = m_memory.peek(m_gpr.hl); }, 1, 2 };
    OPCODES[0x67] = { "LD_h_a",   [this]() { m_gpr.h = m_gpr.a;       }, 1, 1 };
    OPCODES[0x60] = { "LD_h_b",   [this]() { m_gpr.h = m_gpr.b;       }, 1, 1 };
    OPCODES[0x61] = { "LD_h_c",   [this]() { m_gpr.h = m_gpr.c;       }, 1, 1 };
    OPCODES[0x62] = { "LD_h_d",   [this]() { m_gpr.h = m_gpr.d;       }, 1, 1 };
    OPCODES[0x63] = { "LD_h_e",   [this]() { m_gpr.h = m_gpr.e;       }, 1, 1 };
    OPCODES[0x64] = { "LD_h_h",   [this]() { m_gpr.h = m_gpr.h;       }, 1, 1 };
    OPCODES[0x65] = { "LD_h_l",   [this]() { m_gpr.h = m_gpr.l;       }, 1, 1 };
    OPCODES[0x2E] = { "LD_l_n",   [this]() { m_gpr.l = m_operands[0]; }, 2, 2 };
    OPCODES[0x6E] = { "LD_l_(hl)", [this]() { m_gpr.l = m_memory.peek(m_gpr.hl); }, 1, 2 };
    OPCODES[0x6F] = { "LD_l_a",   [this]() { m_gpr.l = m_gpr.a;       }, 1, 1 };
    OPCODES[0x68] = { "LD_l_b",   [this]() { m_gpr.l = m_gpr.b;       }, 1, 1 };
    OPCODES[0x69] = { "LD_l_c",   [this]() { m_gpr.l = m_gpr.c;       }, 1, 1 };
    OPCODES[0x6A] = { "LD_l_d",   [this]() { m_gpr.l = m_gpr.d;       }, 1, 1 };
    OPCODES[0x6B] = { "LD_l_e",   [this]() { m_gpr.l = m_gpr.e;       }, 1, 1 };
    OPCODES[0x6C] = { "LD_l_h",   [this]() { m_gpr.l = m_gpr.h;       }, 1, 1 };
    OPCODES[0x6D] = { "LD_l_l",   [this]() { m_gpr.l = m_gpr.l;       }, 1, 1 };

    OPCODES[0x36] = { "LD_(hl)_n", [this]() { loadMem(m_gpr.hl, m_operands[0]); }, 2, 3 };
    OPCODES[0x77] = { "LD_(hl)_a", [this]() { loadMem(m_gpr.hl, m_gpr.a);       }, 1, 2 };
    OPCODES[0x70] = { "LD_(hl)_b", [this]() { loadMem(m_gpr.hl, m_gpr.b);       }, 1, 2 };
    OPCODES[0x71] = { "LD_(hl)_c", [this]() { loadMem(m_gpr.hl, m_gpr.c);       }, 1, 2 };
    OPCODES[0x72] = { "LD_(hl)_d", [this]() { loadMem(m_gpr.hl, m_gpr.d);       }, 1, 2 };
    OPCODES[0x73] = { "LD_(hl)_e", [this]() { loadMem(m_gpr.hl, m_gpr.e);       }, 1, 2 };
    OPCODES[0x74] = { "LD_(hl)_h", [this]() { loadMem(m_gpr.hl, m_gpr.h);       }, 1, 2 };
    OPCODES[0x75] = { "LD_(hl)_l", [this]() { loadMem(m_gpr.hl, m_gpr.l);       }, 1, 2 };

    OPCODES[0x21] = { "LD_hl_nn", [this]() { load(m_gpr.hl);                       }, 3, 3 };
    OPCODES[0x31] = { "LD_sp_nn", [this]() { load(m_sp);                           }, 3, 3 };
    OPCODES[0xF8] = { "LD_hl_sp", [this]() { add16(m_gpr.hl, m_sp, m_operands[0]); }, 2, 3 };
    OPCODES[0xF9] = { "LD_sp_hl", [this]() { m_sp = m_gpr.hl;                      }, 1, 2 };

    OPCODES[0xF6] = { "OR_n",    [this]() { or8(m_operands[0]);           }, 2, 2 };
    OPCODES[0xB6] = { "OR_(hl)", [this]() { or8(m_memory.peek(m_gpr.hl)); }, 1, 2 };
    OPCODES[0xB7] = { "OR_a",    [this]() { or8(m_gpr.a);                 }, 1, 1 };
    OPCODES[0xB0] = { "OR_b",    [this]() { or8(m_gpr.b);                 }, 1, 1 };
    OPCODES[0xB1] = { "OR_c",    [this]() { or8(m_gpr.c);                 }, 1, 1 };
    OPCODES[0xB2] = { "OR_d",    [this]() { or8(m_gpr.d);                 }, 1, 1 };
    OPCODES[0xB3] = { "OR_e",    [this]() { or8(m_gpr.e);                 }, 1, 1 };
    OPCODES[0xB4] = { "OR_h",    [this]() { or8(m_gpr.h);                 }, 1, 1 };
    OPCODES[0xB5] = { "OR_l",    [this]() { or8(m_gpr.l);                 }, 1, 1 };

    OPCODES[0xF1] = { "POP_af", [this]() { pop(m_gpr.af); }, 1, 3 };
    OPCODES[0xC1] = { "POP_bc", [this]() { pop(m_gpr.bc); }, 1, 3 };
    OPCODES[0xD1] = { "POP_de", [this]() { pop(m_gpr.de); }, 1, 3 };
    OPCODES[0xE1] = { "POP_hl", [this]() { pop(m_gpr.hl); }, 1, 3 };

    OPCODES[0xF5] = { "PUSH_af", [this]() { push(m_gpr.af); }, 1, 4 };
    OPCODES[0xC5] = { "PUSH_bc", [this]() { push(m_gpr.bc); }, 1, 4 };
    OPCODES[0xD5] = { "PUSH_de", [this]() { push(m_gpr.de); }, 1, 4 };
    OPCODES[0xE5] = { "PUSH_hl", [this]() { push(m_gpr.hl); }, 1, 4 };

    OPCODES[0xC9] = { "RET",    [this]() { ret(false);                           }, 1, 1 };
    OPCODES[0xD8] = { "RET_C",  [this]() { if (isCarryFlagSet())  { ret(false); }}, 1, 2 };
    OPCODES[0xD0] = { "RET_NC", [this]() { if (!isCarryFlagSet()) { ret(false); }}, 1, 2 };
    OPCODES[0xC0] = { "RET_NZ", [this]() { if (!isZeroFlagSet())  { ret(false); }}, 1, 2 };
    OPCODES[0xC8] = { "RET_Z",  [this]() { if (isZeroFlagSet())   { ret(false); }}, 1, 2 };
    OPCODES[0xD9] = { "RETI",   [this]() { ret(true);                            }, 1, 1 };

    OPCODES[0x17] = { "RLA",  [this]() { rotatel(m_gpr.a, true, true); }, 1, 1 };
    OPCODES[0x07] = { "RLCA", [this]() { rlc(m_gpr.a, true); }, 1, 1 };
    OPCODES[0x1F] = { "RRA",  [this]() { rotater(m_gpr.a, true, true); }, 1, 1 };
    OPCODES[0x0F] = { "RRCA", [this]() { rrc(m_gpr.a, true); }, 1, 1 };

    OPCODES[0xC7] = { "RST_$00", [this]() { rst(0x00); }, 1, 4 };
    OPCODES[0xCF] = { "RST_$08", [this]() { rst(0x08); }, 1, 4 };
    OPCODES[0xD7] = { "RST_$10", [this]() { rst(0x10); }, 1, 4 };
    OPCODES[0xDF] = { "RST_$18", [this]() { rst(0x18); }, 1, 4 };
    OPCODES[0xE7] = { "RST_$20", [this]() { rst(0x20); }, 1, 4 };
    OPCODES[0xEF] = { "RST_$28", [this]() { rst(0x28); }, 1, 4 };
    OPCODES[0xF7] = { "RST_$30", [this]() { rst(0x30); }, 1, 4 };
    OPCODES[0xFF] = { "RST_$38", [this]() { rst(0x38); }, 1, 4 };

    OPCODES[0x37] = { "SCF", [this]() { scf(); }, 1, 1 };

    OPCODES[0x10] = { "STOP", [this]() { stop(); }, 1, 1 };

    OPCODES[0xDE] = { "SBC_a_n",  [this]() { sbc(m_operands[0]); }, 2, 2 };
    OPCODES[0x9F] = { "SBC_a_a",  [this]() { sbc(m_gpr.a);       }, 1, 1 };
    OPCODES[0x98] = { "SBC_a_b",  [this]() { sbc(m_gpr.b);       }, 1, 1 };
    OPCODES[0x99] = { "SBC_a_c",  [this]() { sbc(m_gpr.c);       }, 1, 1 };
    OPCODES[0x9A] = { "SBC_a_d",  [this]() { sbc(m_gpr.d);       }, 1, 1 };
    OPCODES[0x9B] = { "SBC_a_e",  [this]() { sbc(m_gpr.e);       }, 1, 1 };
    OPCODES[0x9C] = { "SBC_a_h",  [this]() { sbc(m_gpr.h);       }, 1, 1 };
    OPCODES[0x9D] = { "SBC_a_l",  [this]() { sbc(m_gpr.l);       }, 1, 1 };
    OPCODES[0x9E] = { "SBC_(hl)", [this]() { sbc(m_memory.peek(m_gpr.hl)); }, 1, 2 };

    OPCODES[0xD6] = { "SUB_a_n",  [this]() { sub8(m_operands[0], false); }, 2, 2 };
    OPCODES[0x97] = { "SUB_a_a",  [this]() { sub8(m_gpr.a, false);       }, 1, 1 };
    OPCODES[0x90] = { "SUB_a_b",  [this]() { sub8(m_gpr.b, false);       }, 1, 1 };
    OPCODES[0x91] = { "SUB_a_c",  [this]() { sub8(m_gpr.c, false);       }, 1, 1 };
    OPCODES[0x92] = { "SUB_a_d",  [this]() { sub8(m_gpr.d, false);       }, 1, 1 };
    OPCODES[0x93] = { "SUB_a_e",  [this]() { sub8(m_gpr.e, false);       }, 1, 1 };
    OPCODES[0x94] = { "SUB_a_h",  [this]() { sub8(m_gpr.h, false);       }, 1, 1 };
    OPCODES[0x95] = { "SUB_a_l",  [this]() { sub8(m_gpr.l, false);       }, 1, 1 };
    OPCODES[0x96] = { "SUB_(hl)", [this]() { sub8(m_memory.peek(m_gpr.hl), false); }, 1, 2 };

    OPCODES[0xEE] = { "XOR_a_n",  [this]() { xor8(m_operands[0]); }, 2, 2 };
    OPCODES[0xAF] = { "XOR_a_a",  [this]() { xor8(m_gpr.a);       }, 1, 1 };
    OPCODES[0xA8] = { "XOR_a_b",  [this]() { xor8(m_gpr.b);       }, 1, 1 };
    OPCODES[0xA9] = { "XOR_a_c",  [this]() { xor8(m_gpr.c);       }, 1, 1 };
    OPCODES[0xAA] = { "XOR_a_d",  [this]() { xor8(m_gpr.d);       }, 1, 1 };
    OPCODES[0xAB] = { "XOR_a_e",  [this]() { xor8(m_gpr.e);       }, 1, 1 };
    OPCODES[0xAC] = { "XOR_a_h",  [this]() { xor8(m_gpr.h);       }, 1, 1 };
    OPCODES[0xAD] = { "XOR_a_l",  [this]() { xor8(m_gpr.l);       }, 1, 1 };

    OPCODES[0xAE] = { "XOR_a_(hl)", [this]() { xor8(m_memory.peek(m_gpr.hl)); }, 1, 2 };


    CB_OPCODES[0x46] = { "BIT_0_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 0); }, 1, 3 };
    CB_OPCODES[0x47] = { "BIT_0_a", [this]() { bit(m_gpr.a, 0); }, 1, 2 };
    CB_OPCODES[0x40] = { "BIT_0_b", [this]() { bit(m_gpr.b, 0); }, 1, 2 };
    CB_OPCODES[0x41] = { "BIT_0_c", [this]() { bit(m_gpr.c, 0); }, 1, 2 };
    CB_OPCODES[0x42] = { "BIT_0_d", [this]() { bit(m_gpr.d, 0); }, 1, 2 };
    CB_OPCODES[0x43] = { "BIT_0_e", [this]() { bit(m_gpr.e, 0); }, 1, 2 };
    CB_OPCODES[0x44] = { "BIT_0_h", [this]() { bit(m_gpr.h, 0); }, 1, 2 };
    CB_OPCODES[0x45] = { "BIT_0_l", [this]() { bit(m_gpr.l, 0); }, 1, 2 };
    CB_OPCODES[0x4E] = { "BIT_1_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 1); }, 1, 3 };
    CB_OPCODES[0x4F] = { "BIT_1_a", [this]() { bit(m_gpr.a, 1); }, 1, 2 };
    CB_OPCODES[0x48] = { "BIT_1_b", [this]() { bit(m_gpr.b, 1); }, 1, 2 };
    CB_OPCODES[0x49] = { "BIT_1_c", [this]() { bit(m_gpr.c, 1); }, 1, 2 };
    CB_OPCODES[0x4A] = { "BIT_1_d", [this]() { bit(m_gpr.d, 1); }, 1, 2 };
    CB_OPCODES[0x4B] = { "BIT_1_e", [this]() { bit(m_gpr.e, 1); }, 1, 2 };
    CB_OPCODES[0x4C] = { "BIT_1_h", [this]() { bit(m_gpr.h, 1); }, 1, 2 };
    CB_OPCODES[0x4D] = { "BIT_1_l", [this]() { bit(m_gpr.l, 1); }, 1, 2 };
    CB_OPCODES[0x56] = { "BIT_2_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 2); }, 1, 3 };
    CB_OPCODES[0x57] = { "BIT_2_a", [this]() { bit(m_gpr.a, 2); }, 1, 2 };
    CB_OPCODES[0x50] = { "BIT_2_b", [this]() { bit(m_gpr.b, 2); }, 1, 2 };
    CB_OPCODES[0x51] = { "BIT_2_c", [this]() { bit(m_gpr.c, 2); }, 1, 2 };
    CB_OPCODES[0x52] = { "BIT_2_d", [this]() { bit(m_gpr.d, 2); }, 1, 2 };
    CB_OPCODES[0x53] = { "BIT_2_e", [this]() { bit(m_gpr.e, 2); }, 1, 2 };
    CB_OPCODES[0x54] = { "BIT_2_h", [this]() { bit(m_gpr.h, 2); }, 1, 2 };
    CB_OPCODES[0x55] = { "BIT_2_l", [this]() { bit(m_gpr.l, 2); }, 1, 2 };
    CB_OPCODES[0x5E] = { "BIT_3_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 3); }, 1, 3 };
    CB_OPCODES[0x5F] = { "BIT_3_a", [this]() { bit(m_gpr.a, 3); }, 1, 2 };
    CB_OPCODES[0x58] = { "BIT_3_b", [this]() { bit(m_gpr.b, 3); }, 1, 2 };
    CB_OPCODES[0x59] = { "BIT_3_c", [this]() { bit(m_gpr.c, 3); }, 1, 2 };
    CB_OPCODES[0x5A] = { "BIT_3_d", [this]() { bit(m_gpr.d, 3); }, 1, 2 };
    CB_OPCODES[0x5B] = { "BIT_3_e", [this]() { bit(m_gpr.e, 3); }, 1, 2 };
    CB_OPCODES[0x5C] = { "BIT_3_h", [this]() { bit(m_gpr.h, 3); }, 1, 2 };
    CB_OPCODES[0x5D] = { "BIT_3_l", [this]() { bit(m_gpr.l, 3); }, 1, 2 };
    CB_OPCODES[0x66] = { "BIT_4_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 4); }, 1, 3 };
    CB_OPCODES[0x67] = { "BIT_4_a", [this]() { bit(m_gpr.a, 4); }, 1, 2 };
    CB_OPCODES[0x60] = { "BIT_4_b", [this]() { bit(m_gpr.b, 4); }, 1, 2 };
    CB_OPCODES[0x61] = { "BIT_4_c", [this]() { bit(m_gpr.c, 4); }, 1, 2 };
    CB_OPCODES[0x62] = { "BIT_4_d", [this]() { bit(m_gpr.d, 4); }, 1, 2 };
    CB_OPCODES[0x63] = { "BIT_4_e", [this]() { bit(m_gpr.e, 4); }, 1, 2 };
    CB_OPCODES[0x64] = { "BIT_4_h", [this]() { bit(m_gpr.h, 4); }, 1, 2 };
    CB_OPCODES[0x65] = { "BIT_4_l", [this]() { bit(m_gpr.l, 4); }, 1, 2 };
    CB_OPCODES[0x6E] = { "BIT_5_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 5); }, 1, 3 };
    CB_OPCODES[0x6F] = { "BIT_5_a", [this]() { bit(m_gpr.a, 5); }, 1, 2 };
    CB_OPCODES[0x68] = { "BIT_5_b", [this]() { bit(m_gpr.b, 5); }, 1, 2 };
    CB_OPCODES[0x69] = { "BIT_5_c", [this]() { bit(m_gpr.c, 5); }, 1, 2 };
    CB_OPCODES[0x6A] = { "BIT_5_d", [this]() { bit(m_gpr.d, 5); }, 1, 2 };
    CB_OPCODES[0x6B] = { "BIT_5_e", [this]() { bit(m_gpr.e, 5); }, 1, 2 };
    CB_OPCODES[0x6C] = { "BIT_5_h", [this]() { bit(m_gpr.h, 5); }, 1, 2 };
    CB_OPCODES[0x6D] = { "BIT_5_l", [this]() { bit(m_gpr.l, 5); }, 1, 2 };
    CB_OPCODES[0x76] = { "BIT_6_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 6); }, 1, 3 };
    CB_OPCODES[0x77] = { "BIT_6_a", [this]() { bit(m_gpr.a, 6); }, 1, 2 };
    CB_OPCODES[0x70] = { "BIT_6_b", [this]() { bit(m_gpr.b, 6); }, 1, 2 };
    CB_OPCODES[0x71] = { "BIT_6_c", [this]() { bit(m_gpr.c, 6); }, 1, 2 };
    CB_OPCODES[0x72] = { "BIT_6_d", [this]() { bit(m_gpr.d, 6); }, 1, 2 };
    CB_OPCODES[0x73] = { "BIT_6_e", [this]() { bit(m_gpr.e, 6); }, 1, 2 };
    CB_OPCODES[0x74] = { "BIT_6_h", [this]() { bit(m_gpr.h, 6); }, 1, 2 };
    CB_OPCODES[0x75] = { "BIT_6_l", [this]() { bit(m_gpr.l, 6); }, 1, 2 };
    CB_OPCODES[0x7E] = { "BIT_7_(hl)", [this]() { bit(m_memory.peek(m_gpr.hl), 7); }, 1, 3 };
    CB_OPCODES[0x7F] = { "BIT_7_a", [this]() { bit(m_gpr.a, 7); }, 1, 2 };
    CB_OPCODES[0x78] = { "BIT_7_b", [this]() { bit(m_gpr.b, 7); }, 1, 2 };
    CB_OPCODES[0x79] = { "BIT_7_c", [this]() { bit(m_gpr.c, 7); }, 1, 2 };
    CB_OPCODES[0x7A] = { "BIT_7_d", [this]() { bit(m_gpr.d, 7); }, 1, 2 };
    CB_OPCODES[0x7B] = { "BIT_7_e", [this]() { bit(m_gpr.e, 7); }, 1, 2 };
    CB_OPCODES[0x7C] = { "BIT_7_h", [this]() { bit(m_gpr.h, 7); }, 1, 2 };
    CB_OPCODES[0x7D] = { "BIT_7_l", [this]() { bit(m_gpr.l, 7); }, 1, 2 };

    CB_OPCODES[0x86] = { "RES_0_(hl)", [this]() { res(m_gpr.hl, 0); }, 1, 4 };
    CB_OPCODES[0x87] = { "RES_0_a", [this]() { res(m_gpr.a, 0); }, 1, 2 };
    CB_OPCODES[0x80] = { "RES_0_b", [this]() { res(m_gpr.b, 0); }, 1, 2 };
    CB_OPCODES[0x81] = { "RES_0_c", [this]() { res(m_gpr.c, 0); }, 1, 2 };
    CB_OPCODES[0x82] = { "RES_0_d", [this]() { res(m_gpr.d, 0); }, 1, 2 };
    CB_OPCODES[0x83] = { "RES_0_e", [this]() { res(m_gpr.e, 0); }, 1, 2 };
    CB_OPCODES[0x84] = { "RES_0_h", [this]() { res(m_gpr.h, 0); }, 1, 2 };
    CB_OPCODES[0x85] = { "RES_0_l", [this]() { res(m_gpr.l, 0); }, 1, 2 };
    CB_OPCODES[0x8E] = { "RES_1_(hl)", [this]() { res(m_gpr.hl, 1); }, 1, 4 };
    CB_OPCODES[0x8F] = { "RES_1_a", [this]() { res(m_gpr.a, 1); }, 1, 2 };
    CB_OPCODES[0x88] = { "RES_1_b", [this]() { res(m_gpr.b, 1); }, 1, 2 };
    CB_OPCODES[0x89] = { "RES_1_c", [this]() { res(m_gpr.c, 1); }, 1, 2 };
    CB_OPCODES[0x8A] = { "RES_1_d", [this]() { res(m_gpr.d, 1); }, 1, 2 };
    CB_OPCODES[0x8B] = { "RES_1_e", [this]() { res(m_gpr.e, 1); }, 1, 2 };
    CB_OPCODES[0x8C] = { "RES_1_h", [this]() { res(m_gpr.h, 1); }, 1, 2 };
    CB_OPCODES[0x8D] = { "RES_1_l", [this]() { res(m_gpr.l, 1); }, 1, 2 };
    CB_OPCODES[0x96] = { "RES_2_(hl)", [this]() { res(m_gpr.hl, 2); }, 1, 4 };
    CB_OPCODES[0x97] = { "RES_2_a", [this]() { res(m_gpr.a, 2); }, 1, 2 };
    CB_OPCODES[0x90] = { "RES_2_b", [this]() { res(m_gpr.b, 2); }, 1, 2 };
    CB_OPCODES[0x91] = { "RES_2_c", [this]() { res(m_gpr.c, 2); }, 1, 2 };
    CB_OPCODES[0x92] = { "RES_2_d", [this]() { res(m_gpr.d, 2); }, 1, 2 };
    CB_OPCODES[0x93] = { "RES_2_e", [this]() { res(m_gpr.e, 2); }, 1, 2 };
    CB_OPCODES[0x94] = { "RES_2_h", [this]() { res(m_gpr.h, 2); }, 1, 2 };
    CB_OPCODES[0x95] = { "RES_2_l", [this]() { res(m_gpr.l, 2); }, 1, 2 };
    CB_OPCODES[0x9E] = { "RES_3_(hl)", [this]() { res(m_gpr.hl, 3); }, 1, 4 };
    CB_OPCODES[0x9F] = { "RES_3_a", [this]() { res(m_gpr.a, 3); }, 1, 2 };
    CB_OPCODES[0x98] = { "RES_3_b", [this]() { res(m_gpr.b, 3); }, 1, 2 };
    CB_OPCODES[0x99] = { "RES_3_c", [this]() { res(m_gpr.c, 3); }, 1, 2 };
    CB_OPCODES[0x9A] = { "RES_3_d", [this]() { res(m_gpr.d, 3); }, 1, 2 };
    CB_OPCODES[0x9B] = { "RES_3_e", [this]() { res(m_gpr.e, 3); }, 1, 2 };
    CB_OPCODES[0x9C] = { "RES_3_h", [this]() { res(m_gpr.h, 3); }, 1, 2 };
    CB_OPCODES[0x9D] = { "RES_3_l", [this]() { res(m_gpr.l, 3); }, 1, 2 };
    CB_OPCODES[0xA6] = { "RES_4_(hl)", [this]() { res(m_gpr.hl, 4); }, 1, 4 };
    CB_OPCODES[0xA7] = { "RES_4_a", [this]() { res(m_gpr.a, 4); }, 1, 2 };
    CB_OPCODES[0xA0] = { "RES_4_b", [this]() { res(m_gpr.b, 4); }, 1, 2 };
    CB_OPCODES[0xA1] = { "RES_4_c", [this]() { res(m_gpr.c, 4); }, 1, 2 };
    CB_OPCODES[0xA2] = { "RES_4_d", [this]() { res(m_gpr.d, 4); }, 1, 2 };
    CB_OPCODES[0xA3] = { "RES_4_e", [this]() { res(m_gpr.e, 4); }, 1, 2 };
    CB_OPCODES[0xA4] = { "RES_4_h", [this]() { res(m_gpr.h, 4); }, 1, 2 };
    CB_OPCODES[0xA5] = { "RES_4_l", [this]() { res(m_gpr.l, 4); }, 1, 2 };
    CB_OPCODES[0xAE] = { "RES_5_(hl)", [this]() { res(m_gpr.hl, 5); }, 1, 4 };
    CB_OPCODES[0xAF] = { "RES_5_a", [this]() { res(m_gpr.a, 5); }, 1, 2 };
    CB_OPCODES[0xA8] = { "RES_5_b", [this]() { res(m_gpr.b, 5); }, 1, 2 };
    CB_OPCODES[0xA9] = { "RES_5_c", [this]() { res(m_gpr.c, 5); }, 1, 2 };
    CB_OPCODES[0xAA] = { "RES_5_d", [this]() { res(m_gpr.d, 5); }, 1, 2 };
    CB_OPCODES[0xAB] = { "RES_5_e", [this]() { res(m_gpr.e, 5); }, 1, 2 };
    CB_OPCODES[0xAC] = { "RES_5_h", [this]() { res(m_gpr.h, 5); }, 1, 2 };
    CB_OPCODES[0xAD] = { "RES_5_l", [this]() { res(m_gpr.l, 5); }, 1, 2 };
    CB_OPCODES[0xB6] = { "RES_6_(hl)", [this]() { res(m_gpr.hl, 6); }, 1, 4 };
    CB_OPCODES[0xB7] = { "RES_6_a", [this]() { res(m_gpr.a, 6); }, 1, 2 };
    CB_OPCODES[0xB0] = { "RES_6_b", [this]() { res(m_gpr.b, 6); }, 1, 2 };
    CB_OPCODES[0xB1] = { "RES_6_c", [this]() { res(m_gpr.c, 6); }, 1, 2 };
    CB_OPCODES[0xB2] = { "RES_6_d", [this]() { res(m_gpr.d, 6); }, 1, 2 };
    CB_OPCODES[0xB3] = { "RES_6_e", [this]() { res(m_gpr.e, 6); }, 1, 2 };
    CB_OPCODES[0xB4] = { "RES_6_h", [this]() { res(m_gpr.h, 6); }, 1, 2 };
    CB_OPCODES[0xB5] = { "RES_6_l", [this]() { res(m_gpr.l, 6); }, 1, 2 };
    CB_OPCODES[0xBE] = { "RES_7_(hl)", [this]() { res(m_gpr.hl, 7); }, 1, 4 };
    CB_OPCODES[0xBF] = { "RES_7_a", [this]() { res(m_gpr.a, 7); }, 1, 2 };
    CB_OPCODES[0xB8] = { "RES_7_b", [this]() { res(m_gpr.b, 7); }, 1, 2 };
    CB_OPCODES[0xB9] = { "RES_7_c", [this]() { res(m_gpr.c, 7); }, 1, 2 };
    CB_OPCODES[0xBA] = { "RES_7_d", [this]() { res(m_gpr.d, 7); }, 1, 2 };
    CB_OPCODES[0xBB] = { "RES_7_e", [this]() { res(m_gpr.e, 7); }, 1, 2 };
    CB_OPCODES[0xBC] = { "RES_7_h", [this]() { res(m_gpr.h, 7); }, 1, 2 };
    CB_OPCODES[0xBD] = { "RES_7_l", [this]() { res(m_gpr.l, 7); }, 1, 2 };

    CB_OPCODES[0x16] = { "RL_(hl)", [this]() { rotatel(m_gpr.hl, true, false); }, 1, 4 };
    CB_OPCODES[0x17] = { "RL_a",    [this]() { rotatel(m_gpr.a, true, false); }, 1, 2 };
    CB_OPCODES[0x10] = { "RL_b",    [this]() { rotatel(m_gpr.b, true, false); }, 1, 2 };
    CB_OPCODES[0x11] = { "RL_c",    [this]() { rotatel(m_gpr.c, true, false); }, 1, 2 };
    CB_OPCODES[0x12] = { "RL_d",    [this]() { rotatel(m_gpr.d, true, false); }, 1, 2 };
    CB_OPCODES[0x13] = { "RL_e",    [this]() { rotatel(m_gpr.e, true, false); }, 1, 2 };
    CB_OPCODES[0x14] = { "RL_h",    [this]() { rotatel(m_gpr.h, true, false); }, 1, 2 };
    CB_OPCODES[0x15] = { "RL_l",    [this]() { rotatel(m_gpr.l, true, false); }, 1, 2 };

    CB_OPCODES[0x06] = { "RLC_(hl)", [this]() { rlc(m_gpr.hl, false); }, 1, 4 };
    CB_OPCODES[0x07] = { "RLC_a",    [this]() { rlc(m_gpr.a, false); }, 1, 2 };
    CB_OPCODES[0x00] = { "RLC_b",    [this]() { rlc(m_gpr.b, false); }, 1, 2 };
    CB_OPCODES[0x01] = { "RLC_c",    [this]() { rlc(m_gpr.c, false); }, 1, 2 };
    CB_OPCODES[0x02] = { "RLC_d",    [this]() { rlc(m_gpr.d, false); }, 1, 2 };
    CB_OPCODES[0x03] = { "RLC_e",    [this]() { rlc(m_gpr.e, false); }, 1, 2 };
    CB_OPCODES[0x04] = { "RLC_h",    [this]() { rlc(m_gpr.h, false); }, 1, 2 };
    CB_OPCODES[0x05] = { "RLC_l",    [this]() { rlc(m_gpr.l, false); }, 1, 2 };

    CB_OPCODES[0x0E] = { "RRC_(hl)", [this]() { rrc(m_gpr.hl, false); }, 1, 4 };
    CB_OPCODES[0x0F] = { "RRC_a",    [this]() { rrc(m_gpr.a, false); }, 1, 2 };
    CB_OPCODES[0x08] = { "RRC_b",    [this]() { rrc(m_gpr.b, false); }, 1, 2 };
    CB_OPCODES[0x09] = { "RRC_c",    [this]() { rrc(m_gpr.c, false); }, 1, 2 };
    CB_OPCODES[0x0A] = { "RRC_d",    [this]() { rrc(m_gpr.d, false); }, 1, 2 };
    CB_OPCODES[0x0B] = { "RRC_e",    [this]() { rrc(m_gpr.e, false); }, 1, 2 };
    CB_OPCODES[0x0C] = { "RRC_h",    [this]() { rrc(m_gpr.h, false); }, 1, 2 };
    CB_OPCODES[0x0D] = { "RRC_l",    [this]() { rrc(m_gpr.l, false); }, 1, 2 };

    CB_OPCODES[0x1E] = { "RR_(hl)", [this]() { rotater(m_gpr.hl, true, false); }, 1, 4 };
    CB_OPCODES[0x1F] = { "RR_a",    [this]() { rotater(m_gpr.a, true, false); }, 1, 2 };
    CB_OPCODES[0x18] = { "RR_b",    [this]() { rotater(m_gpr.b, true, false); }, 1, 2 };
    CB_OPCODES[0x19] = { "RR_c",    [this]() { rotater(m_gpr.c, true, false); }, 1, 2 };
    CB_OPCODES[0x1A] = { "RR_d",    [this]() { rotater(m_gpr.d, true, false); }, 1, 2 };
    CB_OPCODES[0x1B] = { "RR_e",    [this]() { rotater(m_gpr.e, true, false); }, 1, 2 };
    CB_OPCODES[0x1C] = { "RR_h",    [this]() { rotater(m_gpr.h, true, false); }, 1, 2 };
    CB_OPCODES[0x1D] = { "RR_l",    [this]() { rotater(m_gpr.l, true, false); }, 1, 2 };

    CB_OPCODES[0xC6] = { "SET_0_(hl)", [this]() { set(m_gpr.hl, 0); }, 1, 4 };
    CB_OPCODES[0xC7] = { "SET_0_a", [this]() { set(m_gpr.a, 0); }, 1, 2 };
    CB_OPCODES[0xC0] = { "SET_0_b", [this]() { set(m_gpr.b, 0); }, 1, 2 };
    CB_OPCODES[0xC1] = { "SET_0_c", [this]() { set(m_gpr.c, 0); }, 1, 2 };
    CB_OPCODES[0xC2] = { "SET_0_d", [this]() { set(m_gpr.d, 0); }, 1, 2 };
    CB_OPCODES[0xC3] = { "SET_0_e", [this]() { set(m_gpr.e, 0); }, 1, 2 };
    CB_OPCODES[0xC4] = { "SET_0_h", [this]() { set(m_gpr.h, 0); }, 1, 2 };
    CB_OPCODES[0xC5] = { "SET_0_l", [this]() { set(m_gpr.l, 0); }, 1, 2 };
    CB_OPCODES[0xCE] = { "SET_1_(hl)", [this]() { set(m_gpr.hl, 1); }, 1, 4 };
    CB_OPCODES[0xCF] = { "SET_1_a", [this]() { set(m_gpr.a, 1); }, 1, 2 };
    CB_OPCODES[0xC8] = { "SET_1_b", [this]() { set(m_gpr.b, 1); }, 1, 2 };
    CB_OPCODES[0xC9] = { "SET_1_c", [this]() { set(m_gpr.c, 1); }, 1, 2 };
    CB_OPCODES[0xCA] = { "SET_1_d", [this]() { set(m_gpr.d, 1); }, 1, 2 };
    CB_OPCODES[0xCB] = { "SET_1_e", [this]() { set(m_gpr.e, 1); }, 1, 2 };
    CB_OPCODES[0xCC] = { "SET_1_h", [this]() { set(m_gpr.h, 1); }, 1, 2 };
    CB_OPCODES[0xCD] = { "SET_1_l", [this]() { set(m_gpr.l, 1); }, 1, 2 };
    CB_OPCODES[0xD6] = { "SET_2_(hl)", [this]() { set(m_gpr.hl, 2); }, 1, 4 };
    CB_OPCODES[0xD7] = { "SET_2_a", [this]() { set(m_gpr.a, 2); }, 1, 2 };
    CB_OPCODES[0xD0] = { "SET_2_b", [this]() { set(m_gpr.b, 2); }, 1, 2 };
    CB_OPCODES[0xD1] = { "SET_2_c", [this]() { set(m_gpr.c, 2); }, 1, 2 };
    CB_OPCODES[0xD2] = { "SET_2_d", [this]() { set(m_gpr.d, 2); }, 1, 2 };
    CB_OPCODES[0xD3] = { "SET_2_e", [this]() { set(m_gpr.e, 2); }, 1, 2 };
    CB_OPCODES[0xD4] = { "SET_2_h", [this]() { set(m_gpr.h, 2); }, 1, 2 };
    CB_OPCODES[0xD5] = { "SET_2_l", [this]() { set(m_gpr.l, 2); }, 1, 2 };
    CB_OPCODES[0xDE] = { "SET_3_(hl)", [this]() { set(m_gpr.hl, 3); }, 1, 4 };
    CB_OPCODES[0xDF] = { "SET_3_a", [this]() { set(m_gpr.a, 3); }, 1, 2 };
    CB_OPCODES[0xD8] = { "SET_3_b", [this]() { set(m_gpr.b, 3); }, 1, 2 };
    CB_OPCODES[0xD9] = { "SET_3_c", [this]() { set(m_gpr.c, 3); }, 1, 2 };
    CB_OPCODES[0xDA] = { "SET_3_d", [this]() { set(m_gpr.d, 3); }, 1, 2 };
    CB_OPCODES[0xDB] = { "SET_3_e", [this]() { set(m_gpr.e, 3); }, 1, 2 };
    CB_OPCODES[0xDC] = { "SET_3_h", [this]() { set(m_gpr.h, 3); }, 1, 2 };
    CB_OPCODES[0xDD] = { "SET_3_l", [this]() { set(m_gpr.l, 3); }, 1, 2 };
    CB_OPCODES[0xE6] = { "SET_4_(hl)", [this]() { set(m_gpr.hl, 4); }, 1, 4 };
    CB_OPCODES[0xE7] = { "SET_4_a", [this]() { set(m_gpr.a, 4); }, 1, 2 };
    CB_OPCODES[0xE0] = { "SET_4_b", [this]() { set(m_gpr.b, 4); }, 1, 2 };
    CB_OPCODES[0xE1] = { "SET_4_c", [this]() { set(m_gpr.c, 4); }, 1, 2 };
    CB_OPCODES[0xE2] = { "SET_4_d", [this]() { set(m_gpr.d, 4); }, 1, 2 };
    CB_OPCODES[0xE3] = { "SET_4_e", [this]() { set(m_gpr.e, 4); }, 1, 2 };
    CB_OPCODES[0xE4] = { "SET_4_h", [this]() { set(m_gpr.h, 4); }, 1, 2 };
    CB_OPCODES[0xE5] = { "SET_4_l", [this]() { set(m_gpr.l, 4); }, 1, 2 };
    CB_OPCODES[0xEE] = { "SET_5_(hl)", [this]() { set(m_gpr.hl, 5); }, 1, 4 };
    CB_OPCODES[0xEF] = { "SET_5_a", [this]() { set(m_gpr.a, 5); }, 1, 2 };
    CB_OPCODES[0xE8] = { "SET_5_b", [this]() { set(m_gpr.b, 5); }, 1, 2 };
    CB_OPCODES[0xE9] = { "SET_5_c", [this]() { set(m_gpr.c, 5); }, 1, 2 };
    CB_OPCODES[0xEA] = { "SET_5_d", [this]() { set(m_gpr.d, 5); }, 1, 2 };
    CB_OPCODES[0xEB] = { "SET_5_e", [this]() { set(m_gpr.e, 5); }, 1, 2 };
    CB_OPCODES[0xEC] = { "SET_5_h", [this]() { set(m_gpr.h, 5); }, 1, 2 };
    CB_OPCODES[0xED] = { "SET_5_l", [this]() { set(m_gpr.l, 5); }, 1, 2 };
    CB_OPCODES[0xF6] = { "SET_6_(hl)", [this]() { set(m_gpr.hl, 6); }, 1, 4 };
    CB_OPCODES[0xF7] = { "SET_6_a", [this]() { set(m_gpr.a, 6); }, 1, 2 };
    CB_OPCODES[0xF0] = { "SET_6_b", [this]() { set(m_gpr.b, 6); }, 1, 2 };
    CB_OPCODES[0xF1] = { "SET_6_c", [this]() { set(m_gpr.c, 6); }, 1, 2 };
    CB_OPCODES[0xF2] = { "SET_6_d", [this]() { set(m_gpr.d, 6); }, 1, 2 };
    CB_OPCODES[0xF3] = { "SET_6_e", [this]() { set(m_gpr.e, 6); }, 1, 2 };
    CB_OPCODES[0xF4] = { "SET_6_h", [this]() { set(m_gpr.h, 6); }, 1, 2 };
    CB_OPCODES[0xF5] = { "SET_6_l", [this]() { set(m_gpr.l, 6); }, 1, 2 };
    CB_OPCODES[0xFE] = { "SET_7_(hl)", [this]() { set(m_gpr.hl, 7); }, 1, 4 };
    CB_OPCODES[0xFF] = { "SET_7_a", [this]() { set(m_gpr.a, 7); }, 1, 2 };
    CB_OPCODES[0xF8] = { "SET_7_b", [this]() { set(m_gpr.b, 7); }, 1, 2 };
    CB_OPCODES[0xF9] = { "SET_7_c", [this]() { set(m_gpr.c, 7); }, 1, 2 };
    CB_OPCODES[0xFA] = { "SET_7_d", [this]() { set(m_gpr.d, 7); }, 1, 2 };
    CB_OPCODES[0xFB] = { "SET_7_e", [this]() { set(m_gpr.e, 7); }, 1, 2 };
    CB_OPCODES[0xFC] = { "SET_7_h", [this]() { set(m_gpr.h, 7); }, 1, 2 };
    CB_OPCODES[0xFD] = { "SET_7_l", [this]() { set(m_gpr.l, 7); }, 1, 2 };

    CB_OPCODES[0x26] = { "SLA_(hl)", [this]() { sla(m_gpr.hl); }, 1, 4 };
    CB_OPCODES[0x27] = { "SLA_a",    [this]() { sla(m_gpr.a); }, 1, 2 };
    CB_OPCODES[0x20] = { "SLA_b",    [this]() { sla(m_gpr.b); }, 1, 2 };
    CB_OPCODES[0x21] = { "SLA_c",    [this]() { sla(m_gpr.c); }, 1, 2 };
    CB_OPCODES[0x22] = { "SLA_d",    [this]() { sla(m_gpr.d); }, 1, 2 };
    CB_OPCODES[0x23] = { "SLA_e",    [this]() { sla(m_gpr.e); }, 1, 2 };
    CB_OPCODES[0x24] = { "SLA_h",    [this]() { sla(m_gpr.h); }, 1, 2 };
    CB_OPCODES[0x25] = { "SLA_l",    [this]() { sla(m_gpr.l); }, 1, 2 };

    CB_OPCODES[0x2E] = { "SRA_(hl)", [this]() { sra(m_gpr.hl); }, 1, 4 };
    CB_OPCODES[0x2F] = { "SRA_a",    [this]() { sra(m_gpr.a); }, 1, 2 };
    CB_OPCODES[0x28] = { "SRA_b",    [this]() { sra(m_gpr.b); }, 1, 2 };
    CB_OPCODES[0x29] = { "SRA_c",    [this]() { sra(m_gpr.c); }, 1, 2 };
    CB_OPCODES[0x2A] = { "SRA_d",    [this]() { sra(m_gpr.d); }, 1, 2 };
    CB_OPCODES[0x2B] = { "SRA_e",    [this]() { sra(m_gpr.e); }, 1, 2 };
    CB_OPCODES[0x2C] = { "SRA_h",    [this]() { sra(m_gpr.h); }, 1, 2 };
    CB_OPCODES[0x2D] = { "SRA_l",    [this]() { sra(m_gpr.l); }, 1, 2 };

    CB_OPCODES[0x36] = { "SWAP_(hl)", [this]() { swap(m_gpr.hl); }, 1, 4 };
    CB_OPCODES[0x37] = { "SWAP_a",    [this]() { swap(m_gpr.a); }, 1, 2 };
    CB_OPCODES[0x30] = { "SWAP_b",    [this]() { swap(m_gpr.b); }, 1, 2 };
    CB_OPCODES[0x31] = { "SWAP_c",    [this]() { swap(m_gpr.c); }, 1, 2 };
    CB_OPCODES[0x32] = { "SWAP_d",    [this]() { swap(m_gpr.d); }, 1, 2 };
    CB_OPCODES[0x33] = { "SWAP_e",    [this]() { swap(m_gpr.e); }, 1, 2 };
    CB_OPCODES[0x34] = { "SWAP_h",    [this]() { swap(m_gpr.h); }, 1, 2 };
    CB_OPCODES[0x35] = { "SWAP_l",    [this]() { swap(m_gpr.l); }, 1, 2 };

    CB_OPCODES[0x3E] = { "SRL_(hl)", [this]() { srl(m_gpr.hl); }, 1, 4 };
    CB_OPCODES[0x3F] = { "SRL_a",    [this]() { srl(m_gpr.a); }, 1, 2 };
    CB_OPCODES[0x38] = { "SRL_b",    [this]() { srl(m_gpr.b); }, 1, 2 };
    CB_OPCODES[0x39] = { "SRL_c",    [this]() { srl(m_gpr.c); }, 1, 2 };
    CB_OPCODES[0x3A] = { "SRL_d",    [this]() { srl(m_gpr.d); }, 1, 2 };
    CB_OPCODES[0x3B] = { "SRL_e",    [this]() { srl(m_gpr.e); }, 1, 2 };
    CB_OPCODES[0x3C] = { "SRL_h",    [this]() { srl(m_gpr.h); }, 1, 2 };
    CB_OPCODES[0x3D] = { "SRL_l",    [this]() { srl(m_gpr.l); }, 1, 2 };

    ILLEGAL_OPCODES = {
        0xD3, 0xDB, 0xDD, 0xE3, 0xE4, 0xEB, 0xEC, 0xED, 0xF4, 0xFC, 0xFD,
    };

#ifdef DEBUG
    for (uint16_t i = 0; i < 0x100; i++) {
        if ((CB_PREFIX == i) || (!OPCODES.at(i).name.empty())) {
            continue;
        }
        if (ILLEGAL_OPCODES.end() != ILLEGAL_OPCODES.find(i)) {
            continue;
        }
        WARN("Unimplemented opcode 0x%02x\n", i);
    }
    for (uint16_t i = 0; i < 0x100; i++) {
        if (CB_OPCODES.at(i).name.empty()) {
            WARN("Unimplemented CB opcode 0x%02x\n", i);
        }
    }
#endif
}
