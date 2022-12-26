//-----------------------------------------------------------
// Play an audio stream on the default audio rendering
// device. The PlayAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data to the
// rendering device. The inner loop runs every 1/2 second.
//-----------------------------------------------------------

#include <Windows.h>
#include <math.h>
#include <iostream>
// Windows multimedia device
#include <Mmdeviceapi.h>
#include <string>
// WASAPI
#include <Audioclient.h>
#include "readWavHeader.h"
#include <Functiondiscoverykeys_devpkey.h>
#include "audioPlayer.h"

class MyAudioSource
{
public:
	// empty constructor
	MyAudioSource();
	// ~ tilde means its a destructor and runs when the instance of the object gets destroyed
	~MyAudioSource();

	HRESULT SetFormat(WAVEFORMATEX *);
	HRESULT LoadData(UINT32, BYTE *, DWORD *, WAV_HEADER *);
	void init(WAV_HEADER *);

private:
	bool initialised = false;
	WAVEFORMATEXTENSIBLE format;
	unsigned int pcmPos = 0;
	UINT32 bufferSize;
	UINT32 bufferPos = 0;
	std::vector<std::vector<float>> pcmAudio;
};

//-----------------------------------------------------------
MyAudioSource::MyAudioSource()
{
	std::cout << "Starting my audio source..." << std::endl;
}
//-----------------------------------------------------------

MyAudioSource::~MyAudioSource()
{
	pcmAudio.clear();
	pcmAudio.shrink_to_fit();
}
//-----------------------------------------------------------

void MyAudioSource::init(WAV_HEADER *wavHeader)
{
	pcmAudio = wavHeader->pFloatdata;
	initialised = true;
	std::cout << "initialized the audio source..." << wavHeader->pFloatdata.size() << std::endl;
}
//-----------------------------------------------------------
HRESULT MyAudioSource::SetFormat(WAVEFORMATEX *wfex)
{
	if (wfex->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		format = *reinterpret_cast<WAVEFORMATEXTENSIBLE *>(wfex);
	}
	else
	{
		format.Format = *wfex;
		format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		INIT_WAVEFORMATEX_GUID(&format.SubFormat, wfex->wFormatTag);
		format.Samples.wValidBitsPerSample = format.Format.wBitsPerSample;
		format.dwChannelMask = 0;
	}
	const UINT16 formatTag = EXTRACT_WAVEFORMATEX_ID(&format.SubFormat);

	std::cout << "Channel Count: " << format.Format.nChannels << '\n';
	std::cout << "Audio Format: ";
	switch (formatTag)
	{
	case WAVE_FORMAT_IEEE_FLOAT:
		std::cout << "WAVE_FORMAT_IEEE_FLOAT\n";
		break;
	case WAVE_FORMAT_PCM:
		std::cout << "WAVE_FORMAT_PCM\n";
		break;
	default:
		std::cout << "Wave Format Unknown\n";
		break;
	}
	return 0;
}
HRESULT MyAudioSource::LoadData(UINT32 totalFrames, BYTE *dataOut, DWORD *flags, WAV_HEADER *wavHeader)
{

	float *fData = (float *)dataOut;
	UINT32 totalSamples = totalFrames * format.Format.nChannels;
	std::cout << "Starting loading data 1: "
			  << totalSamples << " | " << pcmPos << std::endl;
	std::cout << "pcm audio : " << pcmAudio.size() << std::endl;
	std::cout << "pcmAudio[0].size() : " << pcmAudio[0].size() << std::endl;
	if (pcmPos < pcmAudio[0].size())
	{
		std::cout << "Starting loading data: " << totalSamples << " | " << pcmPos << std::endl;
		for (UINT32 i = 0; i < totalSamples; i += format.Format.nChannels)
		{
			// This is writing to both channels
			for (size_t chan = 0; chan < format.Format.nChannels; chan++)
			{
				fData[i + chan] = (pcmPos < pcmAudio[0].size()) ? pcmAudio[chan][pcmPos] : 0.0f;
			}
			pcmPos++;
		}
	}
	else
	{
		*flags = AUDCLNT_BUFFERFLAGS_SILENT;
	}
	return 0;
}
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

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

namespace sourceComs
{
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

}
/* Each COM class is identified by a CLSID, a unique 128-bit GUID, which the server must register.
COM uses this CLSID, at the request of a client, to associate specific data with the DLL or EXE containing the code that implements the class,
 thus creating an instance of the object. */
// IID - Describes a GUID structure used to describe an identifier for a MAPI interface. --> https://docs.microsoft.com/en-us/office/client-developer/outlook/mapi/iid
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

HRESULT PlayAudioStream(WAV_HEADER *wavHeader)
{

	std::cout << "Audio stream checkpoint 1" << std::endl;

	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	IAudioRenderClient *pRenderClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	UINT32 numFramesPadding;
	BYTE *pData;
	DWORD flags = 0;
	MyAudioSource *pMySource = new MyAudioSource();
	pMySource->init(wavHeader);

	std::cout << "Audio stream checkpoint 2" << std::endl;

	hr = CoCreateInstance(
		sourceComs::CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, sourceComs::IID_IMMDeviceEnumerator,
		(void **)&pEnumerator);
	EXIT_ON_ERROR(hr);
	std::cout << "Audio stream checkpoint 3" << std::endl;

	// GETTING THE DEFAULT AUDIO ENDPOINT DEVICE
	if (audioEndpointId == NULL)
	{

		hr = pEnumerator->GetDefaultAudioEndpoint(
			eRender, eMultimedia, &pDevice);
		std::cout << "Audio stream checkpoint endpoint checkpoint 1" << std::endl;

		EXIT_ON_ERROR(hr);
	}
	else
	{
		std::wstring wstr(audioEndpointId);
		std::string str(wstr.begin(), wstr.end());

		hr = pEnumerator->GetDevice(audioEndpointId, &pDevice);
		std::cout << "Audio stream checkpoint endpoint checkpoint 2" << str << std::endl;

		EXIT_ON_ERROR(hr);
	}
	std::cout << "Audio stream checkpoint 4" << std::endl;

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void **)&pAudioClient);
	std::cout << "Audio stream checkpoint 5" << std::endl;

	EXIT_ON_ERROR(hr);
	// The GetMixFormat method retrieves the stream format that the audio engine uses for its internal processing of shared-mode streams.
	hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr);
	std::cout << "Got mix format: " << pwfx->wFormatTag << std::endl;
	std::cout << "Audio stream checkpoint 6" << std::endl;

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
	EXIT_ON_ERROR(hr);
	std::cout << "Audio stream checkpoint 7" << std::endl;

	// Tell the audio source which format to use.
	hr = pMySource->SetFormat(pwfx);
	EXIT_ON_ERROR(hr);
	std::cout << "Audio stream checkpoint 8" << std::endl;

	// Get the actual size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr);
	std::cout << "Audio stream checkpoint 9" << std::endl;

	hr = pAudioClient->GetService(
		IID_IAudioRenderClient,
		(void **)&pRenderClient);
	EXIT_ON_ERROR(hr);
	std::cout << "Audio stream checkpoint 10" << std::endl;

	// Grab the entire buffer for the initial fill operation.
	hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
	EXIT_ON_ERROR(hr);
	std::cout << "Audio stream checkpoint 11" << std::endl;

	// Load the initial data into the shared buffer.
	hr = pMySource->LoadData(bufferFrameCount, pData, &flags, wavHeader);
	std::cout << "Audio stream checkpoint 12" << std::endl;

	std::cout << "got done loading data" << std::endl;

	EXIT_ON_ERROR(hr);
	hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
	std::cout << "got done releasing buffer" << std::endl;

	EXIT_ON_ERROR(hr);

	// Calculate the actual duration of the allocated buffer.
	hnsActualDuration = (double)REFTIMES_PER_SEC *
						bufferFrameCount / pwfx->nSamplesPerSec;
	std::cout << "Calculated 'hnsActualDuration': " << hnsActualDuration << std::endl;
	hr = pAudioClient->Start(); // Start playing.
	std::cout << "Started playing...." << std::endl;

	EXIT_ON_ERROR(hr);

	// Each loop fills about half of the shared buffer.
	while (flags != AUDCLNT_BUFFERFLAGS_SILENT && !stopMusicFlag)
	{
		// Sleep for half the buffer duration.
		Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

		// See how much buffer space is available.
		hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
		EXIT_ON_ERROR(hr);

		numFramesAvailable = bufferFrameCount - numFramesPadding;

		// Grab all the available space in the shared buffer.
		hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
		EXIT_ON_ERROR(hr);

		// Get next 1/2-second of data from the audio source.
		hr = pMySource->LoadData(numFramesAvailable, pData, &flags, wavHeader);
		EXIT_ON_ERROR(hr);

		hr = pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
		EXIT_ON_ERROR(hr);
	}

	if (!stopMusicFlag)
	{
		// Wait for last data in buffer to play before stopping.
		Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));
	}

	hr = pAudioClient->Stop(); // Stop playing.
	EXIT_ON_ERROR(hr);

Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator);
	SAFE_RELEASE(pDevice);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pRenderClient);
	wavHeader->pFloatdata.clear();
	wavHeader->pFloatdata.shrink_to_fit();
	delete pMySource;
	return hr;
}

int playClipFromFile(std::string clipName)
{
	WAV_HEADER wavHeader;
	std::cout << clipName << std::endl;

	readFile(&wavHeader, clipName);
	CoInitialize(nullptr);
	PlayAudioStream(&wavHeader);
	CoUninitialize();
	return 0;
}

// int main()
//{
//	WAV_HEADER wavHeader;
//
//	readFile(&wavHeader, "sharks");
//	CoInitialize(nullptr);
//	std::cout << "Hello world!" << wavHeader.AudioFormat << std::endl;
//	PlayAudioStream(&wavHeader);
//	CoUninitialize();
//	return 0;
// }