#include "emulatorthread.h"
#include <thread>
#include <iostream>

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

    // 1/(59.7275)... this is dumb.
    using frames = std::chrono::duration<int64_t, std::ratio<400, 23891>>;
    emulation_start_time = std::chrono::system_clock::now();

    while(true) {
        if (TestDestroy()) {
            return (wxThread::ExitCode) 0;
        }

        // Tick the emulator
        bool frame_complete = _gb->tick();

        // Get a new sound sample.
        int expected_samples = (std::min(_gb->frame_number, (uint64_t) 2) * 1064) + ((_gb->frame_cycles / 132) * 2);
        if (_gb->frame_cycles % 132 == 0) {
            _sound_stream.add_sample(_gb->apu().current_sample(false), _gb->apu().current_sample(true));
        }

        if (frame_complete) {
            on_frame_complete();
            // Frame is done! Wait enough time...
            if (_gb->frame_number == 2) {
                _sound_stream.play();
            }

            std::this_thread::sleep_until(emulation_start_time + frames{_gb->frame_number});

            while (!TestDestroy() && _gb->frame_number > _pause_after_frame) {
                std::this_thread::sleep_for(frames{1});
            }
        }
    }

    return (wxThread::ExitCode) 0;
}

void EmulatorThread::pause() {
    _pause_after_frame = 0;
}

void EmulatorThread::unpause() {
    _pause_after_frame = -1;
    emulation_start_time = std::chrono::system_clock::now();
    _gb->frame_number = 0;
}

void EmulatorThread::next_frame() {
    _pause_after_frame = -1;
}