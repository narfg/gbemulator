#include "cpu.h"

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <unistd.h>

#include "dummydisplay.h"
#include "instructiondecoder.h"
#include "joypad.h"
#include "ppu.h"
#include "romloader.h"
#include "utils.h"

CPU::CPU(uint8_t* ram, Display* display, Joypad* joypad):
    ram_(ram == nullptr ? new uint8_t[65536] : ram),
    ppu_(ram_, display == nullptr ? new DummyDisplay : display),
    timer_(ram_),
    joypad_(joypad),
    pc_(0x100),
    mbc_rombank_(1),
    mbc_mode_(0),
    interrupt_master_enable_(false),
    halted_(false),
    stopped_(false),
    cycles_(0)
{
    // Initialize registers
    af_.a_ = 0x01; // Game Boy Color has 0x11 here
    af_.f_ = 0xB0;
    bc_.b_ = 0x00;
    bc_.c_ = 0x13;
    de_.d_ = 0x00;
    de_.e_ = 0xD8;
    hl_.h_ = 0x01;
    hl_.l_ = 0x4D;
    io_ = ram_ + 0xFF00;
    sp_ = 0xFFFE;

    // Read cartridge type
    //cartridge_type_ = rom_[0x0147];

    ram_[0xFF00] = 0xCF; // Buttons

    // Clear serial data register
    ram_[0xFF01] = 0x00;
    ram_[0xFF02] = 0x7E; // set SIO_EN and SIO_CLK to 0

    ram_[0xFF03] = 0xFF;
    ram_[0xFF04] = 0x18; // DIV
    // ram_[0xFF04] = 0x19; // DIV
    // ram_[0xFF04] = 0xAC; // DIV
    ram_[0xFF05] = 0x00; // TIMA
    ram_[0xFF06] = 0x00; // TMA
    ram_[0xFF07] = 0xF8; // TAC

    ram_[0xFF08] = 0xFF;
    ram_[0xFF09] = 0xFF;
    ram_[0xFF0A] = 0xFF;
    ram_[0xFF0B] = 0xFF;
    ram_[0xFF0C] = 0xFF;
    ram_[0xFF0D] = 0xFF;
    ram_[0xFF0E] = 0xFF;
    ram_[0xFF0F] = 0xE1; // IF
    ram_[0xFF10] = 0x80;
    ram_[0xFF11] = 0xBF;
    ram_[0xFF12] = 0xF3;
    ram_[0xFF13] = 0xFF;
    ram_[0xFF14] = 0xBF;
    ram_[0xFF15] = 0xFF;
    ram_[0xFF16] = 0x3F;
    ram_[0xFF18] = 0xFF;
    ram_[0xFF19] = 0xBF;
    ram_[0xFF1A] = 0x7F;
    ram_[0xFF1B] = 0xFF;
    ram_[0xFF1C] = 0x9F;
    ram_[0xFF1D] = 0xFF;
    ram_[0xFF1E] = 0xBF;
    ram_[0xFF1F] = 0xFF;

    ram_[0xFF20] = 0xFF;
    ram_[0xFF21] = 0x00;

    ram_[0xFF23] = 0xBF;
    ram_[0xFF24] = 0x77;
    ram_[0xFF25] = 0xF3;
    // ram_[0xFF26] = 0xF0;
    ram_[0xFF26] = 0xF1;

    for (uint16_t addr = 0xFF27; addr < 0xFF80; ++addr) {
        ram_[addr] = 0xFF;
    }
    ram_[0xFF40] = 0x91; // LCDC
    ram_[0xFF41] = 0x83; // STAT
    ram_[0xFF42] = 0x00; // SCY
    ram_[0xFF43] = 0x00; // SCX
    ram_[0xFF44] = 0x00; // LY maybe 01
    ram_[0xFF45] = 0x00; // LYC
    ram_[0xFF47] = 0xFC; // BGP
    ram_[0xFF4A] = 0x00; // WY
    ram_[0xFF4B] = 0x00; // WX
    ram_[0xFFFF] = 0x00; // IE

    setFlagZ();
    setFlagH();
    setFlagC();
}

void CPU::run() {
    while (true) {
        checkForInterrupts();
        uint8_t cycles = 4;
        if (!(halted_ || stopped_)) {
            uint8_t three_bytes[] = {readFromMemory(pc_), readFromMemory(pc_+1), readFromMemory(pc_+2)};
            uint16_t pc_old = pc_;
            Instruction instr = id_.decode(three_bytes, &pc_);
            if (halt_bug_) {
                // Check length of instruction
                if (pc_ - pc_old == 1) {
                    pc_ = pc_old;
                } else {
                    uint8_t three_bytes[] = {readFromMemory(pc_old), readFromMemory(pc_old), readFromMemory(pc_old+1)};
                    pc_ = pc_old;
                    instr = id_.decode(three_bytes, &pc_);
                    pc_--;
                }
                halt_bug_ = false;
            }
            bool taken = false;
            executeInstruction(instr.instruction, instr.prefix, instr.operand0, instr.operand1, &taken);
            if (taken) {
                cycles = instr.cycles_if_taken;
            } else {
                cycles = instr.cycles;
            }
            if (pc_ == breakpoint_) {
                printf("break\n");
            }
        }
        if (!stopped_) {
            for (int k = 0; k < cycles; ++k) {
                timer_.tick();
            }
            for (int k = 0; k < cycles; k+=4) {
                ppu_.tick();
            }
        }
        cycles_ += cycles;
    }
}

void CPU::checkForInterrupts() {
    uint8_t i_enable = ram_[0xFFFF];
    uint8_t i_flags = ram_[0xFF0F];
    if (interrupt_master_enable_) {
        for (uint8_t n = 0; n < 5; ++n) {
            if (getBitN(i_flags, n) && getBitN(i_enable, n)) {
                // printf("Interrupt %d, cycles: %ld\n", n, cycles_);
                // fflush(stdout);
                resetBit(&ram_[0xFF0F], n);
                interrupt_master_enable_ = false;
                halted_ = false;
                stopped_ = false;
                push(pc_);
                uint16_t interrupt_address = 0x0040 + n * 8;
                jump(interrupt_address);

                // Interrupts with lower address have priority
                break;
            }
        }
    } else {
        if (halted_) {
            if ((i_enable & i_flags & 0x1F) != 0) {
                halted_ = false;
            }
        }
    }
}

void CPU::printInstrWSP(const char* string, uint8_t operand0, uint8_t operand1) const {
    printf_instr("A:%02X ", getA());
    printf_instr("F:%c%c%c%c ", getFlagZ()?'Z':'-', getFlagN()?'N':'-', getFlagH()?'H':'-', getFlagC()?'C':'-');
    printf_instr("BC:%04X ", getBC());
    printf_instr("DE:%04x ", getDE());
    printf_instr("HL:%04x ", getHL());
    printf_instr("SP:%04x ", getSP());
    printf_instr("PC:%04x ", getPC());
    printf_instr("DIV:%02x ", ram_[0xFF04]);
    printf_instr("TIMA:%02x ", ram_[0xFF05]);
    // printf_instr("Flags: Z: %d, N: %d, H: %d, C: %d ", getFlagZ(), getFlagN(), getFlagH(), getFlagC());
    printf_instr("OP0: %02X OP1: %02X %s", operand0, operand1, string);
}

bool CPU::executeInstruction(uint8_t instruction, bool prefix, uint8_t operand0, uint8_t operand1, bool* taken) {
    if (!prefix) {
        switch (instruction) {
        case 0x00:
            printInstrWSP("NOP\n");
            break;
        case 0x01:
            printInstrWSP("LD BC, d16\n");
            bc_.c_ = operand0;
            bc_.b_ = operand1;
            break;
        case 0x02:
            printInstrWSP("LD (BC), A\n");
            writeToMemory(bc_.bc_, af_.a_);
            break;
        case 0x03:
            printInstrWSP("INC BC\n");
            increment(&bc_.bc_);
            break;
        case 0x04:
            printInstrWSP("INC B\n");
            increment(&bc_.b_);
            break;
        case 0x05:
            printInstrWSP("DEC B\n");
            decrement(&bc_.b_);
            break;
        case 0x06:
            printInstrWSP("LD B, d8\n");
            bc_.b_ = operand1;
            break;
        case 0x07:
            printInstrWSP("RLCA\n");
            // rotateLeft(&af_.a_);
            rlca();
            break;
        case 0x08:
            printInstrWSP("LD (a16), SP\n");
            writeToMemory(to16(operand0, operand1), sp_ & 0xFF);
            writeToMemory(to16(operand0, operand1)+1, (sp_ & 0xFF00) >> 8);
            break;
        case 0x09:
            printInstrWSP("ADD HL, BC\n");
            addHL(bc_.bc_);
            break;
        case 0x0A:
            printInstrWSP("LD A, (BC)\n");
            af_.a_ = readFromMemory(bc_.bc_);
            break;
        case 0x0B:
            printInstrWSP("DEC BC\n");
            decrement(&bc_.bc_);
            break;
        case 0x0C:
            printInstrWSP("INC C\n");
            increment(&bc_.c_);
            break;
        case 0x0D:
            printInstrWSP("DEC C\n");
            decrement(&bc_.c_);
            break;
        case 0x0E:
            printInstrWSP("LD C, d8\n");
            bc_.c_ = operand1;
            break;
        case 0x0F:
            printInstrWSP("RRCA\n");
            rrca();
            break;
        case 0x10:
            printInstrWSP("STOP 0\n");
            stopped_ = true;
            break;
        case 0x11:
            printInstrWSP("LD DE, d16\n");
            de_.e_ = operand0;
            de_.d_ = operand1;
            break;
        case 0x12:
            printInstrWSP("LD (DE), A\n");
            writeToMemory(de_.de_, af_.a_);
            break;
        case 0x13:
            printInstrWSP("INC DE\n");
            increment(&de_.de_);
            break;
        case 0x14:
            printInstrWSP("INC D\n");
            increment(&de_.d_);
            break;
        case 0x15:
            printInstrWSP("DEC D\n");
            decrement(&de_.d_);
            break;
        case 0x16:
            printInstrWSP("LD D, d8\n");
            de_.d_ = operand1;
            break;
        case 0x17:
            printInstrWSP("RLA\n");
            rla();
            break;
        case 0x18:
            printInstrWSP("JR r8\n");
            jump(1, static_cast<int8_t>(operand1));
            break;
        case 0x19:
            printInstrWSP("ADD HL, DE\n");
            addHL(de_.de_);
            break;
        case 0x1A:
            printInstrWSP("LD A, (DE)\n");
            af_.a_ = readFromMemory(de_.de_);
            break;
        case 0x1B:
            printInstrWSP("DEC DE\n");
            decrement(&de_.de_);
            break;
        case 0x1C:
            printInstrWSP("INC E\n");
            increment(&de_.e_);
            break;
        case 0x1D:
            printInstrWSP("DEC E\n");
            decrement(&de_.e_);
            break;
        case 0x1E:
            printInstrWSP("LD E, d8\n");
            de_.e_ = operand1;
            break;
        case 0x1F:
            printInstrWSP("RRA\n");
            rra();
            break;
        case 0x20:
            printInstrWSP("JR NZ, r8\n");
            if (!getFlagZ()) *taken = true;
            jump(!getFlagZ(), static_cast<int8_t>(operand1));
            break;
        case 0x21:
            printInstrWSP("LD HL, d16\n");
            hl_.l_ = operand0;
            hl_.h_ = operand1;
            break;
        case 0x22:
            printInstrWSP("LD (HL+), A\n");
            loadIncAddr(&hl_.hl_, af_.a_);
            break;
        case 0x23:
            printInstrWSP("INC HL\n");
            hl_.hl_++; // FIXME
            break;
        case 0x24:
            printInstrWSP("INC H\n");
            increment(&hl_.h_);
            break;
        case 0x25:
            printInstrWSP("DEC H\n");
            decrement(&hl_.h_);
            break;
        case 0x26:
            printInstrWSP("LD H, d8\n");
            hl_.h_ = operand1;
            break;
        case 0x27:
            printInstrWSP("DAA\n");
            daa();
            break;
        case 0x28:
            printInstrWSP("JR Z, r8\n");
            if (getFlagZ()) *taken = true;
            jump(getFlagZ(), static_cast<int8_t>(operand1));
            break;
        case 0x29:
            printInstrWSP("ADD HL, HL\n");
            addHL(hl_.hl_);
            break;
        case 0x2A:
            printInstrWSP("LD A, (HL+)\n");
            loadAHLPlus();
            break;
        case 0x2B:
            printInstrWSP("DEC HL\n");
            decrement(&hl_.hl_);
            break;
        case 0x2C:
            printInstrWSP("INC L\n");
            increment(&hl_.l_);
            break;
        case 0x2D:
            printInstrWSP("DEC L\n");
            decrement(&hl_.l_);
            break;
        case 0x2E:
            printInstrWSP("LD L, d8\n");
            hl_.l_ = operand1;
            break;
        case 0x2F:
            printInstrWSP("CPL\n");
            flipAllBits(&af_.a_);
            break;
        case 0x30:
            printInstrWSP("JR NC, r8\n");
            if (!getFlagC()) *taken = true;
            jump(!getFlagC(), static_cast<int8_t>(operand1)); // FIXME
            break;
        case 0x31:
            printInstrWSP("LD SP, d16\n");
            loadSP(operand0, operand1);
            break;
        case 0x32:
            printInstrWSP("LD (HL-), A\n");
            loadDecAddr(&hl_.hl_, af_.a_);
            break;
        case 0x33:
            printInstrWSP("INC SP\n");
            increment(&sp_);
            break;
        case 0x34:
            printInstrWSP("INC (HL)\n");
            increment(&ram_[hl_.hl_]); // FIXME: Can't be traced
            break;
        case 0x35:
            printInstrWSP("DEC (HL), d8\n");
            decrement(&ram_[hl_.hl_]); // FIXME: Can't be traced
            break;
        case 0x36:
            printInstrWSP("LD (HL), d8\n");
            writeToMemory(hl_.hl_, operand1);
            break;
        case 0x37:
            printInstrWSP("SCF\n");
            setCarryFlag();
            break;
        case 0x38:
            printInstrWSP("JR C, r8\n");
            if (getFlagC()) *taken = true;
            jump(getFlagC(), static_cast<int8_t>(operand1));
            break;
        case 0x39:
            printInstrWSP("ADD HL, SP\n");
            addHL(sp_);
            break;
        case 0x3A:
            printInstrWSP("LD A, (HL-)\n");
            af_.a_ = readFromMemory(hl_.hl_);
            hl_.hl_--;
            break;
        case 0x3B:
            printInstrWSP("DEC SP\n");
            decrement(&sp_);
            break;
        case 0x3C:
            printInstrWSP("INC A\n");
            increment(&af_.a_);
            break;
        case 0x3D:
            printInstrWSP("DEC A\n");
            decrement(&af_.a_);
            break;
        case 0x3E:
            printInstrWSP("LD A, d8\n");
            af_.a_ = operand1;
            break;
        case 0x3F:
            printInstrWSP("CCF\n");
            complementCarry();
            break;
        case 0x40:
            printInstrWSP("LD B, B\n");
            break;
        case 0x41:
            printInstrWSP("LD B, C\n");
            bc_.b_ = bc_.c_;
            break;
        case 0x42:
            printInstrWSP("LD B, D\n");
            bc_.b_ = de_.d_;
            break;
        case 0x43:
            printInstrWSP("LD B, E\n");
            bc_.b_ = de_.e_;
            break;
        case 0x44:
            printInstrWSP("LD B, H\n");
            bc_.b_ = hl_.h_;
            break;
        case 0x45:
            printInstrWSP("LD B, L\n");
            bc_.b_ = hl_.l_;
            break;
        case 0x46:
            printInstrWSP("LD B, (HL)\n");
            bc_.b_ = readFromMemory(hl_.hl_);
            break;
        case 0x47:
            printInstrWSP("LD B, A\n");
            bc_.b_ = af_.a_;
            break;
        case 0x48:
            printInstrWSP("LD C, B\n");
            bc_.c_ = bc_.b_;
            break;
        case 0x49:
            printInstrWSP("LD C, C\n");
            bc_.c_ = bc_.c_;
            break;
        case 0x4A:
            printInstrWSP("LD C, D\n");
            bc_.c_ = de_.d_;
            break;
        case 0x4B:
            printInstrWSP("LD C, E\n");
            bc_.c_ = de_.e_;
            break;
        case 0x4C:
            printInstrWSP("LD C, H\n");
            bc_.c_ = hl_.h_;
            break;
        case 0x4D:
            printInstrWSP("LD C, L\n");
            bc_.c_ = hl_.l_;
            break;
        case 0x4E:
            printInstrWSP("LD C, (HL)\n");
            bc_.c_ = readFromMemory(hl_.hl_);
            break;
        case 0x4F:
            printInstrWSP("LD C, A\n");
            bc_.c_ = af_.a_;
            break;
        case 0x50:
            printInstrWSP("LD D, B\n");
            de_.d_ = bc_.b_;
            break;
        case 0x51:
            printInstrWSP("LD D, C\n");
            de_.d_ = bc_.c_;
            break;
        case 0x52:
            printInstrWSP("LD D, D\n");
            de_.d_ = de_.d_;
            break;
        case 0x53:
            printInstrWSP("LD D, E\n");
            de_.d_ = de_.e_;
            break;
        case 0x54:
            printInstrWSP("LD D, H\n");
            de_.d_ = hl_.h_;
            break;
        case 0x55:
            printInstrWSP("LD D, L\n");
            de_.d_ = hl_.l_;
            break;
        case 0x56:
            printInstrWSP("LD D, (HL)\n");
            de_.d_ = readFromMemory(hl_.hl_);
            break;
        case 0x57:
            printInstrWSP("LD D, A\n");
            de_.d_ = af_.a_;
            break;
        case 0x58:
            printInstrWSP("LD E, B\n");
            de_.e_ = bc_.b_;
            break;
        case 0x59:
            printInstrWSP("LD E, C\n");
            de_.e_ = bc_.c_;
            break;
        case 0x5A:
            printInstrWSP("LD E, D\n");
            de_.e_ = de_.d_;
            break;
        case 0x5B:
            printInstrWSP("LD E, E\n");
            de_.e_ = de_.e_;
            break;
        case 0x5C:
            printInstrWSP("LD E, H\n");
            de_.e_ = hl_.h_;
            break;
        case 0x5D:
            printInstrWSP("LD E, L\n");
            de_.e_ = hl_.l_;
            break;
        case 0x5E:
            printInstrWSP("LD E, (HL)\n");
            de_.e_ = readFromMemory(hl_.hl_);
            break;
        case 0x5F:
            printInstrWSP("LD E, A\n");
            de_.e_ = af_.a_;
            break;
        case 0x60:
            printInstrWSP("LD H, B\n");
            hl_.h_ = bc_.b_;
            break;
        case 0x61:
            printInstrWSP("LD H, C\n");
            hl_.h_ = bc_.c_;
            break;
        case 0x62:
            printInstrWSP("LD H, D\n");
            hl_.h_ = de_.d_;
            break;
        case 0x63:
            printInstrWSP("LD H, E\n");
            hl_.h_ = de_.e_;
            break;
        case 0x64:
            printInstrWSP("LD H, H\n");
            hl_.h_ = hl_.h_; // Why?
            break;
        case 0x65:
            printInstrWSP("LD H, L\n");
            hl_.h_ = hl_.l_;
            break;
        case 0x66:
            printInstrWSP("LD H, (HL)\n");
            hl_.h_ = readFromMemory(hl_.hl_);
            break;
        case 0x67:
            printInstrWSP("LD H, A\n");
            hl_.h_ = af_.a_;
            break;
        case 0x68:
            printInstrWSP("LD L, B\n");
            hl_.l_ = bc_.b_;
            break;
        case 0x69:
            printInstrWSP("LD L, C\n");
            hl_.l_ = bc_.c_;
            break;
        case 0x6A:
            printInstrWSP("LD L, D\n");
            hl_.l_ = de_.d_;
            break;
        case 0x6B:
            printInstrWSP("LD L, E\n");
            hl_.l_ = de_.e_;
            break;
        case 0x6C:
            printInstrWSP("LD L, H\n");
            hl_.l_ = hl_.h_;
            break;
        case 0x6D:
            printInstrWSP("LD L, L\n");
            hl_.l_ = hl_.l_; // FIXME: Why?
            break;
        case 0x6E:
            printInstrWSP("LD L, (HL)\n");
            hl_.l_ = readFromMemory(hl_.hl_);
            break;
        case 0x6F:
            printInstrWSP("LD L, A\n");
            hl_.l_ = af_.a_;
            break;
        case 0x70:
            printInstrWSP("LD (HL), B\n");
            loadAddr(hl_.hl_, bc_.b_);
            break;
        case 0x71:
            printInstrWSP("LD (HL), C\n");
            loadAddr(hl_.hl_, bc_.c_);
            break;
        case 0x72:
            printInstrWSP("LD (HL), D\n");
            loadAddr(hl_.hl_, de_.d_);
            break;
        case 0x73:
            printInstrWSP("LD (HL), E\n");
            loadAddr(hl_.hl_, de_.e_);
            break;
        case 0x74:
            printInstrWSP("LD (HL), H\n");
            loadAddr(hl_.hl_, hl_.h_);
            break;
        case 0x75:
            printInstrWSP("LD (HL), L\n");
            loadAddr(hl_.hl_, hl_.l_);
            break;
        case 0x76:
            printInstrWSP("HALT\n");
            halt();
            break;
        case 0x77:
            printInstrWSP("LD (HL), A\n");
            loadAddr(hl_.hl_, af_.a_);
            break;
        case 0x78:
            printInstrWSP("LD A, B\n");
            af_.a_ = bc_.b_;
            break;
        case 0x79:
            printInstrWSP("LD A, C\n");
            af_.a_ = bc_.c_;
            break;
        case 0x7A:
            printInstrWSP("LD A, D\n");
            af_.a_ = de_.d_;
            break;
        case 0x7B:
            printInstrWSP("LD A, E\n");
            af_.a_ = de_.e_;
            break;
        case 0x7C:
            printInstrWSP("LD A, H\n");
            af_.a_ = hl_.h_;
            break;
        case 0x7D:
            printInstrWSP("LD A, L\n");
            af_.a_ = hl_.l_;
            break;
        case 0x7E:
            printInstrWSP("LD A, (HL)\n");
            af_.a_ = readFromMemory(hl_.hl_);
            break;
        case 0x7F:
            printInstrWSP("LD A, A\n");
            af_.a_ = af_.a_;
            break;
        case 0x80:
            printInstrWSP("ADD A, B\n");
            add(bc_.b_);
            break;
        case 0x81:
            printInstrWSP("ADD A, C\n");
            add(bc_.c_);
            break;
        case 0x82:
            printInstrWSP("ADD A, D\n");
            add(de_.d_);
            break;
        case 0x83:
            printInstrWSP("ADD A, E\n");
            add(de_.e_);
            break;
        case 0x84:
            printInstrWSP("ADD A, H\n");
            add(hl_.h_);
            break;
        case 0x85:
            printInstrWSP("ADD A, L\n");
            add(hl_.l_);
            break;
        case 0x86:
            printInstrWSP("ADD A, (HL)\n");
            add(readFromMemory(hl_.hl_));
            break;
        case 0x87:
            printInstrWSP("ADD A, A\n");
            add(af_.a_);
            break;
        case 0x88:
            printInstrWSP("ADC A, B\n");
            addNPlusCarry(bc_.b_);
            break;
        case 0x89:
            printInstrWSP("ADC A, C\n");
            addNPlusCarry(bc_.c_);
            break;
        case 0x8A:
            printInstrWSP("ADC A, D\n");
            addNPlusCarry(de_.d_);
            break;
        case 0x8B:
            printInstrWSP("ADC A, E\n");
            addNPlusCarry(de_.e_);
            break;
        case 0x8C:
            printInstrWSP("ADC A, H\n");
            addNPlusCarry(hl_.h_);
            break;
        case 0x8D:
            printInstrWSP("ADC A, L\n");
            addNPlusCarry(hl_.l_);
            break;
        case 0x8E:
            printInstrWSP("ADC A, (HL)\n");
            addNPlusCarry(readFromMemory(hl_.hl_));
            break;
        case 0x8F:
            printInstrWSP("ADC A, A\n");
            addNPlusCarry(af_.a_);
            break;
        case 0x90:
            printInstrWSP("SUB B\n");
            sub(bc_.b_);
            break;
        case 0x91:
            printInstrWSP("SUB C\n");
            sub(bc_.c_);
            break;
        case 0x92:
            printInstrWSP("SUB D\n");
            sub(de_.d_);
            break;
        case 0x93:
            printInstrWSP("SUB E\n");
            sub(de_.e_);
            break;
        case 0x94:
            printInstrWSP("SUB H\n");
            sub(hl_.h_);
            break;
        case 0x95:
            printInstrWSP("SUB L\n");
            sub(hl_.l_);
            break;
        case 0x96:
            printInstrWSP("SUB (HL)\n");
            sub(readFromMemory(hl_.hl_));
            break;
        case 0x97:
            printInstrWSP("SUB A\n");
            sub(af_.a_);
            break;
        case 0x98:
            printInstrWSP("SBC A, B\n");
            subNPlusCarry(bc_.b_);
            break;
        case 0x99:
            printInstrWSP("SBC A, C\n");
            subNPlusCarry(bc_.c_);
            break;
        case 0x9A:
            printInstrWSP("SBC A, D\n");
            subNPlusCarry(de_.d_);
            break;
        case 0x9B:
            printInstrWSP("SBC A, E\n");
            subNPlusCarry(de_.e_);
            break;
        case 0x9C:
            printInstrWSP("SBC A, H\n");
            subNPlusCarry(hl_.h_);
            break;
        case 0x9D:
            printInstrWSP("SBC A, L\n");
            subNPlusCarry(hl_.l_);
            break;
        case 0x9E:
            printInstrWSP("SBC A, (HL)\n");
            subNPlusCarry(readFromMemory(hl_.hl_));
            break;
        case 0x9F:
            printInstrWSP("SBC A, A\n");
            subNPlusCarry(af_.a_);
            break;
        case 0xA0:
            printInstrWSP("AND B\n");
            andA(bc_.b_);
            break;
        case 0xA1:
            printInstrWSP("AND C\n");
            andA(bc_.c_);
            break;
        case 0xA2:
            printInstrWSP("AND D\n");
            andA(de_.d_);
            break;
        case 0xA3:
            printInstrWSP("AND E\n");
            andA(de_.e_);
            break;
        case 0xA4:
            printInstrWSP("AND H\n");
            andA(hl_.h_);
            break;
        case 0xA5:
            printInstrWSP("AND L\n");
            andA(hl_.l_);
            break;
        case 0xA6:
            printInstrWSP("AND (HL)\n");
            andA(readFromMemory(hl_.hl_));
            break;
        case 0xA7:
            printInstrWSP("AND A\n");
            andA(af_.a_);
            break;
        case 0xA8:
            printInstrWSP("XOR B\n");
            xorA(bc_.b_);
            break;
        case 0xA9:
            printInstrWSP("XOR C\n");
            xorA(bc_.c_);
            break;
        case 0xAA:
            printInstrWSP("XOR D\n");
            xorA(de_.d_);
            break;
        case 0xAB:
            printInstrWSP("XOR E\n");
            xorA(de_.e_);
            break;
        case 0xAC:
            printInstrWSP("XOR H\n");
            xorA(hl_.h_);
            break;
        case 0xAD:
            printInstrWSP("XOR L\n");
            xorA(hl_.l_);
            break;
        case 0xAE:
            printInstrWSP("XOR (HL)\n");
            xorA(readFromMemory(hl_.hl_));
            break;
        case 0xAF:
            printInstrWSP("XOR A\n");
            xorA(af_.a_);
            break;
        case 0xB0:
            printInstrWSP("OR B\n");
            orA(bc_.b_);
            break;
        case 0xB1:
            printInstrWSP("OR C\n");
            orA(bc_.c_);
            break;
        case 0xB2:
            printInstrWSP("OR D\n");
            orA(de_.d_);
            break;
        case 0xB3:
            printInstrWSP("OR E\n");
            orA(de_.e_);
            break;
        case 0xB4:
            printInstrWSP("OR H\n");
            orA(hl_.h_);
            break;
        case 0xB5:
            printInstrWSP("OR L\n");
            orA(hl_.l_);
            break;
        case 0xB6:
            printInstrWSP("OR (HL)\n");
            orA(readFromMemory(hl_.hl_));
            break;
        case 0xB7:
            printInstrWSP("OR A\n");
            orA(af_.a_);
            break;
        case 0xB8:
            printInstrWSP("CP B\n");
            compare(bc_.b_);
            break;
        case 0xB9:
            printInstrWSP("CP C\n");
            compare(bc_.c_);
            break;
        case 0xBA:
            printInstrWSP("CP D\n");
            compare(de_.d_);
            break;
        case 0xBB:
            printInstrWSP("CP E\n");
            compare(de_.e_);
            break;
        case 0xBC:
            printInstrWSP("CP H\n");
            compare(hl_.h_);
            break;
        case 0xBD:
            printInstrWSP("CP L\n");
            compare(hl_.l_);
            break;
        case 0xBE:
            printInstrWSP("CP (HL)\n");
            compare(readFromMemory(hl_.hl_));
            break;
        case 0xBF:
            printInstrWSP("CP A\n");
            compare(af_.a_);
            break;
        case 0xC0:
            printInstrWSP("RET NZ\n");
            if (!getFlagZ()) *taken = true;
            ret(!getFlagZ());
            break;
        case 0xC1:
            printInstrWSP("POP BC\n");
            pop(&bc_.c_, &bc_.b_);
            break;
        case 0xC2:
            printInstrWSP("JP NZ, (a16)\n", operand0, operand1);
            if (!getFlagZ()) *taken = true;
            jump(!getFlagZ(), to16(operand0, operand1));
            break;
        case 0xC3:
            printInstrWSP("JP a16\n", operand0, operand1);
            jump(to16(operand0, operand1));
            break;
        case 0xC4:
            printInstrWSP("CALL NZ, a16\n", operand0, operand1);
            if (!getFlagZ()) *taken = true;
            call(!getFlagZ(), to16(operand0, operand1));
            break;
        case 0xC5:
            printInstrWSP("PUSH BC\n");
            push(bc_.bc_);
            break;
        case 0xC6:
            printInstrWSP("ADD A, d8\n");
            add(operand1);
            break;
        case 0xC7:
            printInstrWSP("RES 00H\n");
            rst(0x00);
            break;
        case 0xC8:
            printInstrWSP("RET Z\n");
            if (getFlagZ()) *taken = true;
            ret(getFlagZ());
            break;
        case 0xC9:
            printInstrWSP("RET\n");
            ret();
            break;
        case 0xCA:
            printInstrWSP("JP Z, a16\n");
            jump(getFlagZ(), to16(operand0, operand1));
            break;
        case 0xCB:
            // printInstrWSP("PREFIX CB\n");
            break;
        case 0xCC:
            printInstrWSP("CALL Z, a16\n", operand0, operand1);
            if (getFlagZ()) *taken = true;
            call(getFlagZ(), to16(operand0, operand1));
            break;
        case 0xCD:
            printInstrWSP("CALL a16\n", operand0, operand1);
            call(operand0, operand1);
            break;
        case 0xCE:
            printInstrWSP("ADC A, d8\n", operand0, operand1);
            addNPlusCarry(operand1);
            break;
        case 0xCF:
            printInstrWSP("RES 08H\n");
            rst(0x08);
            break;
        case 0xD0:
            printInstrWSP("RET NC\n");
            if (!getFlagC()) *taken = true;
            ret(!getFlagC());
            break;
        case 0xD1:
            printInstrWSP("POP DE\n");
            pop(&de_.de_);
            break;
        case 0xD2:
            printInstrWSP("JP NC, a16\n");
            if (!getFlagC()) *taken = true;
            jump(!getFlagC(), to16(operand0, operand1));
            break;
        case 0xD4:
            printInstrWSP("CALL NC, a16\n");
            if (!getFlagC()) *taken = true;
            call(!getFlagC(), to16(operand0, operand1));
            break;
        case 0xD5:
            printInstrWSP("PUSH DE\n");
            push(de_.de_);
            break;
        case 0xD6:
            printInstrWSP("SUB d8\n");
            sub(operand1);
            break;
        case 0xD7:
            printInstrWSP("RES 10H\n");
            rst(0x10);
            break;
        case 0xD8:
            printInstrWSP("RET C\n");
            if (getFlagC()) *taken = true;
            ret(getFlagC());
            break;
        case 0xD9:
            printInstrWSP("RETI\n");
            reti();
            break;
        case 0xDA:
            printInstrWSP("JP C, a16\n");
            if (getFlagC()) *taken = true;
            jump(getFlagC(), to16(operand0, operand1));
            break;
        case 0xDC:
            printInstrWSP("CALL C, a16\n");
            if (getFlagC()) *taken = true;
            call(getFlagC(), to16(operand0, operand1));
            break;
        case 0xDE:
            printInstrWSP("SBC A, d8\n");
            subNPlusCarry(operand1);
            break;
        case 0xDF:
            printInstrWSP("RES 18H\n");
            rst(0x18);
            break;
        case 0xE0:
            printInstrWSP("LDH (a8), A\n", operand0, operand1);
            writeIO(operand1, af_.a_);
            break;
        case 0xE1:
            printInstrWSP("POP HL\n");
            pop(&hl_.hl_);
            break;
        case 0xE2:
            printInstrWSP("LD (C), A\n");
            writeIO(bc_.c_, af_.a_);
            break;
        case 0xE5:
            printInstrWSP("PUSH HL\n");
            push(hl_.hl_);
            break;
        case 0xE6:
            printInstrWSP("AND d8\n");
            andA(operand1);
            break;
        case 0xE7:
            printInstrWSP("RST 20H\n");
            rst(0x20);
            break;
        case 0xE8:
            printInstrWSP("ADD SP,r8\n");
            addSP(operand1);
            break;
        case 0xE9:
            printInstrWSP("JP (HL)\n");
            // FIXME: Why is this JP (HL) and not JP HL?
            jump(hl_.hl_);
            break;
        case 0xEA:
            printInstrWSP("LD (a16), A\n");
            writeToMemory(to16(operand0, operand1), af_.a_);
            break;
        case 0xEE:
            printInstrWSP("XOR d8\n");
            xorA(operand1);
            break;
        case 0xEF:
            printInstrWSP("RST 28H\n");
            rst(0x28);
            break;
        case 0xF0:
            printInstrWSP("LDH A, (a8)\n");
            af_.a_ = readFromMemory(0xFF00 + operand1);
            break;
        case 0xF1:
            printInstrWSP("POP AF\n");
            pop(&af_.af_);
            // Clear lower 4 bits of f_
            af_.f_ &= 0xF0;
            break;
        case 0xF2:
            printInstrWSP("LD A, (C)\n");
            af_.a_ = readFromMemory(0xFF00 + bc_.c_);
            break;
        case 0xF3:
            printInstrWSP("DI\n");
            interrupt_master_enable_ = false;
            break;
        case 0xF5:
            printInstrWSP("PUSH AF\n");
            push(af_.af_);
            break;
        case 0xF6:
            printInstrWSP("OR d8\n");
            orA(operand1);
            break;
        case 0xF7:
            printInstrWSP("RES 30H\n");
            rst(0x30);
            break;
        case 0xF8:
            printInstrWSP("LD HL, SP + r8\n");
            ldHLSP(static_cast<int8_t>(operand1));
            break;
        case 0xF9:
            printInstrWSP("LD SP, HL\n");
            sp_ = hl_.hl_;
            break;
        case 0xFA:
            printInstrWSP("LD A, (a16)\n");
            af_.a_ = readFromMemory(to16(operand0, operand1));
            break;
        case 0xFB:
            printInstrWSP("EI\n");
            interrupt_master_enable_ = true;
            break;
        case 0xFE:
            printInstrWSP("CP d8\n");
            compare(operand1);
            break;
        case 0xFF:
            printInstrWSP("RST 38H\n");
            rst(0x38);
            break;
        default:
            fprintf(stderr, "ERROR executing instruction: %02X\n", instruction);
            // sleep(10);
            exit(1);
            return false;
        }
    } else {
        switch (instruction) {
        case 0x00:
            printInstrWSP("RLC B\n");
            rotateLeft(&bc_.b_);
            break;
        case 0x01:
            printInstrWSP("RLC C\n");
            rotateLeft(&bc_.c_);
            break;
        case 0x02:
            printInstrWSP("RLC D\n");
            rotateLeft(&de_.d_);
            break;
        case 0x03:
            printInstrWSP("RLC E\n");
            rotateLeft(&de_.e_);
            break;
        case 0x04:
            printInstrWSP("RLC H\n");
            rotateLeft(&hl_.h_);
            break;
        case 0x05:
            printInstrWSP("RLC L\n");
            rotateLeft(&hl_.l_);
            break;
        case 0x06:
            printInstrWSP("RLC (HL)\n");
            rotateLeft(&ram_[hl_.hl_]);
            break;
        case 0x07:
            printInstrWSP("RLC A\n");
            rotateLeft(&af_.a_);
            break;
        case 0x08:
            printInstrWSP("RRC B\n");
            rotateRight(&bc_.b_);
            break;
        case 0x09:
            printInstrWSP("RRC C\n");
            rotateRight(&bc_.c_);
            break;
        case 0x0A:
            printInstrWSP("RRC D\n");
            rotateRight(&de_.d_);
            break;
        case 0x0B:
            printInstrWSP("RRC E\n");
            rotateRight(&de_.e_);
            break;
        case 0x0C:
            printInstrWSP("RRC H\n");
            rotateRight(&hl_.h_);
            break;
        case 0x0D:
            printInstrWSP("RRC L\n");
            rotateRight(&hl_.l_);
            break;
        case 0x0E:
            printInstrWSP("RRC (HL)\n");
            rotateRight(&ram_[hl_.hl_]);
            break;
        case 0x0F:
            printInstrWSP("RRC A\n");
            rotateRight(&af_.a_);
            break;
        case 0x10:
            printInstrWSP("RL B\n");
            rotateLeftThroughCarry(&bc_.b_);
            break;
        case 0x11:
            printInstrWSP("RL C\n");
            rotateLeftThroughCarry(&bc_.c_);
            break;
        case 0x12:
            printInstrWSP("RL D\n");
            rotateLeftThroughCarry(&de_.d_);
            break;
        case 0x13:
            printInstrWSP("RL E\n");
            rotateLeftThroughCarry(&de_.e_);
            break;
        case 0x14:
            printInstrWSP("RL H\n");
            rotateLeftThroughCarry(&hl_.h_);
            break;
        case 0x15:
            printInstrWSP("RL L\n");
            rotateLeftThroughCarry(&hl_.l_);
            break;
        case 0x16:
            printInstrWSP("RL (HL)\n");
            rotateLeftThroughCarry(&ram_[hl_.hl_]);
            break;
        case 0x17:
            printInstrWSP("RL A\n");
            rotateLeftThroughCarry(&af_.a_);
            break;
        case 0x18:
            printInstrWSP("RR B\n");
            rotateRightThroughCarry(&bc_.b_);
            break;
        case 0x19:
            printInstrWSP("RR C\n");
            rotateRightThroughCarry(&bc_.c_);
            break;
        case 0x1A:
            printInstrWSP("RR D\n");
            rotateRightThroughCarry(&de_.d_);
            break;
        case 0x1B:
            printInstrWSP("RR E\n");
            rotateRightThroughCarry(&de_.e_);
            break;
        case 0x1C:
            printInstrWSP("RR H\n");
            rotateRightThroughCarry(&hl_.h_);
            break;
        case 0x1D:
            printInstrWSP("RR L\n");
            rotateRightThroughCarry(&hl_.l_);
            break;
        case 0x1E:
            printInstrWSP("RR (HL)\n");
            rotateRightThroughCarry(&ram_[hl_.hl_]);
            break;
        case 0x1F:
            printInstrWSP("RR A\n");
            rotateRightThroughCarry(&af_.a_);
            break;
        case 0x20:
            printInstrWSP("SLA B\n");
            shiftLeftIntoCarry(&bc_.b_);
            break;
        case 0x21:
            printInstrWSP("SLA C\n");
            shiftLeftIntoCarry(&bc_.c_);
            break;
        case 0x22:
            printInstrWSP("SLA D\n");
            shiftLeftIntoCarry(&de_.d_);
            break;
        case 0x23:
            printInstrWSP("SLA E\n");
            shiftLeftIntoCarry(&de_.e_);
            break;
        case 0x24:
            printInstrWSP("SLA H\n");
            shiftLeftIntoCarry(&hl_.h_);
            break;
        case 0x25:
            printInstrWSP("SLA L\n");
            shiftLeftIntoCarry(&hl_.l_);
            break;
        case 0x26:
            printInstrWSP("SLA (HL)\n");
            shiftLeftIntoCarry(&ram_[hl_.hl_]);
            break;
        case 0x27:
            printInstrWSP("SLA A\n");
            shiftLeftIntoCarry(&af_.a_);
            break;
        case 0x28:
            printInstrWSP("SRA B\n");
            shiftRightIntoCarryMSB(&bc_.b_);
            break;
        case 0x29:
            printInstrWSP("SRA C\n");
            shiftRightIntoCarryMSB(&bc_.c_);
            break;
        case 0x2A:
            printInstrWSP("SRA D\n");
            shiftRightIntoCarryMSB(&de_.d_);
            break;
        case 0x2B:
            printInstrWSP("SRA E\n");
            shiftRightIntoCarryMSB(&de_.e_);
            break;
        case 0x2C:
            printInstrWSP("SRA H\n");
            shiftRightIntoCarryMSB(&hl_.h_);
            break;
        case 0x2D:
            printInstrWSP("SRA L\n");
            shiftRightIntoCarryMSB(&hl_.l_);
            break;
        case 0x2E:
            printInstrWSP("SRA (HL)\n");
            shiftRightIntoCarryMSB(&ram_[hl_.hl_]);
            break;
        case 0x2F:
            printInstrWSP("SRA A\n");
            shiftRightIntoCarryMSB(&af_.a_);
            break;
        case 0x30:
            printInstrWSP("SWAP B\n");
            swap(&bc_.b_);
            break;
        case 0x31:
            printInstrWSP("SWAP C\n");
            swap(&bc_.c_);
            break;
        case 0x32:
            printInstrWSP("SWAP D\n");
            swap(&de_.d_);
            break;
        case 0x33:
            printInstrWSP("SWAP E\n");
            swap(&de_.e_);
            break;
        case 0x34:
            printInstrWSP("SWAP H\n");
            swap(&hl_.h_);
            break;
        case 0x35:
            printInstrWSP("SWAP L\n");
            swap(&hl_.l_);
            break;
        case 0x36:
            printInstrWSP("SWAP (HL)\n");
            swap(&ram_[hl_.hl_]);
            break;
        case 0x37:
            printInstrWSP("SWAP A\n");
            swap(&af_.a_);
            break;
        case 0x38:
            printInstrWSP("SRL B\n");
            shiftRightIntoCarry(&bc_.b_);
            break;
        case 0x39:
            printInstrWSP("SRL C\n");
            shiftRightIntoCarry(&bc_.c_);
            break;
        case 0x3A:
            printInstrWSP("SRL D\n");
            shiftRightIntoCarry(&de_.d_);
            break;
        case 0x3B:
            printInstrWSP("SRL E\n");
            shiftRightIntoCarry(&de_.e_);
            break;
        case 0x3C:
            printInstrWSP("SRL H\n");
            shiftRightIntoCarry(&hl_.h_);
            break;
        case 0x3D:
            printInstrWSP("SRL L\n");
            shiftRightIntoCarry(&hl_.l_);
            break;
        case 0x3E:
            printInstrWSP("SRL (HL)\n");
            shiftRightIntoCarry(&ram_[hl_.hl_]);
            break;
        case 0x3F:
            printInstrWSP("SRL A\n");
            shiftRightIntoCarry(&af_.a_);
            break;
        case 0x40:
            printInstrWSP("BIT 0, B\n");
            testBit(bc_.b_, 0);
            break;
        case 0x41:
            printInstrWSP("BIT 0, C\n");
            testBit(bc_.c_, 0);
            break;
        case 0x42:
            printInstrWSP("BIT 0, D\n");
            testBit(de_.d_, 0);
            break;
        case 0x43:
            printInstrWSP("BIT 0, E\n");
            testBit(de_.e_, 0);
            break;
        case 0x44:
            printInstrWSP("BIT 0, H\n");
            testBit(hl_.h_, 0);
            break;
        case 0x45:
            printInstrWSP("BIT 0, L\n");
            testBit(hl_.l_, 0);
            break;
        case 0x46:
            printInstrWSP("BIT 0, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 0);
            break;
        case 0x47:
            printInstrWSP("BIT 0, A\n");
            testBit(af_.a_, 0);
            break;
        case 0x49:
            printInstrWSP("BIT 1, C\n");
            testBit(bc_.c_, 1);
            break;
        case 0x4A:
            printInstrWSP("BIT 1, D\n");
            testBit(de_.d_, 1);
            break;
        case 0x4B:
            printInstrWSP("BIT 1, E\n");
            testBit(de_.e_, 1);
            break;
        case 0x4C:
            printInstrWSP("BIT 1, H\n");
            testBit(hl_.h_, 1);
            break;
        case 0x4D:
            printInstrWSP("BIT 1, L\n");
            testBit(hl_.l_, 1);
            break;
        case 0x4E:
            printInstrWSP("BIT 1, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 1);
            break;
        case 0x4F:
            printInstrWSP("BIT 1, A\n");
            testBit(af_.a_, 1);
            break;
        case 0x48:
            printInstrWSP("BIT 1, B\n");
            testBit(bc_.b_, 1);
            break;
        case 0x50:
            printInstrWSP("BIT 2, B\n");
            testBit(bc_.b_, 2);
            break;
        case 0x51:
            printInstrWSP("BIT 2, C\n");
            testBit(bc_.c_, 2);
            break;
        case 0x52:
            printInstrWSP("BIT 2, D\n");
            testBit(de_.d_, 2);
            break;
        case 0x53:
            printInstrWSP("BIT 2, E\n");
            testBit(de_.e_, 2);
            break;
        case 0x54:
            printInstrWSP("BIT 2, H\n");
            testBit(hl_.h_, 2);
            break;
        case 0x55:
            printInstrWSP("BIT 2, L\n");
            testBit(hl_.l_, 2);
            break;
        case 0x56:
            printInstrWSP("BIT 2, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 2);
            break;
        case 0x57:
            printInstrWSP("BIT 2, A\n");
            testBit(af_.a_, 2);
            break;
        case 0x58:
            printInstrWSP("BIT 3, B\n");
            testBit(bc_.b_, 3);
            break;
        case 0x59:
            printInstrWSP("BIT 3, C\n");
            testBit(bc_.c_, 3);
            break;
        case 0x5A:
            printInstrWSP("BIT 3, D\n");
            testBit(de_.d_, 3);
            break;
        case 0x5B:
            printInstrWSP("BIT 3, E\n");
            testBit(de_.e_, 3);
            break;
        case 0x5C:
            printInstrWSP("BIT 3, H\n");
            testBit(hl_.h_, 3);
            break;
        case 0x5D:
            printInstrWSP("BIT 3, L\n");
            testBit(hl_.l_, 3);
            break;
        case 0x5E:
            printInstrWSP("BIT 3, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 3);
            break;
        case 0x5F:
            printInstrWSP("BIT 3, A\n");
            testBit(af_.a_, 3);
            break;
        case 0x60:
            printInstrWSP("BIT 4, B\n");
            testBit(bc_.b_, 4);
            break;
        case 0x61:
            printInstrWSP("BIT 4, C\n");
            testBit(bc_.c_, 4);
            break;
        case 0x62:
            printInstrWSP("BIT 4, D\n");
            testBit(de_.d_, 4);
            break;
        case 0x63:
            printInstrWSP("BIT 4, E\n");
            testBit(de_.e_, 4);
            break;
        case 0x64:
            printInstrWSP("BIT 4, H\n");
            testBit(hl_.h_, 4);
            break;
        case 0x65:
            printInstrWSP("BIT 4, L\n");
            testBit(hl_.l_, 4);
            break;
        case 0x66:
            printInstrWSP("BIT 4, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 4);
            break;
        case 0x67:
            printInstrWSP("BIT 4, A\n");
            testBit(af_.a_, 4);
            break;
        case 0x68:
            printInstrWSP("BIT 5, B\n");
            testBit(bc_.b_, 5);
            break;
        case 0x69:
            printInstrWSP("BIT 5, C\n");
            testBit(bc_.c_, 5);
            break;
        case 0x6A:
            printInstrWSP("BIT 5, D\n");
            testBit(de_.d_, 5);
            break;
        case 0x6B:
            printInstrWSP("BIT 5, E\n");
            testBit(de_.e_, 5);
            break;
        case 0x6C:
            printInstrWSP("BIT 5, H\n");
            testBit(hl_.h_, 5);
            break;
        case 0x6D:
            printInstrWSP("BIT 5, L\n");
            testBit(hl_.l_, 5);
            break;
        case 0x6E:
            printInstrWSP("BIT 5, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 5);
            break;
        case 0x6F:
            printInstrWSP("BIT 5, A\n");
            testBit(af_.a_, 5);
            break;
        case 0x70:
            printInstrWSP("BIT 6, B\n");
            testBit(bc_.b_, 6);
            break;
        case 0x71:
            printInstrWSP("BIT 6, C\n");
            testBit(bc_.c_, 6);
            break;
        case 0x72:
            printInstrWSP("BIT 6, D\n");
            testBit(de_.d_, 6);
            break;
        case 0x73:
            printInstrWSP("BIT 6, E\n");
            testBit(de_.e_, 6);
            break;
        case 0x74:
            printInstrWSP("BIT 6, H\n");
            testBit(hl_.h_, 6);
            break;
        case 0x75:
            printInstrWSP("BIT 6, L\n");
            testBit(hl_.l_, 6);
            break;
        case 0x76:
            printInstrWSP("BIT 6, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 6);
            break;
        case 0x79:
            printInstrWSP("BIT 7, C\n");
            testBit(bc_.c_, 7);
            break;
        case 0x7A:
            printInstrWSP("BIT 7, D\n");
            testBit(de_.d_, 7);
            break;
        case 0x7B:
            printInstrWSP("BIT 7, E\n");
            testBit(de_.e_, 7);
            break;
        case 0x7C:
            printInstrWSP("BIT 7,H\n");
            testBit(hl_.h_, 7);
            break;
        case 0x7D:
            printInstrWSP("BIT 7, L\n");
            testBit(hl_.l_, 7);
            break;
        case 0x7E:
            printInstrWSP("BIT 7, (HL)\n");
            testBit(readFromMemory(hl_.hl_), 7);
            break;
        case 0x77:
            printInstrWSP("BIT 6, A\n");
            testBit(af_.a_, 6);
            break;
        case 0x78:
            printInstrWSP("BIT 7, B\n");
            testBit(bc_.b_, 7);
            break;
        case 0x7F:
            printInstrWSP("BIT 7, A\n");
            testBit(af_.a_, 7);
            break;
        case 0x80:
            printInstrWSP("RES 0, B\n");
            resetBit(&bc_.b_, 0);
            break;
        case 0x81:
            printInstrWSP("RES 0, C\n");
            resetBit(&bc_.c_, 0);
            break;
        case 0x82:
            printInstrWSP("RES 0, D\n");
            resetBit(&de_.d_, 0);
            break;
        case 0x83:
            printInstrWSP("RES 0, E\n");
            resetBit(&de_.e_, 0);
            break;
        case 0x84:
            printInstrWSP("RES 0, H\n");
            resetBit(&hl_.h_, 0);
            break;
        case 0x85:
            printInstrWSP("RES 0, L\n");
            resetBit(&hl_.l_, 0);
            break;
        case 0x86:
            printInstrWSP("RES 0, (HL)\n");
            // FIXME: this RAM write can't be traced
            resetBit(&ram_[hl_.hl_], 0);
            break;
        case 0x87:
            printInstrWSP("RES 0, A\n");
            resetBit(&af_.a_, 0);
            break;
        case 0x88:
            printInstrWSP("RES 1, B\n");
            resetBit(&bc_.b_, 1);
            break;
        case 0x89:
            printInstrWSP("RES 1, C\n");
            resetBit(&bc_.c_, 1);
            break;
        case 0x8A:
            printInstrWSP("RES 1, D\n");
            resetBit(&de_.d_, 1);
            break;
        case 0x8B:
            printInstrWSP("RES 1, E\n");
            resetBit(&de_.e_, 1);
            break;
        case 0x8C:
            printInstrWSP("RES 1, H\n");
            resetBit(&hl_.h_, 1);
            break;
        case 0x8D:
            printInstrWSP("RES 1, L\n");
            resetBit(&hl_.l_, 1);
            break;
        case 0x8E:
            printInstrWSP("RES 1, (HL)\n");
            resetBit(&ram_[hl_.hl_], 1);
            break;
        case 0x8F:
            printInstrWSP("RES 1, A\n");
            resetBit(&af_.a_, 1);
            break;
        case 0x90:
            printInstrWSP("RES 2, B\n");
            resetBit(&bc_.b_, 2);
            break;
        case 0x91:
            printInstrWSP("RES 2, C\n");
            resetBit(&bc_.c_, 2);
            break;
        case 0x92:
            printInstrWSP("RES 2, D\n");
            resetBit(&de_.d_, 2);
            break;
        case 0x93:
            printInstrWSP("RES 2, E\n");
            resetBit(&de_.e_, 2);
            break;
        case 0x94:
            printInstrWSP("RES 2, H\n");
            resetBit(&hl_.h_, 2);
            break;
        case 0x95:
            printInstrWSP("RES 2, L\n");
            resetBit(&hl_.l_, 2);
            break;
        case 0x96:
            printInstrWSP("RES 2, (HL)\n");
            resetBit(&ram_[hl_.hl_], 2);
            break;
        case 0x97:
            printInstrWSP("RES 2, A\n");
            resetBit(&af_.a_, 2);
            break;
        case 0x98:
            printInstrWSP("RES 3, B\n");
            resetBit(&bc_.b_, 3);
            break;
        case 0x99:
            printInstrWSP("RES 3, C\n");
            resetBit(&bc_.c_, 3);
            break;
        case 0x9A:
            printInstrWSP("RES 3, D\n");
            resetBit(&de_.d_, 3);
            break;
        case 0x9B:
            printInstrWSP("RES 3, E\n");
            resetBit(&de_.e_, 3);
            break;
        case 0x9C:
            printInstrWSP("RES 3, H\n");
            resetBit(&hl_.h_, 3);
            break;
        case 0x9D:
            printInstrWSP("RES 3, L\n");
            resetBit(&hl_.l_, 3);
            break;
        case 0x9E:
            printInstrWSP("RES 3, (HL)\n");
            resetBit(&ram_[hl_.hl_], 3);
            break;
        case 0x9F:
            printInstrWSP("RES 3, A\n");
            resetBit(&af_.a_, 3);
            break;
        case 0xA0:
            printInstrWSP("RES 4, B\n");
            resetBit(&bc_.b_, 4);
            break;
        case 0xA1:
            printInstrWSP("RES 4, C\n");
            resetBit(&bc_.c_, 4);
            break;
        case 0xA2:
            printInstrWSP("RES 4, D\n");
            resetBit(&de_.d_, 4);
            break;
        case 0xA3:
            printInstrWSP("RES 4, E\n");
            resetBit(&de_.e_, 4);
            break;
        case 0xA4:
            printInstrWSP("RES 4, H\n");
            resetBit(&hl_.h_, 4);
            break;
        case 0xA5:
            printInstrWSP("RES 4, L\n");
            resetBit(&hl_.l_, 4);
            break;
        case 0xA6:
            printInstrWSP("RES 4, (HL)\n");
            resetBit(&ram_[hl_.hl_], 4);
            break;
        case 0xA7:
            printInstrWSP("RES 4, A\n");
            resetBit(&af_.a_, 4);
            break;
        case 0xA8:
            printInstrWSP("RES 5, B\n");
            resetBit(&bc_.b_, 5);
            break;
        case 0xA9:
            printInstrWSP("RES 5, C\n");
            resetBit(&bc_.c_, 5);
            break;
        case 0xAA:
            printInstrWSP("RES 5, D\n");
            resetBit(&de_.d_, 5);
            break;
        case 0xAB:
            printInstrWSP("RES 5, E\n");
            resetBit(&de_.e_, 5);
            break;
        case 0xAC:
            printInstrWSP("RES 5, H\n");
            resetBit(&hl_.h_, 5);
            break;
        case 0xAD:
            printInstrWSP("RES 5, L\n");
            resetBit(&hl_.l_, 5);
            break;
        case 0xAF:
            printInstrWSP("RES 5, A\n");
            resetBit(&af_.a_, 5);
            break;
        case 0xAE:
            printInstrWSP("RES 5, (HL)\n");
            // FIXME: this RAM write can't be traced
            resetBit(&ram_[hl_.hl_], 5);
            break;
        case 0xB0:
            printInstrWSP("RES 6, B\n");
            resetBit(&bc_.b_, 6);
            break;
        case 0xB1:
            printInstrWSP("RES 6, C\n");
            resetBit(&bc_.c_, 6);
            break;
        case 0xB2:
            printInstrWSP("RES 6, D\n");
            resetBit(&de_.d_, 6);
            break;
        case 0xB3:
            printInstrWSP("RES 6, E\n");
            resetBit(&de_.e_, 6);
            break;
        case 0xB4:
            printInstrWSP("RES 6, H\n");
            resetBit(&hl_.h_, 6);
            break;
        case 0xB5:
            printInstrWSP("RES 6, L\n");
            resetBit(&hl_.l_, 6);
            break;
        case 0xB6:
            printInstrWSP("RES 6, (HL)\n");
            resetBit(&ram_[hl_.hl_], 6);
            break;
        case 0xB7:
            printInstrWSP("RES 6, A\n");
            resetBit(&af_.a_, 6);
            break;
        case 0xB8:
            printInstrWSP("RES 7, B\n");
            resetBit(&bc_.b_, 7);
            break;
        case 0xB9:
            printInstrWSP("RES 7, C\n");
            resetBit(&bc_.c_, 7);
            break;
        case 0xBA:
            printInstrWSP("RES 7, D\n");
            resetBit(&de_.d_, 7);
            break;
        case 0xBB:
            printInstrWSP("RES 7, E\n");
            resetBit(&de_.e_, 7);
            break;
        case 0xBC:
            printInstrWSP("RES 7, H\n");
            resetBit(&hl_.h_, 7);
            break;
        case 0xBD:
            printInstrWSP("RES 7, L\n");
            resetBit(&hl_.l_, 7);
            break;
        case 0xBE:
            printInstrWSP("RES 7, (HL)\n");
            // FIXME: this RAM write can't be traced
            resetBit(&ram_[hl_.hl_], 7);
            break;
        case 0xBF:
            printInstrWSP("RES 7, A\n");
            resetBit(&af_.a_, 7);
            break;
        case 0xC0:
            printInstrWSP("SET 0, B\n");
            setBit(&bc_.b_, 0);
            break;
        case 0xC1:
            printInstrWSP("SET 0, C\n");
            setBit(&bc_.c_, 0);
            break;
        case 0xC2:
            printInstrWSP("SET 0, D\n");
            setBit(&de_.d_, 0);
            break;
        case 0xC3:
            printInstrWSP("SET 0, E\n");
            setBit(&de_.e_, 0);
            break;
        case 0xC4:
            printInstrWSP("SET 0, H\n");
            setBit(&hl_.h_, 0);
            break;
        case 0xC5:
            printInstrWSP("SET 0, L\n");
            setBit(&hl_.l_, 0);
            break;
        case 0xC6:
            printInstrWSP("SET 0, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 0);
            break;
        case 0xC7:
            printInstrWSP("SET 0, A\n");
            setBit(&af_.a_, 0);
            break;
        case 0xC8:
            printInstrWSP("SET 1, B\n");
            setBit(&bc_.b_, 1);
            break;
        case 0xC9:
            printInstrWSP("SET 1, C\n");
            setBit(&bc_.c_, 1);
            break;
        case 0xCA:
            printInstrWSP("SET 1, D\n");
            setBit(&de_.d_, 1);
            break;
        case 0xCB:
            printInstrWSP("SET 1, E\n");
            setBit(&de_.e_, 1);
            break;
        case 0xCC:
            printInstrWSP("SET 1, H\n");
            setBit(&hl_.h_, 1);
            break;
        case 0xCD:
            printInstrWSP("SET 1, L\n");
            setBit(&hl_.l_, 1);
            break;
        case 0xCE:
            printInstrWSP("SET 1, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 1);
            break;
        case 0xCF:
            printInstrWSP("SET 1, A\n");
            setBit(&af_.a_, 1);
            break;
        case 0xD0:
            printInstrWSP("SET 2, B\n");
            setBit(&bc_.b_, 2);
            break;
        case 0xD1:
            printInstrWSP("SET 2, C\n");
            setBit(&bc_.c_, 2);
            break;
        case 0xD2:
            printInstrWSP("SET 2, D\n");
            setBit(&de_.d_, 2);
            break;
        case 0xD3:
            printInstrWSP("SET 2, E\n");
            setBit(&de_.e_, 2);
            break;
        case 0xD4:
            printInstrWSP("SET 2, H\n");
            setBit(&hl_.h_, 2);
            break;
        case 0xD5:
            printInstrWSP("SET 2, L\n");
            setBit(&hl_.l_, 2);
            break;
        case 0xD6:
            printInstrWSP("SET 2, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 2);
            break;
        case 0xD7:
            printInstrWSP("SET 2, A\n");
            setBit(&af_.a_, 2);
            break;
        case 0xD8:
            printInstrWSP("SET 3, B\n");
            setBit(&bc_.b_, 3);
            break;
        case 0xD9:
            printInstrWSP("SET 3, C\n");
            setBit(&bc_.c_, 3);
            break;
        case 0xDA:
            printInstrWSP("SET 3, D\n");
            setBit(&de_.d_, 3);
            break;
        case 0xDB:
            printInstrWSP("SET 3, E\n");
            setBit(&de_.e_, 3);
            break;
        case 0xDC:
            printInstrWSP("SET 3, H\n");
            setBit(&hl_.h_, 3);
            break;
        case 0xDD:
            printInstrWSP("SET 3, L\n");
            setBit(&hl_.l_, 3);
            break;
        case 0xDE:
            printInstrWSP("SET 3, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 3);
            break;
        case 0xDF:
            printInstrWSP("SET 3, A\n");
            setBit(&af_.a_, 3);
            break;
        case 0xE0:
            printInstrWSP("SET 4, B\n");
            setBit(&bc_.b_, 4);
            break;
        case 0xE1:
            printInstrWSP("SET 4, C\n");
            setBit(&bc_.c_, 4);
            break;
        case 0xE2:
            printInstrWSP("SET 4, D\n");
            setBit(&de_.d_, 4);
            break;
        case 0xE3:
            printInstrWSP("SET 4, E\n");
            setBit(&de_.e_, 4);
            break;
        case 0xE4:
            printInstrWSP("SET 4, H\n");
            setBit(&hl_.h_, 4);
            break;
        case 0xE5:
            printInstrWSP("SET 4, L\n");
            setBit(&hl_.l_, 4);
            break;
        case 0xE6:
            printInstrWSP("SET 4, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 4);
            break;
        case 0xE7:
            printInstrWSP("SET 4, A\n");
            setBit(&af_.a_, 4);
            break;
        case 0xE8:
            printInstrWSP("SET 5, B\n");
            setBit(&bc_.b_, 5);
            break;
        case 0xE9:
            printInstrWSP("SET 5, C\n");
            setBit(&bc_.c_, 5);
            break;
        case 0xEA:
            printInstrWSP("SET 5, D\n");
            setBit(&de_.d_, 5);
            break;
        case 0xEB:
            printInstrWSP("SET 5, E\n");
            setBit(&de_.e_, 5);
            break;
        case 0xEC:
            printInstrWSP("SET 5, H\n");
            setBit(&hl_.h_, 5);
            break;
        case 0xED:
            printInstrWSP("SET 5, L\n");
            setBit(&hl_.l_, 5);
            break;
        case 0xEE:
            printInstrWSP("SET 5, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 5);
            break;
        case 0xEF:
            printInstrWSP("SET 5, A\n");
            setBit(&af_.a_, 5);
            break;
        case 0xF0:
            printInstrWSP("SET 6, B\n");
            setBit(&bc_.b_, 6);
            break;
        case 0xF1:
            printInstrWSP("SET 6, C\n");
            setBit(&bc_.c_, 6);
            break;
        case 0xF2:
            printInstrWSP("SET 6, D\n");
            setBit(&de_.d_, 6);
            break;
        case 0xF3:
            printInstrWSP("SET 6, E\n");
            setBit(&de_.e_, 6);
            break;
        case 0xF4:
            printInstrWSP("SET 6, H\n");
            setBit(&hl_.h_, 6);
            break;
        case 0xF5:
            printInstrWSP("SET 6, L\n");
            setBit(&hl_.l_, 6);
            break;
        case 0xF6:
            printInstrWSP("SET 6, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 6);
            break;
        case 0xF7:
            printInstrWSP("SET 6, A\n");
            setBit(&af_.a_, 6);
            break;
        case 0xF8:
            printInstrWSP("SET 7, B\n");
            setBit(&bc_.b_, 7);
            break;
        case 0xF9:
            printInstrWSP("SET 7, C\n");
            setBit(&bc_.c_, 7);
            break;
        case 0xFA:
            printInstrWSP("SET 7, D\n");
            setBit(&de_.d_, 7);
            break;
        case 0xFB:
            printInstrWSP("SET 7, E\n");
            setBit(&de_.e_, 7);
            break;
        case 0xFC:
            printInstrWSP("SET 7, H\n");
            setBit(&hl_.h_, 7);
            break;
        case 0xFD:
            printInstrWSP("SET 7, L\n");
            setBit(&hl_.l_, 7);
            break;
        case 0xFE:
            printInstrWSP("SET 7, (HL)\n");
            // FIXME: this RAM write can't be traced
            setBit(&ram_[hl_.hl_], 7);
            break;
        case 0xFF:
            printInstrWSP("SET 7, A\n");
            setBit(&af_.a_, 7);
            break;
        default:
            fprintf(stderr, "ERROR executing prefix instruction: %02X\n", instruction);
            sleep(10);
            exit(1);
            return false;
        }
    }
    // printInfo();
    return true;
}

void CPU::printInfo() {
    ppu_.printTileMemory();
    ppu_.printTile(0);
    ppu_.printTile(1);
    ppu_.printTile(2);
}

uint8_t CPU::readFromMemory(uint16_t address) const {
    if (address <= 0x3FFF) {
        return rom_[address];
    }
    if (address >= 0x4000 && address <= 0x7FFF)
    {
        uint16_t new_addr;
        switch (cartridge_type_) {
        case 0x00:
            // ROM only
            new_addr = address;
            break;
        case 0x01:
        case 0x02:
            // MBC1 ROM bank handling
            // FIXME: does this need more bits?
            // FIXME: case 2 has ram
            new_addr = mbc_rombank_ * 0x4000 + (address & 0x3FFF);
            break;
        default:
            fprintf(stderr, "Unknown cartridge type.\n");
            exit(1);
        }

        return rom_[new_addr];
    }
    if (address == 0xFF00) {
        // Read from buttons (set unused upper 2 bits to 1)
        if (getBitN(ram_[address], 4) == 0) {
            return joypad_->getDirections() | 0xC0;
        } else {
            return joypad_->getButtons() | 0xC0;
        }
    }
    if (address == 0xFF03 || address == 0xFF08 || address == 0xFF09 || address == 0xFF0A ||
        address == 0xFF0B || address == 0xFF0C || address == 0xFF0D || address == 0xFF0E ||
        address == 0xFF15 || address == 0xFF1F || address == 0xFF27 || address == 0xFF28 ||
        address == 0xFF29)
    {
        // Unused, always FF
        return 0xFF;
    }
    if (address >= 0xFF4C && address <= 0xFF7F) {
        // Unused, always FF
        return 0xFF;
    }
    if (address == 0xFF0F) {
        // Read from IF (upper 3 bits are always 1)
        return ram_[address] | 0xE0;
    }
    return ram_[address];
}

void CPU::writeToMemory(uint16_t address, uint8_t value) {
    if (address <= 0x1FFF) {
        return;
        // printf("Enable external RAM %02X\n", value);
        // fflush(stdout);
        // exit(1);
    }
    if (address >= 0x2000 && address <= 0x3FFF) {
        // printf("ROM bank low: %02X\n", value);
        // fflush(stdout);
        // Select lower 5 bits
        value &= 0x1F;
        if (value == 0x00) value = 0x01;
        mbc_rombank_ = value;
    }
    if (address >= 0x4000 && address <= 0x5FFF) {
        printf("ROM bank high: %02X\n", value);
        fflush(stdout);
        exit(1);
    }
    if (address >= 0x6000 && address <= 0x7FFF) {
        printf("Mode value: %02X\n", value);
        fflush(stdout);
        if (value == 0x00) {
            // ROM banking mode (default)
            mbc_mode_ = 0;
        } else {
            // RAM banking mode
            mbc_mode_ = 1;
            fprintf(stderr, "RAM banking mode.\n");
            exit(1);
        }
    }
    if (address >= 0x8000 && address <= 0x97FF) {
        if (value != 0) {
            // printf("[writeToMemory] VRAM tile address: %04X value: %02X\n", address, value);
            // fflush(stdout);
            // ppu_.showTiles();
        }
        /* if (address % 32 == 0) {

        } */
    }
    if (address >= 0x9800 && address <= 0x9BFF) {
        if (value != 0) {
            // printf("[writeToMemory] VRAM BG Map 1: %04X value: %02X\n", address, value);
            // fflush(stdout);
            // ppu_.showDisplay();
        }
        /* if (address % 16 == 0) {

        } */
    }
    if (address >= 0x9C00 && address <= 0x9FFF) {
        if (value != 0) {
            // printf("[writeToMemory] VRAM BG Map 2: %04X value: %02X\n", address, value);
            // fflush(stdout);
            // ppu_.showDisplay();
        }
        /* if (address % 16 == 0) {

        } */
    }
    if (address >= 0xFF00) {
        if (address == 0xFF02) {
            // Serial transfer control
            ram_[0xFF02] = (value | 0x7E);
            return;
        }
        if (address == 0xFF04) {
            // Writing to DIV always resets it
            ram_[0xFF04] = 0;
            return;
        }
        if (address == 0xFF07) {
            // Timer control TAC (upper 5 bits are always high)
            ram_[0xFF07] = (value | 0xF8);
            return;
        }
        if (address == 0xFF10) {
            // NR10 - Channel 1 Sweep register (bit 7 is always high)
            ram_[0xFF10] = (value | 0x80);
            return;
        }
        if (address == 0xFF1A) {
            // NR30 - Channel 3 Sound on/off (lower 7 bits are always high)
            ram_[0xFF1A] = (value | 0x7F);
            return;
        }
        if (address == 0xFF1C) {
            // NR32 - Channel 3 Select output level (only bit 5 and 6 are used)
            ram_[0xFF1C] = (value | 0x9F);
            return;
        }
        if (address == 0xFF20) {
            // NR41 - Channel 4 Sound Length (bit 6 and 7 are always high)
            ram_[0xFF20] = (value | 0xC0);
            return;
        }
        if (address == 0xFF23) {
            // NR44 - Channel 4 Counter/consecutive; Inital (bit 0-5 high)
            ram_[0xFF23] = (value | 0x3F);
            return;
        }
        if (address == 0xFF26) {
            // FF26 - NR52 - Sound on/off (bit 4-6 high)
            ram_[0xFF26] = (value | 0x70);
            return;
        }
        if (address == 0xFF41) {
            // LCD Status STAT (bit 7 is always high
            ram_[0xFF41] = (value |= 0x80);
            return;
        }
        if (address == 0xFF44) {
            // Writing to LY always resets it
            ram_[0xFF44] = 0;
            return;
        }
        if (address == 0xFF46) {
            // OAM DMA
            // Starts a memory transfer
            // Source:      XX00-XX9F   ;XX in range from 00-F1h
            // Destination: FE00-FE9F
            // FIXME: This takes 4 clocks for every byte
            for (uint8_t k = 0; k <= 0x9F; ++k) {
                uint16_t src = to16(k, value);
                uint16_t dst = to16(k, 0xFE);
                writeToMemory(dst, readFromMemory(src));
            }
            return;
        }
        // debugIO(address, value);
    }
    if (address == 0xFF00) {
        // Write to buttons
        uint8_t button_state = ram_[address] & 0x0F;
        ram_[address] = (value & 0xF0) | button_state; // lower 4 bits are read only
        // ram_[address] |= 0x0F; // buttons are always not pressed
        ram_[address] |= 0xC0; // unused bits are always 1
        return;
    }
    fflush(stdout);
    ram_[address] = value;
}

void debugIO(uint16_t address, uint8_t value) {
    printf("[writeToMemory] IO address: %04X value: %02X ", address, value);
    fflush(stdout);
    switch (address) {
    case 0xFF01:
        printf("Serial transfer data\n");
        break;
    case 0xFF02:
        printf("Serial transfer control\n");
        break;
    case 0xFF07:
        printf("TAC Timer control\n");
        break;
    case 0xFF0F:
        printf("IF Interrupt flag\n");
        break;
    case 0xFF24:
        printf("NR50 channel control\n");
        break;
    case 0xFF25:
        printf("NR51 sound output\n");
        break;
    case 0xFF26:
        printf("NR52 sound on/off\n");
        break;
    case 0xFF40:
        printf("LCDC\n");
        break;
    case 0xFF42:
        printf("SCY\n");
        break;
    case 0xFF43:
        printf("SCX\n");
        break;
    case 0xFF47:
        printf("BG palette data\n");
        break;
    case 0xFF46:
        printf("OAM\n");
        break;
    case 0xFF80:
    case 0xFF81:
    case 0xFF82:
    case 0xFF83:
        printf("High RAM\n");
        break;
    case 0xFFFF:
        printf("IE Interrupt enable\n");
        break;
    default:
        printf("default \n");
        break;
    }
}

void CPU::printStack() const {
    for (uint16_t addr = 0xFFFE; addr > 0XFFF0; --addr) {
        if (addr == sp_) {
            printf("%04X: %02x <--\n", addr, ram_[addr]);
        } else {
            printf("%04X: %02x\n", addr, ram_[addr]);
        }
    }
    fflush(stdout);
}

void CPU::loadSP(uint8_t operand0, uint8_t operand1) {
    // little endian
    uint8_t* spp = reinterpret_cast<uint8_t*>(&sp_);
    spp[0] = operand0;
    spp[1] = operand1;
}

void CPU::writeIO(uint8_t offset, uint8_t value) {
    // printf("[IO] offset: %02X value: %02X\n", offset, value);
    if (offset == 0x02 && value == 0x81) {
        // printf("[GameLink] value: %c\n", ram_[0xFF01]);
        // printf("%c", readFromMemory(0xFF01));
    }
    // fflush(stdout);
    writeToMemory(0xFF00 + offset, value);
}

void CPU::loadAddr(uint16_t addr, uint8_t value) {
    writeToMemory(addr, value);
}

void CPU::loadDecAddr(uint16_t* addr, uint8_t value) {
    // load value into addr and decrement addr
    writeToMemory(*addr, value);
    (*addr)--;
}

void CPU::loadIncAddr(uint16_t* addr, uint8_t value) {
    // load value into addr and increment addr
    writeToMemory(*addr, value);
    (*addr)++;
}

void CPU::loadAHLPlus() {
    af_.a_ = readFromMemory(hl_.hl_);
    hl_.hl_++;
}

void CPU::testBit(uint8_t reg, uint8_t number) {
    uint8_t test = (reg >> number) & 1;
    if (test == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    setFlagH();
}

void CPU::setBit(uint8_t* reg, uint8_t number) {
    *reg |= static_cast<uint8_t>(1 << number);
}

void CPU::resetBit(uint8_t* reg, uint8_t number) {
    *reg &= static_cast<uint8_t>(~(1 << number));
}

void CPU::flipAllBits(uint8_t* reg) {
    *reg = ~(*reg);
    setFlagN();
    setFlagH();
}

void CPU::compare(uint8_t value) {
    uint8_t result = af_.a_ - value;
    uint8_t h = checkHalfCarrySub(af_.a_, value);

    if (result == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    setFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }
    if (af_.a_ < value) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

void CPU::halt() {
    // Do nothing, wait for interrupts
    uint8_t i_enable = ram_[0xFFFF];
    uint8_t i_flags = ram_[0xFF0F];
    if (interrupt_master_enable_ == false && (i_enable & i_flags & 0x1F) != 0) {
        // Halt bug
        halted_ = false;
        halt_bug_ = true;
    } else {
        halted_ = true;
    }
}

void CPU::jump(uint16_t address) {
    pc_ = address;
}

void CPU::jump(uint8_t condition, int8_t offset) {
    if (condition == 1) {
        pc_ += offset;
    }
}

void CPU::jump(uint8_t condition, uint16_t address) {
    if (condition == 1) {
        pc_ = address;
    }
}

void CPU::push(uint8_t operand0, uint8_t operand1) {
    sp_--;
    writeToMemory(sp_, operand0);
    sp_--;
    writeToMemory(sp_, operand1);
}

void CPU::push(uint16_t operand) {
    push(getHigh(operand), getLow(operand));
}

void CPU::pop(uint8_t* operand0, uint8_t* operand1) {
    *operand0 = readFromMemory(sp_++);
    *operand1 = readFromMemory(sp_++);
}

void CPU::pop(uint16_t *operand) {
    uint8_t low;
    uint8_t high;
    pop(&low, &high);
    *operand = to16(low, high);
}

void CPU::call(uint8_t operand0, uint8_t operand1) {
    push(pc_);
    jump(to16(operand0, operand1));
}

void CPU::call(uint16_t address) {
    push(pc_);
    jump(address);
}

void CPU::call(uint8_t condition, uint16_t address) {
    if (condition == 1) {
        call(address);
    }
}

void CPU::ret() {
    uint16_t addr;
    pop(&addr);
    jump(addr);
}

void CPU::ret(uint8_t condition) {
    if (condition == 1) {
        ret();
    }
}

void CPU::reti() {
    ret();
    interrupt_master_enable_ = true;
}

void CPU::rst(uint8_t offset) {
    push(pc_);
    jump(0x0000 + offset);
}

void CPU::increment(uint8_t* reg) {
    uint8_t h = checkHalfCarry(*reg, 1);
    (*reg)++;
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }
    // Carry is not affected
}

void CPU::increment(uint16_t* reg) {
    (*reg)++;
    // No flags are affected
}

void CPU::decrement(uint8_t* reg) {
    uint8_t h = checkHalfCarrySub(*reg, 1);
    (*reg)--;
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    setFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }
    // Carry is not affected
}

void CPU::decrement(uint16_t* reg) {
    (*reg)--;
    // No flags are affected
}

// Add reg to A
void CPU::add(uint8_t reg) {
    uint8_t h = checkHalfCarry(af_.a_, reg);
    uint8_t c = checkCarry(af_.a_, reg);

    af_.a_ = af_.a_ + reg;
    if (af_.a_ == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }

    if (c == 0x01) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

// Subtract reg from A
void CPU::sub(uint8_t reg) {
    uint8_t h = checkHalfCarrySub(af_.a_, reg);
    uint8_t c = checkCarrySub(af_.a_, reg);

    af_.a_ = af_.a_ - reg;
    if (af_.a_ == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    setFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }

    if (c == 0x01) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

// Add value to register HL
void CPU::addHL(uint16_t value) {
    uint8_t h = checkHalfCarry16bit(hl_.hl_, value);
    uint8_t c = checkCarry16bit(hl_.hl_, value);

    hl_.hl_ += value;
    // Flag Z is not affected (strange)
    resetFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }
    if (c == 0x01) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

// Add value to stack pointer
void CPU::addSP(uint8_t value) {
    int8_t value_signed = static_cast<int8_t>(value);
    uint8_t h = checkHalfCarry(sp_, value_signed);
    uint8_t c = checkCarry(sp_, value_signed);

    sp_ += value_signed;

    resetFlagZ();
    resetFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }

    if (c == 0x01) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

void CPU::ldHLSP(int8_t value)
{
    uint8_t h = checkHalfCarry(sp_, value);
    uint8_t c = checkCarry(sp_, value);
    hl_.hl_ = sp_ + value;

    resetFlagZ();
    resetFlagN();
    if (h == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }

    if (c == 0x01) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

uint8_t CPU::checkHalfCarry(uint8_t a, uint8_t b) const {
    return (((a & 0x0F) + (b & 0x0F)) & 0x10) == 0x10;
}

uint8_t CPU::checkHalfCarry16bit(uint16_t a, uint16_t b) const {
    return (((a & 0x0FFF) + (b & 0x0FFF)) & 0x1000) == 0x1000;
}

uint8_t CPU::checkHalfCarrySub(uint8_t a, uint8_t b) const {
    return (((a & 0x0F) - (b & 0x0F)) & 0x10) == 0x10;
}

uint8_t CPU::checkCarry(uint8_t a, uint8_t b) const {
    uint16_t test = static_cast<uint16_t>(a) + static_cast<uint16_t>(b);
    return (test >> 8) & 0x1;
}

uint8_t CPU::checkCarry16bit(uint16_t a, uint16_t b) const {
    uint32_t test = static_cast<uint32_t>(a) + static_cast<uint32_t>(b);
    return (test >> 16) & 0x1;
}

uint8_t CPU::checkCarrySub(uint8_t a, uint8_t b) const {
    uint16_t test = static_cast<uint16_t>(a) - static_cast<uint16_t>(b);
    return (test >> 8) & 0x1;
}

void CPU::complementCarry() {
    // Z is not affected
    resetFlagN();
    resetFlagH();
    if (getFlagC()) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

// Add N plus Carry to A
void CPU::addNPlusCarry(uint8_t value) {
    // There has to be a more elegant way...
    uint8_t h1 = checkHalfCarry(af_.a_, value);
    uint8_t c1 = checkCarry(af_.a_, value);

    uint8_t h2 = checkHalfCarry(af_.a_, value + getFlagC());
    uint8_t c2 = checkCarry(af_.a_, value + getFlagC());

    uint8_t h3 = checkHalfCarry(value, getFlagC());
    uint8_t c3 = checkCarry(value, getFlagC());

    af_.a_ += value + getFlagC();

    if (af_.a_ == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();

    if (h1 == 0x01 || h2 == 0x01 || h3 == 0x01) {
        setFlagH();
    } else {
        resetFlagH();
    }

    if (c1 == 0x01 || c2 == 0x01 || c3 == 0x01) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

// Subtract N plus Carry from A
void CPU::subNPlusCarry(uint8_t value) {
    uint8_t h1 = checkHalfCarrySub(af_.a_, value);
    uint8_t c1 = checkCarrySub(af_.a_, value);

    uint8_t h2 = checkHalfCarrySub(af_.a_, getFlagC());
    uint8_t c2 = checkCarrySub(af_.a_, getFlagC());

    uint8_t h3 = checkHalfCarrySub(af_.a_ - value, getFlagC());
    uint8_t c3 = checkCarrySub(af_.a_ - value, getFlagC());

    af_.a_ -= (value + getFlagC());

    if (af_.a_ == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    setFlagN();

    if (h1 || h2 || h3) {
        setFlagH();
    } else {
        resetFlagH();
    }

    if (c1 || c2 || c3) {
        setFlagC();
    } else {
        resetFlagC();
    }
}

void CPU::xorA(uint8_t reg) {
    af_.a_ = af_.a_ ^ reg;
    if (af_.a_ == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    resetFlagC();
}

void CPU::orA(uint8_t reg) {
    af_.a_ = af_.a_ | reg;
    if (af_.a_ == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    resetFlagC();
}

void CPU::andA(uint8_t value) {
    af_.a_ = af_.a_ & value;
    if (af_.a_ == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    setFlagH();
    resetFlagC();
}

// Swap upper and lower nibbles of register
void CPU::swap(uint8_t* reg) {
    uint8_t lower_nibble = *reg & 0x0F;
    uint8_t upper_nibble = (*reg >> 4) & 0x0F;
    uint8_t value = static_cast<uint8_t>((lower_nibble << 4)) | upper_nibble;
    *reg = value;
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    resetFlagC();
}

void CPU::rlca() {
    uint8_t bit7 = getBitN(af_.a_, 7);
    af_.a_ = static_cast<uint8_t>(((af_.a_) << 1));
    af_.a_ |= bit7;
    resetFlagZ(); // Different from rotateLeft
    resetFlagN();
    resetFlagH();
    if (bit7 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::rotateLeft(uint8_t* reg) {
    uint8_t bit7 = getBitN(*reg, 7);
    *reg = static_cast<uint8_t>(((*reg) << 1));
    *reg |= bit7;
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    if (bit7 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::rrca() {
    uint8_t bit0 = getBitN(af_.a_, 0);
    af_.a_ = static_cast<uint8_t>(((af_.a_) >> 1));
    af_.a_ |= (bit0 << 7);
    resetFlagZ(); // Different from rotateRight
    resetFlagN();
    resetFlagH();
    if (bit0 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::rotateRight(uint8_t* reg) {
    uint8_t bit0 = getBitN(*reg, 0);
    *reg = static_cast<uint8_t>(((*reg) >> 1));
    *reg |= (bit0 << 7);
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    if (bit0 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::rla() {
    uint8_t bit7 = getBitN(af_.a_, 7);
    af_.a_ = static_cast<uint8_t>(((af_.a_) << 1));
    af_.a_ |= getFlagC();
    resetFlagZ(); // Different from rotateLeftThroughCarry
    resetFlagN();
    resetFlagH();
    if (bit7 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::rotateLeftThroughCarry(uint8_t *reg) {
    uint8_t bit7 = getBitN(*reg, 7);
    *reg = static_cast<uint8_t>(((*reg) << 1));
    *reg |= getFlagC();
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    if (bit7 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::rra() {
    uint8_t bit0 = getBitN(af_.a_, 0);
    af_.a_ = static_cast<uint8_t>(((af_.a_) >> 1));
    af_.a_ |= (getFlagC() << 7);
    resetFlagZ(); // Different from rotateRightThroughCarry
    resetFlagN();
    resetFlagH();
    if (bit0 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::rotateRightThroughCarry(uint8_t *reg) {
    uint8_t bit0 = getBitN(*reg, 0);
    *reg = static_cast<uint8_t>(((*reg) >> 1));
    *reg |= (getFlagC() << 7);
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    if (bit0 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

// Shift right into carry (SRL)
void CPU::shiftRightIntoCarry(uint8_t *reg) {
    uint8_t bit0 = getBitN(*reg, 0);

    // Shift right
    *reg = static_cast<uint8_t>(((*reg) >> 1));

    // Set MSB to 0
    *reg &= 0x7F;
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    if (bit0 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

// Shift right into carry (SRA)
void CPU::shiftRightIntoCarryMSB(uint8_t *reg) {
    uint8_t bit0 = getBitN(*reg, 0);
    uint8_t bit7 = getBitN(*reg, 7);

    // Shift right
    *reg = static_cast<uint8_t>(((*reg) >> 1));

    // MSB does not change
    if (bit7) {
        setBit(reg, 7);
    } else {
        resetBit(reg, 7);
    }

    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    if (bit0 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::shiftLeftIntoCarry(uint8_t *reg) {
    uint8_t bit7 = getBitN(*reg, 7);

    // Shift left
    *reg = static_cast<uint8_t>(((*reg) << 1));

    // Set LSB to 0
    *reg &= 0xFE;
    if (*reg == 0) {
        setFlagZ();
    } else {
        resetFlagZ();
    }
    resetFlagN();
    resetFlagH();
    if (bit7 == 0) {
        resetFlagC();
    } else {
        setFlagC();
    }
}

void CPU::setCarryFlag() {
    resetFlagN();
    resetFlagH();
    setFlagC();
}

void CPU::daa() {
    int tmp = af_.a_;

    if ( ! getFlagN() ) {
        if ( getFlagH() || ( tmp & 0x0F ) > 9 )
            tmp += 6;
        if ( getFlagC() || tmp > 0x9F )
            tmp += 0x60;
    } else {
        if ( getFlagH() ) {
            tmp -= 6;
            if ( ! getFlagC() )
                tmp &= 0xFF;
        }
        if ( getFlagC() )
            tmp -= 0x60;
    }
    resetFlagH();
    resetFlagZ();
    if ( tmp & 0x100 )
        setFlagC();
    af_.a_ = tmp & 0xFF;
    if ( ! af_.a_ )
        setFlagZ();
}

void CPU::setFlagZ() {
    af_.f_ |= 1 << 7;
}

void CPU::resetFlagZ() {
    af_.f_ &= ~(1 << 7);
}

uint8_t CPU::getFlagZ() const {
    return (af_.f_ >> 7) & 1;
}

void CPU::setFlagN() {
    af_.f_ |= 1 << 6;
}

void CPU::resetFlagN() {
    af_.f_ &= ~(1 << 6);
}

uint8_t CPU::getFlagN() const {
    return (af_.f_ >> 6) & 1;
}

void CPU::setFlagH() {
    af_.f_ |= 1 << 5;
}

void CPU::resetFlagH() {
    af_.f_ &= ~(1 << 5);
}

uint8_t CPU::getFlagH() const {
    return (af_.f_ >> 5) & 1;
}

void CPU::setFlagC() {
    af_.f_ |= 1 << 4;
}

void CPU::resetFlagC() {
    af_.f_ &= ~(1 << 4);
}

uint8_t CPU::getFlagC() const {
    return (af_.f_ >> 4) & 1;
}
