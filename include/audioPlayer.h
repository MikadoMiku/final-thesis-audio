#pragma once
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <atomic>
#include <string>
#include "readWavHeader.h"

int playClipFromFile(std::string clipName);

HRESULT PlayAudioStream(WAV_HEADER* wavHeader);

int playDirectSynthesizedAudio(std::string textToSynthesize);

extern std::atomic<bool> stopMusicFlag;

extern LPCWSTR audioEndpointId;
