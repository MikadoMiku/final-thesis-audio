#pragma once
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <atomic>

int playSongFromFile();

extern std::atomic<bool> stopMusicFlag;
