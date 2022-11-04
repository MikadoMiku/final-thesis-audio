#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include "readwavHeader.h"
#include "customHeader.h"
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
    std::string path = "C:/Users/power/Desktop/DEMUT_WAV_CLIPS/";

	// audioFile.load("C:/Users/power/Downloads/sharks.wav");
	audioFile.load(path + clipName + ".wav");

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