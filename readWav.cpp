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

void swap_endian16(uint16_t val) {
	val = (val << 8) | (val >> 8);
}

void swap_endian32(uint32_t val) {
	val = (val << 24) | ((val << 8) & 0x00ff0000) |
		((val >> 8) & 0x0000ff00) | (val >> 24);
}

int readFile(WAV_HEADER* wavHeader)
{

	AudioFile<double> audioFile;

	audioFile.load("C:/Users/power/Downloads/mil.wav");

	int channel = 0;
	int numSamples = audioFile.getNumSamplesPerChannel();

	wavHeader->pFloatdata.resize(2);
	wavHeader->pFloatdata[0].resize(numSamples);
	wavHeader->pFloatdata[1].resize(numSamples);

	for (int x = 0; x < numSamples; x++) {
		for (int y = 0; y < 2; y++) {
			wavHeader->pFloatdata[y][x] = audioFile.samples[y][x];
		}
	}

	for (int i = 100000; i < 100010; i++) {
		std::cout << wavHeader->pFloatdata[1][i] << std::endl;
		if (i == 100009) {
			std::cout << "CHANNEL SWITCH" << std::endl;
		}
	}

	for (int i = 100000; i < 100010; i++) {
		std::cout << wavHeader->pFloatdata[0][i] << std::endl;
	}

	// typedef std::vector<std::vector<T> > AudioBuffer;
	//for (int i = 100000; i < 104000; i++)
	//{
	//	double currentSample = audioFile.samples[1][i];
	//	float currFloatSample = (float)currentSample;
	//	std::cout << currentSample << std::endl;
	//	wavHeader->pFloatdata[i] = currentSample;
	//}


	return 0;
	//int headerSize = sizeof(WAV_HEADER), filelength = 0;
	////const char* filePath;
	////string input;

	////cout << "Input wave file name: ";
	////cin >> input;
	////cin.get();
	////filePath = input.c_str();
	//std::cout << "Headersize ----> " << headerSize << std::endl;
	//FILE* wavFile = fopen("C:/Users/power/Downloads/misha.wav", "r");
	//if (wavFile == nullptr)
	//{
	//	// fprintf(stderr, "Unable to open wave file: %s\n", filePath);
	//	return 1;
	//}

	//// Read the header
	//size_t bytesRead = fread(wavHeader, 1, headerSize , wavFile);
	//cout << "Header Read " << bytesRead << " bytes." << endl;
	//if (bytesRead > 0)
	//{
	//	// Read the data
	//	uint16_t bytesPerSample = wavHeader->bitsPerSample / 8;      // Number     of bytes per sample
	//	uint64_t numSamples = wavHeader->ChunkSize / bytesPerSample; // How many samples are in the wav file?
	//	static const uint16_t BUFFER_SIZE = 4096;
	//	int8_t* buffer = new int8_t[BUFFER_SIZE];

	//	while ((bytesRead = fread(buffer, sizeof buffer[0], BUFFER_SIZE / (sizeof buffer[0]), wavFile)) > 0)
	//	{
	//		/** DO SOMETHING WITH THE WAVE DATA HERE **/
	//		cout << "Readssssssssssssssssssssssssssssss " << bytesRead << " bytes." << endl;
	//	}
	//	//delete[] buffer;
	//	//buffer = nullptr;
	//	filelength = getFileSize(wavFile);

	//	cout << "File is                    :" << filelength << " bytes." << endl;
	//	cout << "RIFF header                :" << wavHeader->RIFF[0] << wavHeader->RIFF[1] << wavHeader->RIFF[2] << wavHeader->RIFF[3] << endl;
	//	cout << "WAVE header                :" << wavHeader->WAVE[0] << wavHeader->WAVE[1] << wavHeader->WAVE[2] << wavHeader->WAVE[3] << endl;
	//	cout << "FMT                        :" << wavHeader->fmt[0] << wavHeader->fmt[1] << wavHeader->fmt[2] << wavHeader->fmt[3] << endl;
	//	cout << "Data size                  :" << wavHeader->ChunkSize << endl;

	//	// Display the sampling Rate from the header
	//	cout << "Sampling Rate              :" << wavHeader->SamplesPerSec << endl;
	//	cout << "Number of bits used        :" << wavHeader->bitsPerSample << endl;
	//	cout << "Number of channels         :" << wavHeader->NumOfChan << endl;
	//	cout << "Number of bytes per second :" << wavHeader->bytesPerSec << endl;
	//	cout << "Data length(Subchunk1)     :" << wavHeader->Subchunk1Size << endl;
	//	cout << "Audio Format               :" << wavHeader->AudioFormat << endl;
	//	// Audio format 1=PCM,6=mulaw,7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM

	//	cout << "Block align                :" << wavHeader->blockAlign << endl;
	//	cout << "Data string                :" << wavHeader->Subchunk2ID[0] << wavHeader->Subchunk2ID[1] << wavHeader->Subchunk2ID[2] << wavHeader->Subchunk2ID[3] << endl;
	//	cout << "Data length(Subchunk2Size) :" << wavHeader->Subchunk2Size << endl;

	//	for (unsigned long i = 0; i < 25; i++)
	//	{
	//		std::cout << buffer[i] << std::endl;
	//	}
	//	wavHeader->pFloatdata = new float[BUFFER_SIZE];
	//	// Convert the buffer to floats. (before resampling)
	//	const float div = (1.0f / 32768.0f);
	//	for (int i = 0; i < BUFFER_SIZE; i++) {
	//		wavHeader->pFloatdata[i] = div * (float)buffer[i];
	//	}		
	//}
	//// wavHeader->data = (unsigned char*)malloc(sizeof(unsigned char) * wavHeader->Subchunk2Size); //set aside sound buffer space

	//// unsigned char* data = new unsigned char[4096]; //set aside sound buffer space

	//
	//// fread(data, sizeof(unsigned char), wavHeader->Subchunk2Size, fp); //read in our whole sound data chunk
	//


	//// wavHeader->pFloatdata = new float[BUFFER_SIZE];

	//// Convert the buffer to floats. (before resampling)
	////const float div = (1.0f / 32768.0f);
	////for (int i = 0; i < BUFFER_SIZE; i++) {
	////	wavHeader->pFloatdata[i] = div * (float)wavH.data[i];
	////}
	//fclose(wavFile);

	//return 0;
}

// find the file size
int getFileSize(FILE* inFile)
{
	int fileSize = 0;
	fseek(inFile, 0, SEEK_END);

	fileSize = ftell(inFile);

	fseek(inFile, 0, SEEK_SET);
	return fileSize;
}