#include "romloader.h"
#include <fstream>
#include <cstring>

RomLoader::RomLoader(const std::string& filename)
{
    ifs_.open(filename, std::ifstream::binary);
    // ifs_boot_rom_.open("/path/to/boot_rom.bin", std::ifstream::binary);
}

bool RomLoader::getNextByte(uint8_t* byte) {
    if (!ifs_.good()) {
        return false;
    }

    // Use this code to put the boot rom in front of the cartridge
    /* if (ifs_boot_rom_.good()) {
        *byte = static_cast<uint8_t>(ifs_boot_rom_.get());
        ifs_.get();
    } else {
        *byte = static_cast<uint8_t>(ifs_.get());
    } */

    // Otherwise use this
    *byte = static_cast<uint8_t>(ifs_.get());
    return true;
}


void RomLoader::writeRom(const std::string& filename, uint8_t* bytes, uint16_t size) const
{
    // Use the original 48byte nintendo logo here if you want to run your ROM on an actual Game Boy or on an
    // emulator that cares about the correct logo.
    uint8_t logo[48] = {0};
    uint8_t rom[32768];
    memset(rom, 0, 32768);

    // Space for GB boot ROM
    uint16_t k = 0x100;

    // Add NOP and jump to 0x150
    rom[k++] = 0x00;
    rom[k++] = 0xC3;
    rom[k++] = 0x50;
    rom[k++] = 0x01;

    // Copy Nintendo logo into ROM
    memcpy(rom+0x104, logo, sizeof(logo));

    // Write program
    k = 0x0150;
    for (uint16_t n = 0; n < size; ++n) {
        rom[k++] = bytes[n];
    }

    // Write header checksum
    uint8_t header_checksum = 0;
    for (uint16_t n = 0x0134; n < 0x014D; ++n) {
        header_checksum = header_checksum - rom[n] - 1;
    }
    rom[0x014D] = header_checksum;

    // Write file
    std::fstream myfile(filename, std::ios::out | std::ios::binary);
    myfile.write(reinterpret_cast<char*>(rom), 32768);
    myfile.close();
}

void RomLoader::writeHeader(const std::string &filename)
{
    std::fstream myfile(filename, std::ios::out);
    myfile << "uint8_t header_rom[] = {";

    bool success = true;
    int cnt = 0;
    while (success) {
        uint8_t rom_byte;
        success = getNextByte(&rom_byte);
        if (success) {
            char buf[6];
            snprintf(buf, 6, "0x%02X,", rom_byte);
            myfile << buf;
            cnt++;
        }
    }
    myfile << "};" << std::endl;
    myfile << "uint32_t header_rom_size = " << cnt << ";";
    printf("ROM size: %d\n", cnt);

    myfile.close();
}
