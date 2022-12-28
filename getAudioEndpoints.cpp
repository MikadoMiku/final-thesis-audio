#include <Windows.h>
#include <napi.h>

#include <Mmdeviceapi.h>
#include <iostream>
#include <Functiondiscoverykeys_devpkey.h>
#include <map>
#include <thread>
#include <string>
#include <filesystem>

#include "audioPlayer.h"
#include "mouseListener.h"
#include "ttsFunctionality.h"

#define EXIT_ON_ERROR(hres)                \
    if (FAILED(hres))                      \
    {                                      \
        std::cout << "ERROR" << std::endl; \
        goto Exit;                         \
    }
#define SAFE_RELEASE(punk) \
    if ((punk) != NULL)    \
    {                      \
        (punk)->Release(); \
        (punk) = NULL;     \
    }

std::thread musicThread;
bool musicRunning = false;
extern std::atomic<bool> stopMusicFlag(false);

// Convert a wide Unicode string to an UTF8 string
// https://technoteshelp.com/c-how-do-you-properly-use-widechartomultibyte/
// I noticed in node api github that Value::From should be able to accept UTF-16 encodings????? why not work
std::string utf8_encode(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

namespace audEndpointsComs
{
    /* Each COM class is identified by a CLSID, a unique 128-bit GUID, which the server must register.
    COM uses this CLSID, at the request of a client, to associate specific data with the DLL or EXE containing the code that implements the class,
     thus creating an instance of the object. */
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    // IID - Describes a GUID structure used to describe an identifier for a MAPI interface. --> https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/iid
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
}

std::map<int, char> endpointMap;
LPCWSTR audioEndpointId = NULL;

Napi::Value getAudioEndpoints(const Napi::CallbackInfo &info)
{
    // Do napi objects and such need to be cleaned aswell? to prevent mem leaks...

    // Check if env was success

    Napi::Env env = info.Env();
    Napi::Array endpointArray;
    Napi::Object jsObject = Napi::Object::New(env);

    IMMDeviceCollection *pEndpointsCollection = NULL;
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pEndpoint = NULL;
    LPWSTR pwszID = NULL;
    IPropertyStore *pProps = NULL;

    hr = CoCreateInstance(
        audEndpointsComs::CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, audEndpointsComs::IID_IMMDeviceEnumerator,
        (void **)&pEnumerator);
    EXIT_ON_ERROR(hr)
    // Get all endpoint devices
    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEndpointsCollection);
    EXIT_ON_ERROR(hr)
    UINT count;
    hr = pEndpointsCollection->GetCount(&count);
    EXIT_ON_ERROR(hr)
    if (count == 0)
    {
        printf("No endpoints found.\n");
    }
    endpointArray = Napi::Array::New(env, count);

    // Each loop prints the name of an endpoint device.
    for (ULONG i = 0; i < count; i++)
    {
        // Get pointer to endpoint number i.
        hr = pEndpointsCollection->Item(i, &pEndpoint);
        EXIT_ON_ERROR(hr)

        // Get the endpoint ID string.
        hr = pEndpoint->GetId(&pwszID);
        EXIT_ON_ERROR(hr)

        hr = pEndpoint->OpenPropertyStore(
            STGM_READ, &pProps);
        EXIT_ON_ERROR(hr)

        PROPVARIANT varName;
        // Initialize container for property value.
        PropVariantInit(&varName);

        // Get the endpoint's friendly-name property.
        hr = pProps->GetValue(
            PKEY_Device_FriendlyName, &varName);
        EXIT_ON_ERROR(hr)

        // Print endpoint friendly name and endpoint ID.
        printf("Endpoint %d: \"%S\" (%S)\n",
               i, varName.pwszVal, pwszID);
        std::wstring deviceName = varName.pwszVal;
        // jsObject.Set("DeviceName", Napi::Value::From(env, varName.pwszVal));
        jsObject.Set(Napi::Value::From(env, utf8_encode(deviceName)), Napi::Value::From(env, utf8_encode(pwszID)));
        // endpointArray[i] = Napi::Value::From(env, utf8_encode(deviceName));
        char intToChar = i;
        endpointMap[i] = intToChar;
        // endpointArray[i] = jsObject;

        CoTaskMemFree(pwszID);
        pwszID = NULL;
        PropVariantClear(&varName);
        SAFE_RELEASE(pProps)
        SAFE_RELEASE(pEndpoint)
    }
    SAFE_RELEASE(pEnumerator)
    SAFE_RELEASE(pEndpointsCollection)
Exit:
    SAFE_RELEASE(pEnumerator)

    std::cout << "Map size: " << endpointMap.size() << std::endl;
    return jsObject;
}

Napi::Value getMapSize(const Napi::CallbackInfo &info)
{
    // Check if env was success
    Napi::Env env = info.Env();
    return Napi::Value::From(env, endpointMap.size());
}

void stopSong(const Napi::CallbackInfo &info)
{
    if (musicThread.joinable())
    {

        stopMusicFlag = true;
        musicThread.join();
        musicRunning = false;
    }
}

void listenToMouseStart(const Napi::CallbackInfo &info)
{
    startMouseListener(info);
}

void listVoiceQuips(const Napi::CallbackInfo &info)
{
    std::string path = "C:/Users/power/Desktop/DEMUT_WAV_CLIPS";
    const std::filesystem::path fPath = "C:/Users/power/Desktop/DEMUT_WAV_CLIPS";
    for (auto const &dir_entry : std::filesystem::directory_iterator{fPath})
    {
        std::cout << dir_entry.path() << '\n';
    }
}

void playClip(const Napi::CallbackInfo &info)
{
    std::string clipName = info[0].ToString().Utf8Value();
    Napi::Env env = info.Env();
    if (musicRunning)
    {
        stopSong(info);
    }
    stopMusicFlag = false;
    musicRunning = true;
    musicThread = std::thread(playClipFromFile, clipName);
}

void setAudioEndpointDeviceId(const Napi::CallbackInfo &info)
{
    std::string audioEndpointIdString = info[0].ToString().Utf8Value();
    std::cout << "audio endpoint id" << audioEndpointIdString << std::endl;
    std::wstring audioEndpointIdWide = std::wstring(audioEndpointIdString.begin(), audioEndpointIdString.end());
    audioEndpointId = audioEndpointIdWide.c_str();
}

void synthesizeTextToAudioFile(const Napi::CallbackInfo &info)
{
    std::string textToSynthesize = info[0].ToString().Utf8Value();
    std::string newSpeechFilename = info[1].ToString().Utf8Value();
    writeOverSynthString(textToSynthesize);
    textToSpeechFile(newSpeechFilename);
}

void synthesizeTextToVoice(const Napi::CallbackInfo &info)
{
    std::string textToSynthesize = info[0].ToString().Utf8Value();
    
    Napi::Env env = info.Env();
    if (musicRunning)
    {
        stopSong(info);
    }
    stopMusicFlag = false;
    musicRunning = true;
    musicThread = std::thread(playDirectSynthesizedAudio, textToSynthesize);
}

// Declare JS functions and map them to native functions
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "getMapSize"), Napi::Function::New(env, getMapSize));
    exports.Set(Napi::String::New(env, "getAudioEndpoints"), Napi::Function::New(env, getAudioEndpoints));
    exports.Set(Napi::String::New(env, "stopSong"), Napi::Function::New(env, stopSong));
    exports.Set(Napi::String::New(env, "startMouseListener"), Napi::Function::New(env, listenToMouseStart));
    exports.Set(Napi::String::New(env, "listAudioClips"), Napi::Function::New(env, listVoiceQuips));
    exports.Set(Napi::String::New(env, "playClip"), Napi::Function::New(env, playClip));
    exports.Set(Napi::String::New(env, "setAudioEndpointDeviceId"), Napi::Function::New(env, setAudioEndpointDeviceId));
    exports.Set(Napi::String::New(env, "synthesizeTextToAudioFile"), Napi::Function::New(env, synthesizeTextToAudioFile));
    exports.Set(Napi::String::New(env, "simulateVoice"), Napi::Function::New(env, synthesizeTextToVoice));
    return exports;
}

NODE_API_MODULE(addon, Init)