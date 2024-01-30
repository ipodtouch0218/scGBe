#include "gbsystem.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <fstream>
#include <thread>

#include <SFML/Audio.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "utils.h"

constexpr int WINDOW_SCALE = 2;

GBSystem* gb;
uint8_t joypad_buttons = 0xFF;

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
                }
                joypad_buttons = utils::set_bit_value(joypad_buttons, index, 1);
                break;
            }
            case sf::Event::KeyPressed: {
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
                }
                joypad_buttons = utils::set_bit_value(joypad_buttons, index, 0);
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
    private:
    sf::Mutex mutex;

    public:
    std::vector<sf::Int16> filled_audio_buffer;
    std::vector<sf::Int16> playing_audio_buffer;


    GBSoundStream() {
        initialize(1, 31775);
        filled_audio_buffer.reserve(532);
        playing_audio_buffer.reserve(532);
    }

    bool onGetData(SoundStream::Chunk& data) {
        mutex.lock();
        playing_audio_buffer = filled_audio_buffer;
        filled_audio_buffer.clear();
        mutex.unlock();

        data.samples = playing_audio_buffer.data();
        data.sampleCount = 532;
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

    gb = new GBSystem(false);

    std::ifstream rom_file(argv[1]);
    if (!rom_file.is_open()) {
        std::cerr << "Failed to open " << argv[1] << std::endl;
        return 1;
    }

    rom_file.read((char*) gb->address_space, 0x7FFF);
    rom_file.close();

    gb->reset();

    sf::Thread thread(&window_thread);
    thread.launch();

    GBSoundStream* sound_stream = new GBSoundStream();

    using frames = std::chrono::duration<int64_t, std::ratio<400, 23891>>;
    auto frame_render_start = std::chrono::system_clock::now();

    while(true) {
        if (gb->tick()) {
            // Frame is done! Wait enough time...
            auto sleep_time = (frame_render_start + frames{1}) - std::chrono::system_clock::now();

            std::this_thread::sleep_until(frame_render_start + frames{1});
            frame_render_start = std::chrono::system_clock::now();

            if (sound_stream->getStatus() != sf::SoundSource::Playing) {
                sound_stream->play();
            }
        }

        // Get a new sound sample.
        if (gb->cycles % 132 == 0) {
            sound_stream->add_sample(gb->apu().current_sample());
        }
    }

    return 0;
}