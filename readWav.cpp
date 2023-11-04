#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include "readwavHeader.h"
#include "customHeader.h"
#include <shlobj.h>
#include <filesystem>
using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::string;

void swap_endian16(uint16_t val)
{
	val = (val << 8) | (val >> 8);
}

void swap_endian32(uint32_t val)
{
	val = (val << 24) | ((val << 8) & 0x00ff0000) |
		  ((val >> 8) & 0x0000ff00) | (val >> 24);
}

std::filesystem::path getUserDocumentsDirectoryLazyOne()
{
	wchar_t profilePath[MAX_PATH];
	HRESULT hr = SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, profilePath);

	if (SUCCEEDED(hr))
	{
		std::filesystem::path documentsPath = profilePath;
		documentsPath /= L"Documents";
		return documentsPath;
	}

	// Fallback in case SHGetFolderPathW fails
	return L"";
}

int readFile(WAV_HEADER *wavHeader, string clipName)
{

	static AudioFile<float> audioFile;

	// std::filesystem::path path{"ProgramData/DEMUT_WAV_CLIPS"};
	// std::string path = "C:/ProgramData/Demut/DEMUT_WAV_CLIPS/";
	// path.append(clipName + ".wav");
	std::filesystem::path fullFilePath = getUserDocumentsDirectoryLazyOne();
	fullFilePath /= L"Demut";
	fullFilePath /= L"DEMUT_WAV_CLIPS";

	// Now you can add your clip name to the path as needed
	fullFilePath /= std::wstring(clipName.begin(), clipName.end()) + L".wav";
	// audioFile.load("C:/Users/power/Downloads/sharks.wav");
	audioFile.load(fullFilePath.generic_string());

	audioFile.printSummary();
	int channel = 0;
	int numSamples = audioFile.getNumSamplesPerChannel();
	wavHeader->pFloatdata.resize(2);
	wavHeader->pFloatdata[0].resize(numSamples);
	wavHeader->pFloatdata[1].resize(numSamples);

	for (int x = 0; x < numSamples; x++)
	{
		for (int y = 0; y < 2; y++)
		{
			wavHeader->pFloatdata[y][x] = audioFile.samples[y][x];
		}
	}
	audioFile.samples.clear();
	audioFile.samples.shrink_to_fit();
	return 0;
}

// find the file size
int getFileSize(FILE *inFile)
{
	int fileSize = 0;
	fseek(inFile, 0, SEEK_END);

	fileSize = ftell(inFile);

	fseek(inFile, 0, SEEK_SET);
	return fileSize;
}