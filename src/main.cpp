#include "cpu.h"
#include <iostream>
#include <fstream>

bool enable_pause = false;

CPU cpu;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Please specify a rom file!" << std::endl;
        return 1;
    }


    std::ifstream rom_file(argv[1]);
    if (!rom_file.is_open()) {
        std::cerr << "Failed to open " << argv[1] << std::endl;
        return 1;
    }

    rom_file.read((char*) cpu.address_space, 0x7FFF);
    rom_file.close();

    cpu.reset();

    while (true) {
        cpu.execute();
        if (enable_pause)
            system("pause");
    }

    return 0;
}