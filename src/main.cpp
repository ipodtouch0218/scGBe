#include "gbsystem.h"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Please specify a rom file!" << std::endl;
        return 1;
    }

    GBSystem gb = GBSystem(4194304);

    std::ifstream rom_file(argv[1]);
    if (!rom_file.is_open()) {
        std::cerr << "Failed to open " << argv[1] << std::endl;
        return 1;
    }

    rom_file.read((char*) gb.address_space, 0x7FFF);
    rom_file.close();

    gb.reset();

    while (true) {
        gb.tick();
    }

    return 0;
}