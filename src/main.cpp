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

/*
constexpr int WINDOW_SCALE = 2;
sf::RenderWindow* window;

void window_thread() {
    window = new sf::RenderWindow(sf::VideoMode(160 * WINDOW_SCALE, 144 * WINDOW_SCALE), "scGBe - " + gb->cartridge().header().str_title());

    sf::Texture texture;
    texture.create(SCREEN_W, SCREEN_H);
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setScale(WINDOW_SCALE, WINDOW_SCALE);

    while (window->isOpen()) {
        sf::Event event;
        while (window->pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed: {
                window->close();
                exit(0);
                break;
            }
            case sf::Event::KeyReleased: {
                if (event.key.code == 57) {
                    log_on = false;
                    break;
                }
                bool valid_key = true;
                uint8_t index;
                switch (event.key.code) {
                case 58: index = 7; break;
                case 59: index = 6; break;
                case 23: index = 5; break;
                case 25: index = 4; break;
                case 74: index = 3; break;
                case 73: index = 2; break;
                case 71: index = 1; break;
                case 72: index = 0; break;
                default: valid_key = false; break;
                }
                if (valid_key) {
                    inputs = utils::set_bit_value(inputs, index, 1);
                }
                break;
            }
            case sf::Event::KeyPressed: {
                if (event.key.code == 57) {
                    log_on = true;
                    break;
                }
                bool valid_key = true;
                uint8_t index;
                switch (event.key.code) {
                case 58: index = 7; break;
                case 59: index = 6; break;
                case 23: index = 5; break;
                case 25: index = 4; break;
                case 74: index = 3; break;
                case 73: index = 2; break;
                case 71: index = 1; break;
                case 72: index = 0; break;
                default: valid_key = false; break;
                }
                if (valid_key) {
                    inputs = utils::set_bit_value(inputs, index, 0);
                }
                break;
            }
            }
        }

        window->clear();
        texture.update((sf::Uint8*) gb->ppu().framebuffer);
        window->draw(sprite);
        window->display();
    }
}


int main(int argc, char* argv[]) {

    gb = new GBSystem(false);
    if (argc >= 2) {
        if (!open_rom(argv[1])) {
            std::cerr << "Failed to open the rom '" << argv[1] << "'" << std::endl;
            exit(2);
        }
    }

    sf::Thread thread(&window_thread);
    thread.launch();

    GBSoundStream* sound_stream = new GBSoundStream();

    using frames = std::chrono::duration<int64_t, std::ratio<400, 23891>>;
    auto game_start = std::chrono::system_clock::now();

    while(true) {
        // Get a new sound sample.
        int expected_samples = (std::min(gb->frame_number, (uint64_t) 2) * 1064) + ((gb->frame_cycles / 132) * 2);
        if (gb->frame_cycles % 132 == 0) {
            sound_stream->add_sample(gb->apu().current_sample(false), gb->apu().current_sample(true));
        }

        if (gb->tick()) {
            // Frame is done! Wait enough time...
            if (gb->frame_number == 2) {
                sound_stream->play();
            }

            std::this_thread::sleep_until(game_start + frames{gb->frame_number});
        }
    }
    return 0;
}
*/

EmulatorThread* emulator_thread = nullptr;

void close_emulator() {
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

    return true;
}

class scGBe : public wxApp {
    virtual bool OnInit() {
        // Open the window
        EmulatorFrame* frame = new EmulatorFrame("scGBe - No ROM Opened", wxSize(160 * 3, 144 * 3));
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(scGBe);