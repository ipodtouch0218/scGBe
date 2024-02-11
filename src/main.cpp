#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <wx/wxprec.h>
#include <wx/wx.h>

#include "gbsystem.h"
#include "emulatorframe.h"
#include "emulatorthread.h"
#include "utils.h"

EmulatorFrame* emulator_frame = nullptr;
EmulatorThread* emulator_thread = nullptr;

void close_emulator() {

    emulator_frame->SetTitle("scGBe - No ROM opened");

    if (!emulator_thread) {
        return;
    }

    EmulatorThread* temp = emulator_thread;
    emulator_thread = nullptr;
    temp->Delete();
}

bool open_emulator(std::string filename) {

    close_emulator();

    std::ifstream rom_file(filename, std::ios::in | std::ios::binary);
    if (!rom_file.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;
        return false;
    }

    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(rom_file)), std::istreambuf_iterator<char>());
    rom_file.close();

    emulator_thread = new EmulatorThread(rom);
    wxThreadError error;
    if ((error = emulator_thread->Run()) != wxTHREAD_NO_ERROR || !emulator_thread->rom_valid()) {
        delete emulator_thread;
        emulator_thread = nullptr;
        return false;
    }

    emulator_frame->SetTitle("scGBe - " + emulator_thread->gb().cartridge().header().str_title());
    return true;
}

class scGBe : public wxApp {
    virtual bool OnInit() {
        // Open the window
        emulator_frame = new EmulatorFrame("scGBe - No ROM Opened", wxSize(160 * 3, 144 * 3));
        emulator_frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(scGBe);