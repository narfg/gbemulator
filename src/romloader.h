#pragma once
#include <cstdint>
#include <string>
#include <fstream>

class RomLoader
{
public:
    RomLoader(const std::string& filename);
    bool getNextByte(uint8_t* byte);
    void writeRom(const std::string& filename, uint8_t* bytes, uint16_t size) const;

    // Writes the rom as a C header that can be used on microcontrollers
    void writeHeader(const std::string &filename);

private:
    std::ifstream ifs_;
    std::ifstream ifs_boot_rom_;
};
