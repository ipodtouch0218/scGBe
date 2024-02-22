#include "soundstreamer.h"
#include <iostream>

SoundStreamer::SoundStreamer() {
    initialize(2, 31775);
    filled_audio_buffer.reserve(532 * 2 * 2);
    playing_audio_buffer.reserve(532 * 2);
}

bool SoundStreamer::onGetData(SoundStream::Chunk& data) {
    mutex.lock();
    int size = filled_audio_buffer.size();
    if (size >= 532 * 2) {
        std::copy_n(filled_audio_buffer.begin(), 532 * 2, playing_audio_buffer.begin());
        filled_audio_buffer.erase(filled_audio_buffer.begin(), filled_audio_buffer.begin() + 532 * 2);
    } else {
        std::cerr << "[AUDIO] didnt have enough samples! (" << size << "/1064)" << std::endl;
    }

    mutex.unlock();

    data.samples = playing_audio_buffer.data();
    data.sampleCount = 532 * 2;
    return true;
}

void SoundStreamer::onSeek(sf::Time timeOffset) {
    // Don't support seeking, we dispose of old samples anyway.
}

void SoundStreamer::add_sample(sf::Int16 leftSample, sf::Int16 rightSample) {
    mutex.lock();
    filled_audio_buffer.push_back(leftSample);
    filled_audio_buffer.push_back(rightSample);
    mutex.unlock();
}