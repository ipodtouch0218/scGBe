#pragma once
#include <vector>
#include <SFML/Audio.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System.hpp>

class SoundStreamer : public sf::SoundStream {
    public:
    sf::Mutex mutex;
    std::vector<sf::Int16> filled_audio_buffer;
    std::vector<sf::Int16> playing_audio_buffer;

    SoundStreamer();

    bool onGetData(SoundStream::Chunk& data);

    void onSeek(sf::Time timeOffset);

    void add_sample(sf::Int16 leftSample, sf::Int16 rightSample);
};
