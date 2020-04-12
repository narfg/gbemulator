#ifndef ARDUINO

#include <fstream>
#include <memory>
#include <string>

#include "cpu.h"
#include "instructiondecoder.h"
#include "joypad.h"
#include "romloader.h"
#include "sdldisplay.h"

bool file_exists(const std::string& filename)
{
    std::ifstream infile(filename);
    return infile.good();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./emulator rom.gb\n");
        return EXIT_FAILURE;
    }

    const std::string filename{argv[1]};

    if (!file_exists(filename)) {
        fprintf(stderr, "Could not open \"%s\".\n", filename.c_str());
        return EXIT_FAILURE;
    }

    RomLoader rl(filename);
    uint8_t ram[65536];
    uint8_t rom[2*65536];

    // Load ROM into RAM
    bool success = true;
    int cnt = 0;
    while (success) {
        uint8_t rom_byte;
        success = rl.getNextByte(&rom_byte);
        rom[cnt++] = rom_byte;
    }
    printf("ROM size: %d\n", cnt);
    if (cnt == 1) {
        fprintf(stderr, "Error reading ROM.\n");
        exit(1);
    }

    Joypad joypad;
    std::shared_ptr<Display> display = std::make_shared<SDLDisplay>(&ram[0], &joypad);

    CPU cpu(&ram[0], display.get(), &joypad);
    cpu.setROM(&rom[0]);
    cpu.setBreakPoint(0x0750);
    cpu.run();

    return EXIT_SUCCESS;
}

#endif
