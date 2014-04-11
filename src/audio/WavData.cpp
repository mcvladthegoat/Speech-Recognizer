#include <iostream>
#include <fstream>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <cassert>
#include "audio.h"
#include "WavData.h"
#include "Frame.h"

using namespace std;

namespace wtm {
namespace audio {

/**
 * Read Wav data from a file
 */
WavDataPtr WavData::readFromFile(const std::string& file) {
	WavHeader wavHeader;

	// Open file
	std::fstream fs;
	fs.open(file.c_str(), std::ios::in | std::ios::binary);

	if (!fs.good()) {
		fprintf(stderr, "Can't open the wave file\n");
		exit(EXIT_FAILURE);
	}

	// Read header
	fs.read((char*)(&wavHeader), sizeof(WavHeader));
	checkHeader(wavHeader);

	// Read raw data
	WavDataPtr wavData(new WavData(wavHeader));
	readRawData(fs, wavHeader, *wavData);
	fs.close();

	// Split data into frames
	wavData->divideIntoFrames();

	return wavData;
}

/**
 * Checks a set of restrictions
 */
void WavData::checkHeader(const WavHeader& wavHeader) {

	if (0 != strncmp(wavHeader.riff, "RIFF", sizeof(wavHeader.riff))
			|| 0 != strncmp(wavHeader.wave, "WAVE", sizeof(wavHeader.wave))) {
		fprintf(stderr, "Invalid RIFF/WAVE format\n");
		exit(EXIT_FAILURE);
	}

	if (1 != wavHeader.audioFormat) {
		fprintf(stderr, "Invalid WAV format: only PCM audio format is supported\n");
		exit(EXIT_FAILURE);
	}

	if (wavHeader.numOfChan > 2) {
		fprintf(stderr, "Invalid WAV format: only 1 or 2 channels audio is supported\n");
		exit(EXIT_FAILURE);
	}

	unsigned long bitsPerChannel = wavHeader.bitsPerSample / wavHeader.numOfChan;
	if (8 != bitsPerChannel && 16 != bitsPerChannel) {
		fprintf(stderr, "Invalid WAV format: only 8 and 16-bit per channel is supported\n");
		exit(EXIT_FAILURE);
	}

	if (wavHeader.subchunk2Size > LONG_MAX) {
		fprintf(stderr, "File too big\n");
		exit(EXIT_FAILURE);
	}
}

void WavData::readRawData(std::fstream& fs, const WavHeader& wavHeader, WavData& wavFile) {
	raw_t value, minValue = 0, maxValue = 0;
	uint8_t value8, valueLeft8, valueRight8;
	int16_t value16, valueLeft16, valueRight16;

	lenght_t bytesPerSample = static_cast<uint32_t>(wavHeader.bitsPerSample / 8);
	unsigned long numberOfSamplesXChannels = wavHeader.subchunk2Size / (wavHeader.numOfChan * bytesPerSample);

	unsigned long sampleNumber = 0;
	for (; sampleNumber < numberOfSamplesXChannels && !fs.eof(); sampleNumber++) {

		if (8 == wavHeader.bitsPerSample) {
			if (1 == wavHeader.numOfChan) {
				fs.read((char*)(&value8), sizeof(uint8_t));
				value = static_cast<raw_t>(value8);

			} else {
				fs.read((char*)(&valueLeft8), sizeof(uint8_t));
				fs.read((char*)(&valueRight8), sizeof(uint8_t));
				value = static_cast<raw_t>((abs(valueLeft8) + abs(valueRight8)) / 2);
			}
		} else {
			if (1 == wavHeader.numOfChan) {
				fs.read((char*)(&value16), sizeof(int16_t));
				value = static_cast<raw_t>(value16);

			} else {
				fs.read((char*)(&valueLeft16), sizeof(int16_t));
				fs.read((char*)(&valueRight16), sizeof(int16_t));
				value = static_cast<raw_t>((abs(valueLeft16) + abs(valueRight16)) / 2);
			}
		}

		if (maxValue < value) {
			maxValue = value;
		}

		if (minValue > value) {
			minValue = value;
		}

		wavFile.rawData->push_back(value);
	}
	assert(sampleNumber > 0);

	// Update values
	wavFile.setMinVal(minValue);
	wavFile.setMaxVal(maxValue);
	wavFile.setNumberOfSamples(sampleNumber);

	lenght_t bytesPerFrame = static_cast<lenght_t>(wavHeader.bytesPerSec * wavFile.frameLengthMs / 1000.0);
	wavFile.samplesPerFrame = static_cast<lenght_t>(bytesPerFrame / bytesPerSample);
	assert(wavFile.samplesPerFrame > 0);
}

void WavData::divideIntoFrames() {
	unsigned int samplesPerNonOverlap =
		static_cast<unsigned int>(samplesPerFrame * (1 - this->frameOverlap));

	unsigned int framesCount =
		(header.subchunk2Size / (header.bitsPerSample / 8)) / samplesPerNonOverlap;

	frames->reserve(framesCount);
	lenght_t indexBegin = 0, indexEnd = 0;
	for (unsigned int i = 0, size = rawData->size(); i < framesCount; ++i) {

		indexBegin = i * samplesPerNonOverlap;
		indexEnd = indexBegin + samplesPerFrame;
		if (indexEnd < size)
			frames->push_back(new Frame(*rawData, indexBegin, indexEnd));
		else
			break;
	}
}

} // namespace audio
} // namespace wtm