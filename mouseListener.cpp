/*
    Example of how to get raw mouse input data using the native Windows API
    through a "message-only" window that outputs to an allocated console window.

    Not prepared for Unicode.
    I don't recommend copy/pasting this, understand it before integrating it.
*/

// Make Windows.h slightly less of a headache.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define UNICODE
#include <Windows.h>
#include <napi.h>
#include <cmath>
#include <atomic>

#include <tchar.h>
// Only for redirecting stdout to the console.
#include <cstdio>
#include <iostream>
#include "mouseListener.h"
#include "audioPlayer.h"

// Structure to store out input data.
// Not necessary, I just find it neat.
struct
{
    struct
    {
        // Keep in mind these are all deltas,
        // they'll change for one frame/cycle
        // before going back to zero.
        int x = 0;
        int y = 0;
        int wheel = 0;
        bool middleDown = false;
    } mouse;
} input;

Napi::ThreadSafeFunction tsfn;

int CalculateDegreeFromVectors(int x, int y);

// Data structure representing our thread-safe function context.
struct TsfnContext
{
    TsfnContext(Napi::Env env){};
    std::thread nativeThread;
};

void ReleaseTSFN();

std::atomic<bool> stopMouseListener = false;

// The thread-safe function finalizer callback. This callback executes
// at destruction of thread-safe function, taking as arguments the finalizer
// data and threadsafe-function context.
void FinalizerCallback(Napi::Env env, void *finalizeData, TsfnContext *context);

// Window message callback.
LRESULT CALLBACK EventHandler(HWND, unsigned, WPARAM, LPARAM);

void startMouseListener(const Napi::CallbackInfo &info)
{
    std::cout << "Starting...." << std::endl;
    Napi::Env env = info.Env();

    // Stop if already running
    ReleaseTSFN();

    // Construct context data
    auto contextData = new TsfnContext(env);

    // Create a ThreadSafeFunction
    tsfn = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(), // JavaScript function called asynchronously
        "Mouse Events",               // Name
        0,                            // Unlimited queue
        1,                            // Only one thread will use this initially
        contextData,                  // Context that can be accessed by Finalizer
        FinalizerCallback,            // Finalizer used to clean threads up
        (void *)nullptr               // Finalizer data
    );

    contextData->nativeThread = std::thread([]
                                            {
    // Why even bother with WinMain?
    HINSTANCE instance = GetModuleHandle(0);

    // Create message-only window:
    const char *class_name = "SimpleEngine Class";

    WNDCLASS window_class = {};
    window_class.lpfnWndProc = EventHandler;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"SimpleEngineClass";

    if (!RegisterClass(&window_class))
    {
        std::cout << "register class went poopoo" << std::endl;
        // return -1;
    }

    HWND window = CreateWindow(L"SimpleEngineClass", L"SimpleEngine", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);

    if (window == nullptr)
    {
        std::cout << "window is nullptr" << std::endl;
        // return -1;
    }
// End of creating window.

// Registering raw input devices
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC ((unsigned short)0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((unsigned short)0x02)
#endif

    // We're configuring just one RAWINPUTDEVICE, the mouse,
    // so it's a single-element array (a pointer).
    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = window;
    RegisterRawInputDevices(rid, 1, sizeof(rid[0]));
    // End of resgistering.

    // Main loop:
    MSG event;
    bool quit = false;

    while (!stopMouseListener || !quit)
    {
        while (GetMessage(&event, 0, 0, 0))
        {
            if (event.message == WM_QUIT || stopMouseListener)
            {
                quit = true;
                break;
            }

            // Does some Windows magic and sends the message to EventHandler()
            // because it's associated with the window we created.
            TranslateMessage(&event);
            DispatchMessage(&event);
        }
    } });
}

// Called from JS to release the TSFN and stop listening to keyboard events
void Stop(const Napi::CallbackInfo &info)
{
    ReleaseTSFN();
}

// Release the TSFN
void ReleaseTSFN()
{
    if (tsfn)
    {
        napi_status status = tsfn.Release();
        if (status != napi_ok)
        {
            std::cerr << "Failed to release the TSFN!" << std::endl;
        }
        tsfn = NULL;
    }
}

LRESULT CALLBACK EventHandler(
    HWND hwnd,
    unsigned event,
    WPARAM wparam,
    LPARAM lparam)
{
    switch (event)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        // return 0;
    case WM_INPUT:
    {
        // The official Microsoft examples are pretty terrible about this.
        // Size needs to be non-constant because GetRawInputData() can return the
        // size necessary for the RAWINPUT data, which is a weird feature.
        unsigned size = sizeof(RAWINPUT);
        static RAWINPUT raw[sizeof(RAWINPUT)];
        GetRawInputData((HRAWINPUT)lparam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));

        if (raw->header.dwType == RIM_TYPEMOUSE)
        {
            if (raw->data.mouse.usButtonFlags == RI_MOUSE_BUTTON_3_DOWN)
            {
                input.mouse.x = 0;
                input.mouse.y = 0;
                input.mouse.middleDown = true;
            }
            else if (raw->data.mouse.usButtonFlags == RI_MOUSE_BUTTON_3_UP)
            {
                input.mouse.y *= -1;
                input.mouse.middleDown = false;
                int sector = CalculateDegreeFromVectors(input.mouse.x, input.mouse.y);
                std::cout << "Sector: " << sector << std::endl;
                napi_status status =
                    tsfn.BlockingCall([=](Napi::Env env, Napi::Function jsCallback)
                                      { jsCallback.Call(
                                            {Napi::Number::New(env, input.mouse.x),
                                             Napi::Number::New(env, input.mouse.y),
                                             Napi::Number::New(env, sector)}); });

                if (status != napi_ok)
                {
                    std::cerr << "Failed to execute BlockingCall!" << std::endl;
                }
            }
            if (input.mouse.middleDown == true)
            {
                input.mouse.x += raw->data.mouse.lLastX;
                input.mouse.y += raw->data.mouse.lLastY;
            }

            // Wheel data needs to be pointer casted to interpret an unsigned short as a short, with no conversion
            // otherwise it'll overflow when going negative.
            // Didn't happen before some minor changes in the code, doesn't seem to go away
            // so it's going to have to be like this.
            if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                input.mouse.wheel = (*(short *)&raw->data.mouse.usButtonData) / WHEEL_DELTA;
        }
    }
        // return 0;
    }

    // Run default message processor for any missed events:
    return DefWindowProc(hwnd, event, wparam, lparam);
}

void FinalizerCallback(Napi::Env env, void *finalizeData, TsfnContext *context)
{
    DWORD threadId = GetThreadId(context->nativeThread.native_handle());
    if (threadId == 0)
    {
        std::cerr << "GetThreadId failed: " << std::endl;
    }
    stopMouseListener = true;
    if (context->nativeThread.joinable())
    {
        context->nativeThread.join();
    }
    else
    {
        std::cerr << "Failed to join nativeThread!" << std::endl;
    }

    delete context;
}

int CalculateDegreeFromVectors(signed int x, signed int y)
{
    double vectorOneLength = y;
    double vectorTwoLength = sqrt(x * x + y * y);
    // VectorOne / VectorTwo
    double vectorLengthDivision = ((double)vectorOneLength) / ((double)vectorTwoLength);
    // Get degree
    double degree = acos(vectorLengthDivision);
    if ((x < 0 && y < 0) || (x < 0 && y > 0))
    {
        double sliverDegree = 360.0 - (180.0 + (degree * 180 / 3.1415));
        if ((180.0 + sliverDegree + 22.5) > 360)
        {
            std::cout << "(360) Sector is: " << ((180.0 + sliverDegree + 22.5) - 360.0) / 45.0 << std::endl;
            return ceil(((180.0 + sliverDegree + 22.5) - 360.0) / 45.0);
        }
        else
        {
            std::cout << "The Sector is: " << (180.0 + sliverDegree + 22.5) / 45.0 << std::endl;
            return ceil((180.0 + sliverDegree) / 45.0);
        }
        // return 180.0 + sliverDegree;
    }

    std::cout << "(Non Sliver)The Sector is: " << ((degree * 180 / 3.1415) + 22.5) / 45.0 << std::endl;
    return ceil(((degree * 180 / 3.1415) + 22.5) / 45.0);
    // return degree * 180 / 3.1415;
}
