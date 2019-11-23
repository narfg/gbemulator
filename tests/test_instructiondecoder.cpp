#define CATCH_CONFIG_MAIN
#include <string>

#include "catch2/catch.hpp"

#include "instructiondecoder.h"


TEST_CASE( "Decode LD SP, d16", "[instructiondecoder]" ) {
    const uint8_t bytes[] = {0x31, 0xFE, 0xFF};

    InstructionDecoder id;
    id.decodeByte(bytes[0]);
    id.decodeByte(bytes[1]);
    id.decodeByte(bytes[2]);
}

TEST_CASE( "Prefix instruction", "[instructiondecoder]" ) {
    const uint8_t bytes[] = {0xCB, 0x7C};

    InstructionDecoder id;
    for (uint8_t cnt = 0; cnt < 2; ++cnt) {
        id.decodeByte(bytes[cnt]);
    }
}

TEST_CASE( "Decode multiple instructions", "[instructiondecoder]" ) {
    const uint8_t bytes[] = {0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32};

    InstructionDecoder id;
    for (uint8_t cnt = 0; cnt < 8; ++cnt) {
        id.decodeByte(bytes[cnt]);
    }
}

TEST_CASE( "Decode multiple instructions2", "[instructiondecoder]" ) {
    const uint8_t bytes[] = {0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32};
    uint16_t pc = 0;

    Instruction ref1{0x31, false, 0xFE, 0xFF, 12, 12};
    Instruction ref2{0xAF, false, 0x00, 0x00, 4, 4};
    Instruction ref3{0x21, false, 0xFF, 0x9F, 12, 12};
    Instruction ref4{0x32, false, 0x00, 0x00, 8, 8};

    InstructionDecoder id;
    Instruction a = id.decode(bytes+pc, &pc);
    REQUIRE(a == ref1);
    REQUIRE(pc == 3);

    Instruction b = id.decode(bytes+pc, &pc);
    REQUIRE(b == ref2);
    REQUIRE(pc == 4);

    Instruction c = id.decode(bytes+pc, &pc);
    REQUIRE(c == ref3);
    REQUIRE(pc == 7);

    Instruction d = id.decode(bytes+pc, &pc);
    REQUIRE(d == ref4);
    REQUIRE(pc == 8);
}

TEST_CASE( "Decode prefix instruction", "[instructiondecoder]" ) {
    const uint8_t bytes[] = {0xCB, 0x01};
    uint16_t pc = 0;

    Instruction ref1{0x01, true, 0x00, 0x00, 12, 12};

    InstructionDecoder id;
    Instruction a = id.decode(bytes, &pc);
    REQUIRE(a == ref1);
    REQUIRE(pc == 2);
}
