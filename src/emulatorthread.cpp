#include "emulatorthread.h"
#include <iostream>
#include <thread>

constexpr double MAX_AUDIO_ERROR_CORRECT_PERCENTAGE = 0.005;

extern void on_frame_complete();

EmulatorThread::EmulatorThread(std::vector<uint8_t>& rom)
    : wxThread(wxTHREAD_DETACHED)
{
    _gb = std::unique_ptr<GBSystem>(new GBSystem(false));

    _gb->reset();
    Cartridge& cartridge = _gb->cartridge();
    cartridge.load_rom(rom);

    uint8_t header_checksum = cartridge.header().calculate_header_checksum();
    _rom_valid = header_checksum == cartridge.header().header_checksum;
}

wxThread::ExitCode EmulatorThread::Entry() {
    // Actual thread entry point

    // 1/(59.7275)... this syntax is... interesting.
    using frames = std::chrono::duration<int64_t, std::ratio<400, 23891>>;
    _emulation_start_time = std::chrono::system_clock::now();

    while(true) {
        if (TestDestroy()) {
            return (wxThread::ExitCode) 0;
        }

        // Audio sampling rates
        double sample_rate = 132.0 * (_fps / 59.7275);

        // Tick the emulator
        bool frame_complete = _gb->tick();

        // Get a new sound sample. (if ready)
        if (++_audio_sample_timer >= sample_rate) {
            _sound_stream.add_sample(_gb->apu().current_sample(false), _gb->apu().current_sample(true));
            _audio_sample_timer -= sample_rate;
        }

        if (frame_complete) {
            on_frame_complete();
            // Frame is done! Wait enough time...
            if (_sound_stream.getStatus() != sf::SoundSource::Status::Playing && _sound_stream.filled_audio_buffer.size() >= (1064 * 2)) {
                _sound_stream.play();
            }

            std::this_thread::sleep_until(_emulation_start_time + (frames{_gb->frame_number} * (59.7275 / _fps)));

            while (!TestDestroy() && _gb->frame_number >= _pause_after_frame) {
                std::this_thread::sleep_for(frames{1});
            }
        }
    }

    return (wxThread::ExitCode) 0;
}

void EmulatorThread::set_fps(double fps) {
    _fps = fps;
    _gb->frame_number = 0;
    _emulation_start_time = std::chrono::system_clock::now();
}

void EmulatorThread::pause() {
    _pause_after_frame = 0;
    _sound_stream.stop();
}

void EmulatorThread::unpause() {
    _pause_after_frame = -1;
    _emulation_start_time = std::chrono::system_clock::now();
    _gb->frame_number = 0;
}

void EmulatorThread::next_frame() {
    _pause_after_frame = -1;
}