#pragma once

#include <napi.h>
#include <atomic>


void startMouseListener(const Napi::CallbackInfo &info);

extern std::atomic<bool> stopMouseListener;
