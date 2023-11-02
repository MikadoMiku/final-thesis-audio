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

int readFile(WAV_HEADER *wavHeader, string clipName)
{

	static AudioFile<float> audioFile;

	// std::filesystem::path path{"ProgramData/DEMUT_WAV_CLIPS"};
	// std::string path = "C:/ProgramData/Demut/DEMUT_WAV_CLIPS/";
	// path.append(clipName + ".wav");
	PWSTR documentsPath;
	std::string path = "C:/ProgramData/Demut/DEMUT_WAV_CLIPS/";
	std::filesystem::path fullFilePath{path};

	if (SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &documentsPath) == S_OK)
	{
		fullFilePath = documentsPath;
		fullFilePath /= L"Demut/DEMUT_WAV_CLIPS";
		fullFilePath /= std::wstring(clipName.begin(), clipName.end()) + L".wav";

		CoTaskMemFree(documentsPath); // Free the allocated memory

		// Now 'fullFilePath' contains the path to the Documents folder with your desired subdirectory and filename.
	}
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