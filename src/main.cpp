#include <string>
#include <fstream>

#include "cpu.h"
#include "instructiondecoder.h"
#include "romloader.h"

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
    CPU cpu(&rl);
    cpu.setBreakPoint(0x0750);
    cpu.run();

    return EXIT_SUCCESS;
}
