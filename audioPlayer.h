#pragma once
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <atomic>
#include <string>
#include "readWavHeader.h"

int playClipFromFile(std::string clipName);

HRESULT PlayAudioStream(WAV_HEADER* wavHeader);

extern std::atomic<bool> stopMusicFlag;

extern LPCWSTR audioEndpointId;