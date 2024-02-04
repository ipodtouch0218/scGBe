#include "gbsystem.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <iterator>
#include <fstream>
#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "utils.h"

constexpr int WINDOW_SCALE = 2;

GBSystem* gb = nullptr;
bool request_dump = false;
bool log_on = false;
extern uint8_t inputs;

void window_thread() {
    sf::RenderWindow window(sf::VideoMode(160 * WINDOW_SCALE, 144 * WINDOW_SCALE), "Test Window");

    sf::Texture texture;
    texture.create(SCREEN_W, SCREEN_H);
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setScale(WINDOW_SCALE, WINDOW_SCALE);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed: {
                window.close();
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

        window.clear();
        texture.update((sf::Uint8*) gb->ppu().framebuffer);
        window.draw(sprite);
        window.display();
    }
}

class GBSoundStream : public sf::SoundStream {
    public:
    sf::Mutex mutex;
    std::vector<sf::Int16> filled_audio_buffer;
    std::vector<sf::Int16> playing_audio_buffer;

    GBSoundStream() {
        initialize(2, 31775);
        filled_audio_buffer.reserve(532 * 2 * 2);
        playing_audio_buffer.reserve(532 * 2);
    }

    bool onGetData(SoundStream::Chunk& data) {
        mutex.lock();
        int size = filled_audio_buffer.size();
        if (size >= 532 * 2) {
            std::copy_n(filled_audio_buffer.begin(), 532 * 2, playing_audio_buffer.begin());
            filled_audio_buffer.erase(filled_audio_buffer.begin(), filled_audio_buffer.begin() + 532 * 2);
        } else {
            // std::cerr << "[AUDIO] didnt have enough samples! (" << size << ")" << std::endl;
        }

        mutex.unlock();

        data.samples = playing_audio_buffer.data();
        data.sampleCount = 532 * 2;
        return true;
    }

    void onSeek(sf::Time timeOffset) { }

    void add_sample(sf::Int16 sample) {
        mutex.lock();
        filled_audio_buffer.push_back(sample);
        mutex.unlock();
    }
};

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Please specify a rom file!" << std::endl;
        return 1;
    }

    std::ifstream rom_file(argv[1], std::ios::in | std::ios::binary);
    if (!rom_file.is_open()) {
        std::cerr << "Failed to open " << argv[1] << std::endl;
        return 1;
    }

    gb = new GBSystem(false);

    std::vector<uint8_t> rom((std::istreambuf_iterator<char>(rom_file)), std::istreambuf_iterator<char>());
    rom_file.close();

    gb->cartridge().load_rom(rom);
    gb->reset();

    sf::Thread thread(&window_thread);
    thread.launch();

    GBSoundStream* sound_stream = new GBSoundStream();

    using frames = std::chrono::duration<int64_t, std::ratio<400, 23891>>;
    auto game_start = std::chrono::system_clock::now();

    while(true) {
        // Get a new sound sample.
        if (gb->cycles % 132 == 0) {
            sound_stream->add_sample(gb->apu().current_sample(false));
            sound_stream->add_sample(gb->apu().current_sample(true));
        }

        if (gb->tick()) {
            // Frame is done! Wait enough time...
            if (gb->frame_number == 3 && sound_stream->getStatus() != sf::SoundSource::Status::Playing) {
                sound_stream->play();
            }

            std::this_thread::sleep_until(game_start + frames{gb->frame_number});
        }
    }
    return 0;
}