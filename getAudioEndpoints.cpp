#include <Windows.h>
#include <napi.h>

#include <Mmdeviceapi.h>
#include <iostream>
#include <Functiondiscoverykeys_devpkey.h>
#include <map>

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

// Convert a wide Unicode string to an UTF8 string
std::string utf8_encode(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

/* Each COM class is identified by a CLSID, a unique 128-bit GUID, which the server must register.
COM uses this CLSID, at the request of a client, to associate specific data with the DLL or EXE containing the code that implements the class,
 thus creating an instance of the object. */
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
// IID - Describes a GUID structure used to describe an identifier for a MAPI interface. --> https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/iid
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

std::map<int, char> endpointMap;

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
        CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
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
        jsObject.Set(Napi::Value::From(env, utf8_encode(deviceName)), Napi::Value::From(env, i));
        endpointArray[i] = Napi::Value::From(env, utf8_encode(deviceName));
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
    return endpointArray;
}

Napi::Value getMapSize(const Napi::CallbackInfo &info)
{
    // Check if env was success
    Napi::Env env = info.Env();
    return Napi::Value::From(env, endpointMap.size());
}

// Declare JS functions and map them to native functions
Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "getMapSize"), Napi::Function::New(env, getMapSize));
    exports.Set(Napi::String::New(env, "getAudioEndpoints"), Napi::Function::New(env, getAudioEndpoints));
    return exports;
}

NODE_API_MODULE(addon, Init)