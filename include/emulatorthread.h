#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>
#include <SFML/Audio.hpp>
#include <wx/thread.h>
#include "gbsystem.h"
#include "soundstreamer.h"

class EmulatorThread : public wxThread {

    std::unique_ptr<GBSystem> _gb;
    uint64_t _pause_after_frame = -1;
    bool _rom_valid = false;
    std::chrono::_V2::system_clock::time_point _emulation_start_time;
    double _fps = 59.7275f;

    SoundStreamer _sound_stream;

    public:
    EmulatorThread(std::vector<uint8_t>& rom);

    ExitCode Entry();

    void pause();
    void unpause();
    void next_frame();
    void set_fps(double fps);

    bool rom_valid() const {
        return _rom_valid;
    }

    GBSystem& gb() {
        return *_gb;
    }

    bool paused() const {
        return _gb->frame_number > _pause_after_frame;
    }
};