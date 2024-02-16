#pragma once

#include <string>
#include "choc/audio/choc_AudioFileFormat_WAV.h"

class Phrase {
public:
	Phrase(
		std::string_view id,
		double startTime,
		double endTime,
		choc::audio::AudioFileData data
	) :
		id(std::string(id)),
		startTime(startTime),
		endTime(endTime),
		data(data)
	{}

	std::string id;
	double startTime;
	double endTime;
	choc::audio::AudioFileData data;
};
