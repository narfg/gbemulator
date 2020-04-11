#pragma once
#include <cstdint>

#include "instructiondecoder.h"
#include "joypad.h"
#include "ppu.h"
#include "timer.h"
#include "romloader.h"

class CPU
{
public:
    CPU(RomLoader* rl);
    void run();
    void checkForInterrupts();
    bool executeInstruction(uint8_t instruction, bool prefix, uint8_t operand0, uint8_t operand1, bool* taken = nullptr);
    void setBreakPoint(uint16_t breakpoint) { breakpoint_ = breakpoint; }
    uint16_t getSP() const { return sp_; }
    uint16_t getPC() const { return pc_; }
    uint8_t getA() const { return af_.a_; }
    uint16_t getHL() const { return hl_.hl_; }
    uint16_t getBC() const { return bc_.bc_; }
    uint16_t getDE() const { return de_.de_; }
    uint8_t getB() const { return bc_.b_; }
    uint8_t getC() const { return bc_.c_; }
    uint8_t getD() const { return de_.d_; }
    uint8_t getE() const { return de_.e_; }
    uint8_t getF() const { return af_.f_; }
    uint8_t getFlagZ() const;
    uint8_t getFlagN() const;
    uint8_t getFlagH() const;
    uint8_t getFlagC() const;
    void printStack() const;

private:
    InstructionDecoder id_;
    PPU ppu_;
    Timer timer_;
    Joypad joypad_;

    void printInfo();
    void printInstrWSP(const char* string, uint8_t operand0 = 0, uint8_t operand1 = 0) const;
    uint8_t readFromMemory(uint16_t address) const;
    void writeToMemory(uint16_t address, uint8_t value);
    void loadSP(uint8_t operand0, uint8_t operand1);
    void writeIO(uint8_t offset, uint8_t value);
    void loadAddr(uint16_t addr, uint8_t value);
    void loadDecAddr(uint16_t* addr, uint8_t value);
    void loadIncAddr(uint16_t* addr, uint8_t value);
    void loadAHLPlus();
    void testBit(uint8_t reg, uint8_t number);
    void setBit(uint8_t *reg, uint8_t number);
    void resetBit(uint8_t *reg, uint8_t number);
    void flipAllBits(uint8_t *reg);
    void compare(uint8_t value);
    void halt();
    void jump(uint8_t condition, int8_t offset);
    void jump(uint8_t condition, uint16_t address);
    void jump(uint16_t address);
    void push(uint8_t operand0, uint8_t operand1);
    void push(uint16_t operand);
    void pop(uint8_t* operand0, uint8_t* operand1);
    void pop(uint16_t *operand);
    void call(uint8_t operand0, uint8_t operand1);
    void call(uint16_t address);
    void call(uint8_t condition, uint16_t address);
    void ret();
    void ret(uint8_t condition);
    void reti();
    void rst(uint8_t offset);
    void increment(uint8_t *reg);
    void increment(uint16_t *reg);
    void decrement(uint8_t *reg);
    void decrement(uint16_t *reg);

    // Add reg to A
    void add(uint8_t reg);
    // Subtract reg from A
    void sub(uint8_t reg);
    void addHL(uint16_t value);
    void addSP(uint8_t value);
    void ldHLSP(int8_t value);
    void addNPlusCarry(uint8_t value);
    void subNPlusCarry(uint8_t value);
    void xorA(uint8_t reg);
    void orA(uint8_t reg);
    void andA(uint8_t value);
    void swap(uint8_t *reg);
    void rlca();
    void rotateLeft(uint8_t *reg);
    void rrca();
    void rotateRight(uint8_t *reg);
    void rla();
    void rotateLeftThroughCarry(uint8_t *reg);
    void rra();
    void rotateRightThroughCarry(uint8_t *reg);
    void shiftRightIntoCarry(uint8_t *reg);
    void shiftRightIntoCarryMSB(uint8_t *reg);
    void shiftLeftIntoCarry(uint8_t *reg);
    void daa();

    // Flags
    uint8_t checkHalfCarry(uint8_t a, uint8_t b) const;
    uint8_t checkHalfCarry16bit(uint16_t a, uint16_t b) const;
    uint8_t checkHalfCarrySub(uint8_t a, uint8_t b) const;
    uint8_t checkCarry(uint8_t a, uint8_t b) const;
    uint8_t checkCarry16bit(uint16_t a, uint16_t b) const;
    uint8_t checkCarrySub(uint8_t a, uint8_t b) const;
    void complementCarry();
    void setCarryFlag();
    void setFlagZ();
    void resetFlagZ();
    void setFlagN();
    void resetFlagN();
    void setFlagH();
    void resetFlagH();

public:
    void setFlagC();
    void resetFlagC();

private:
    // Registers
    union {
        uint16_t hl_;
        struct {
            uint8_t l_;
            uint8_t h_;
        };
    } hl_;

    union {
        uint16_t bc_;
        struct {
            uint8_t c_;
            uint8_t b_;
        };
    } bc_;

    union {
        uint16_t de_;
        struct {
            uint8_t e_;
            uint8_t d_;
        };
    } de_;

    union {
        uint16_t af_;
        struct {
            uint8_t f_;
            uint8_t a_;
        };
    } af_;

    uint16_t sp_;
    uint16_t pc_;

    // Memory
    uint8_t ram_[65536];
    uint8_t rom_[2*65536];
    uint8_t* io_;
    uint8_t cartridge_type_;
    uint8_t mbc_rombank_;
    uint8_t mbc_mode_;

    bool interrupt_master_enable_;
    bool halted_;
    bool halt_bug_;
    bool stopped_;

    uint16_t breakpoint_;
    uint64_t cycles_;
};

