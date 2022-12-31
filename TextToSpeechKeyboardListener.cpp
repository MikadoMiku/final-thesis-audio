// KeyLoggerTTS.cpp : This file contains the 'main' function. Program execution begins and ends there.

// sphelper.h uses deprecated api, defining build_windows fixes it -> https://stackoverflow.com/questions/22303824/warning-c4996-getversionexw-was-declared-deprecated
#define BUILD_WINDOWS
#include <Windows.h>
#include <sapi.h>
#include <iostream>
#include <string>
#include <thread>
#include <atlbase.h>
#include <sphelper.h>
#include "audioPlayer.h"
#include "readWavHeader.h"
#include "ttsFunctionality.h"
#include <vector>
#include <iterator>
#include <filesystem>

enum class Endianness
{
	LittleEndian,
	BigEndian
};

int16_t twoBytesToInt(std::vector<uint8_t> &source, int startIndex, Endianness endianness)
{
	int16_t result;

	if (endianness == Endianness::LittleEndian)
		result = (source[startIndex + 1] << 8) | source[startIndex];
	else
		result = (source[startIndex] << 8) | source[startIndex + 1];

	return result;
}

float sixteenBitIntToSample(int16_t sample)
{
	return static_cast<float>(sample) / static_cast<float>(32768.);
}

#define EXIT_ON_ERROR(hres)                 \
	if (FAILED(hres))                       \
	{                                       \
		std::wcout << "ERROR" << std::endl; \
		goto Exit;                          \
	}
#define SAFE_RELEASE(punk) \
	if ((punk) != NULL)    \
	{                      \
		(punk)->Release(); \
		(punk) = NULL;     \
	}

std::thread nativeThread;

std::wstring ws;

bool listening = FALSE;

// variable to store the HANDLE to the hook. Don't declare it anywhere else then
// globally or you will get problems since every function uses this variable.
HHOOK _hook;

void writeOverSynthString(std::string newString)
{
	ws.clear();
	std::wstring newWideString(newString.begin(), newString.end());
	ws.append(newWideString);
}

// Speech synthesizer and file creation
int textToSpeechFile(std::string fileName)
{
	CComPtr<ISpVoice> cpVoice;
	CComPtr<ISpStream> cpStream;
	CComPtr<IStream> cpBaseStream;
	CSpStreamFormat cAudioFmt;
	HRESULT hr;
	WAVEFORMATEX pcmWaveFormat;
	WAV_HEADER wavHeader;
	pcmWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	pcmWaveFormat.nChannels = 2;
	pcmWaveFormat.nSamplesPerSec = 48000L;
	pcmWaveFormat.nAvgBytesPerSec = 176400L;
	pcmWaveFormat.nBlockAlign = 4;
	pcmWaveFormat.wBitsPerSample = 16;
	pcmWaveFormat.cbSize = 0;

	// Dynamic file creation path
	std::filesystem::path fullFilePath{"C:/ProgramData/Demut/DEMUT_WAV_CLIPS/" + fileName + ".wav"};
	// std::string fullFilePath = "C:\\Users\\power\\Desktop\\DEMUT_WAV_CLIPS\\";
/* 	fullFilePath.append("\\\\");
	fullFilePath.append(fileName);
	fullFilePath.append(".wav"); */
	// std::wstring wideFullFilePath(fullFilePath.begin(), fullFilePath.end());
	std::cout << fullFilePath << std::endl;
	hr = CoInitialize(nullptr);

	EXIT_ON_ERROR(hr);

	hr = cpVoice.CoCreateInstance(CLSID_SpVoice);

	EXIT_ON_ERROR(hr);

	hr = cAudioFmt.AssignFormat(&pcmWaveFormat);

	EXIT_ON_ERROR(hr);

	hr = SPBindToFile(fullFilePath.c_str(), SPFM_CREATE_ALWAYS, &cpStream, &cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr());

	EXIT_ON_ERROR(hr);

	hr = cpVoice->SetOutput(cpStream, TRUE);

	EXIT_ON_ERROR(hr);

	hr = cpVoice->Speak(ws.c_str(), 0, NULL);

	EXIT_ON_ERROR(hr);

	hr = cpVoice->WaitUntilDone(INFINITE);

	EXIT_ON_ERROR(hr);

	Sleep(2000);

	// playClipFromFile(fileName);

	hr = cpStream->Close();

	EXIT_ON_ERROR(hr);

Exit:
	// Release the stream and voice object
	cpStream.Release();
	cpVoice.Release();
	return 0;
}

// Keyboard listener
int secondMain()
{
	std::wcout << "Starting...." << std::endl;

	std::wcout << "Created context...." << std::endl;

	// Create a native thread with its own message loop which is required to
	// attach low level keyboard hooks in order not to block the main thread
	nativeThread = std::thread([]
							   {
								   std::wcout << "Starting thread...." << std::endl;

								   // This is the callback function. Consider it the event that is raised when,
								   // in this case, a key is pressed or released.
								   static auto HookCallback = [](int nCode, WPARAM wParam, LPARAM lParam) -> LRESULT
								   {
									   try
									   {
										   if (nCode >= 0)
										   {
											   // lParam is the pointer to the struct containing the data needed,
											   // so cast and assign it to kdbStruct.
											   LPKBDLLHOOKSTRUCT k = (LPKBDLLHOOKSTRUCT)lParam;
											   BYTE keyStates[256];
											   GetKeyboardState(keyStates);

											   WCHAR TransedChar = NULL;
											   if (wParam == WM_KEYDOWN)
											   {
												   // This should start and end the listening of the keyboard, ending means it should start processing.
												   if (k->vkCode == VK_RETURN)
												   {
													   if (listening)
													   {
														   // Create TTS file
														   listening = FALSE;
														   textToSpeechFile("keyboardListenerTest");
														   ws.clear();
													   }
													   else
													   {
														   listening = TRUE;
													   }
												   }
												   // If problems of getting correct shift toggle, look into async version of getting key state. 0x8000 is for getting the high order bit of the 16 bit return
												   if (!listening || k->vkCode == 160)
												   {
													   return CallNextHookEx(_hook, nCode, wParam, lParam);
												   }
												   // must be a better way of getting correct vk code for delete and shift and such, maybe with like mapvirutalkey?
												   if (k->vkCode == 8)
												   {
													   std::wcout << "deleting char" << std::endl;
													   ws.erase(ws.length() - 1);
													   std::wcout << ws << std::endl;
												   }
												   // Output mouse input to console:
												   // ToUnicodeEx(k->vkCode, k->scanCode, keyStates, &TransedChar, 1, 0, GetKeyboardLayout(0)); // https://cpp.hotexamples.com/examples/-/-/ToUnicodeEx/cpp-tounicodeex-function-examples.html
												   // std::wcout << TransedChar << " | " << k->scanCode << " | " << k->vkCode << std::endl;

												   // implicit casting with char type, otherwise we print out an integer
												   char character = MapVirtualKey(k->vkCode, MAPVK_VK_TO_CHAR);
												   ws += character;
												   std::wcout << character << std::endl;
											   }
										   }
									   }
									   catch (...)
									   {
										   std::wcout << "Something went wrong while handling the key event" << std::endl;
									   }

									   // call the next hook in the hook chain. This is nessecary or your hook
									   // chain will break and the hook stops
									   // Passes hook event to other installed hooks, without this other programs will not receive the event.
									   return CallNextHookEx(_hook, nCode, wParam, lParam);
								   };

								   // Set the hook and set it to use the callback function above
								   // WH_KEYBOARD_LL means it will set a low level keyboard hook. More
								   // information about it at MSDN. The last 2 parameters are NULL, 0 because
								   // the callback function is in the same thread and window as the function
								   // that sets and releases the hook.
								   if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
								   {
									   std::wcout << "Failed to install hook!" << std::endl;
								   }

								   // Create a message loop
								   MSG msg;
								   bool done = FALSE;

								   while (!done)
								   {
									   if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
									   {
										   if (msg.message == WM_QUIT)
										   {
											   done = TRUE;
										   }
										   else
										   {
											   TranslateMessage(&msg);
											   DispatchMessage(&msg);
										   }
									   }
								   } });
	nativeThread.join();
	UnhookWindowsHookEx(_hook);
	std::wcout << "Ending...." << std::endl;
	return 0;
}

// SAPI get raw audio data -- https://github.com/yashasvigirdhar/MS-SAPI-demo/blob/master/DemoApp1/DemoApp1/GetRawAudioData.cpp
// int main(int argc, char *argv[])
int synthesizeVoice(std::string synthText, WAV_HEADER *wavHeader)
{
	HRESULT hr = S_OK;
	CComPtr<ISpVoice> cpVoice;
	CComPtr<ISpStream> cpStream;
	CComPtr<IStream> cpBaseStream;
	GUID guidFormat;
	WAVEFORMATEX *pWavFormatEx = nullptr;
	std::wstring wSynthText(synthText.begin(), synthText.end());

	if (FAILED(::CoInitialize(NULL)))
		return FALSE;

	std::cout << "initialized\n";

	if (SUCCEEDED(hr))
	{
		std::cout << "creating voice\n";
		hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
	}

	if (SUCCEEDED(hr))
	{
		std::cout << "voice created, initialzing istream\n";
		hr = cpStream.CoCreateInstance(CLSID_SpStream);
	}
	if (SUCCEEDED(hr))
	{
		std::cout << "istream created, creating basestream\n";
		hr = CreateStreamOnHGlobal(NULL, TRUE, &cpBaseStream);
	}
	if (SUCCEEDED(hr))
	{
		std::cout << "basestream created, setting format\n";
		hr = SpConvertStreamFormatEnum(SPSF_48kHz16BitStereo, &guidFormat,
									   &pWavFormatEx);
	}
	if (SUCCEEDED(hr))
	{
		std::cout << "format set,setting the basestream\n";
		hr = cpStream->SetBaseStream(cpBaseStream, guidFormat,
									 pWavFormatEx);
		// cpBaseStream.Release();
	}
	if (SUCCEEDED(hr))
	{
		std::cout << "basestream set, setting cpvoice output\n";
		hr = cpVoice->SetOutput(cpStream, TRUE);
		if (SUCCEEDED(hr))
		{
			std::cout << "output set, speaking\n";
			SpeechVoiceSpeakFlags my_Spflag = SpeechVoiceSpeakFlags::SVSFlagsAsync; // declaring and initializing Speech Voice Flags
			hr = cpVoice->Speak(wSynthText.c_str(), my_Spflag, NULL);
			cpVoice->WaitUntilDone(-1);
		}
	}
	if (SUCCEEDED(hr))
	{
		std::cout << "spoken correctly\n";

		/*To verify that the data has been written correctly, uncomment this, you should hear the voice.
			cpVoice->SetOutput(NULL, FALSE);
			cpVoice->SpeakStream(cpStream, SPF_DEFAULT, NULL);
			*/

		// After SAPI writes the stream, the stream position is at the end, so we need to set it to the beginning.
		_LARGE_INTEGER a = {0};
		hr = cpStream->Seek(a, STREAM_SEEK_SET, NULL);

		// get the base istream from the ispstream
		IStream *pIstream;
		cpStream->GetBaseStream(&pIstream);

		// calculate the size that is to be read
		STATSTG stats;
		pIstream->Stat(&stats, STATFLAG_NONAME);

		ULONG sSize = stats.cbSize.QuadPart; // size of the data to be read
		std::cout << "size : " << stats.cbSize.QuadPart << std::endl;

		ULONG bytesRead;					   //	this will tell the number of bytes that have been read
		uint8_t *pBuffer = new uint8_t[sSize]; // buffer to read the data
		std::vector<uint8_t> buff;
		buff.resize(sSize);

		// read the data into the buffer
		pIstream->Read(reinterpret_cast<char *>(buff.data()), sSize, &bytesRead);

		wavHeader->pFloatdata.resize(2);
		wavHeader->pFloatdata[0];
		wavHeader->pFloatdata[1];
		int samplesStartIndex = 0;
		int numSamples = sSize / (2 * 16 / 8);
		uint16_t numBytesPerSample = static_cast<uint16_t>(16) / 8;
		uint16_t numBytesPerBlock = static_cast<uint16_t>(pWavFormatEx->nAvgBytesPerSec);
		int sampleIndex = 0;
		int random = 0;

		// ----------------- mess with buff -------------------------------------

		std::vector<uint8_t> buff2;
		buff2;
		std::cout << "THIS IS BUFF 2 ---> " << buff2.size() << std::endl;

		for (int buffIndex = 0; buffIndex < bytesRead; buffIndex++)
		{
			if (buff[buffIndex] == 0)
				continue;
			buff2.push_back(buff[buffIndex]);
		}
		std::cout << "sSize ---> " << sSize << std::endl;
		std::cout << "ReadBytes ---> " << bytesRead << std::endl;
		std::cout << "THIS IS BUFF 2 ---> " << buff2.size() << std::endl;
		// --------------------------------------------------------------------
		for (int i = 0; i < numSamples; i++)
		{
			for (int channel = 0; channel < 2; channel++)
			{
				// int sampleIndex = samplesStartIndex + (numBytesPerBlock * i) + channel * numBytesPerSample;
				// std::cout << "sampleIndex: " << sampleIndex << std::endl;

				int16_t sampleAsInt = twoBytesToInt(buff, sampleIndex, Endianness::LittleEndian);
				/* 				if (random < 25000)
								{
									if (unsigned(sampleAsInt) == 0)
									{
										sampleIndex += 4;
										break;
									}
								} */
				float sample = sixteenBitIntToSample(sampleAsInt);

				wavHeader->pFloatdata[channel].push_back(sample);
				sampleIndex += 2;
			}
			random++;
		}
		// --------------------------------------------------------------------

		// PlayAudioStream(&wavHeader);
		// playClipFromFile("sharks");
	}

	// don't forget to release everything
	cpStream.Release();
	cpVoice.Release();

	::CoUninitialize();
	return TRUE;
}
