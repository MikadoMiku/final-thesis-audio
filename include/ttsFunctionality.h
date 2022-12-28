#include "string"
#include "readWavHeader.h"

void writeOverSynthString(std::string newString);

int textToSpeechFile(std::string fileName);

int synthesizeVoice(std::string synthText, WAV_HEADER *wavHeader);