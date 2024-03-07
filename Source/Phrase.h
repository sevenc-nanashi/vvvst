#pragma once

#include <choc/audio/choc_AudioFileFormat_WAV.h>

#include <string>

class Phrase {
 public:
  Phrase(std::string_view id, double startTime, double endTime, choc::audio::AudioFileData data, std::string hash)
      : id(std::string(id)), startTime(startTime), endTime(endTime), data(data), hash(hash) {}

  std::string id;
  double startTime;
  double endTime;
  choc::audio::AudioFileData data;
  std::string hash;
};
