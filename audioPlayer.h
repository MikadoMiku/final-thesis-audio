#pragma once
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <atomic>
#include <string>

int playSongFromFile();

int playClipFromFile(std::string clipName);

extern std::atomic<bool> stopMusicFlag;
