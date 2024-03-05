#pragma once

#include <JuceHeader.h>

#include <unordered_map>

#include "Phrase.h"

class VVVSTAudioProcessor : public juce::AudioProcessor, public juce::ActionBroadcaster {
 public:
  VVVSTAudioProcessor();
  ~VVVSTAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  void setProject(std::string projectJson);
  std::string getProject();
  std::unordered_map<std::string, Phrase> phrases;

  double sampleRate;
  void setIsPlaying(bool isPlaying);

  juce::CriticalSection phrasesLock;

 private:
  std::string projectJson;
  std::atomic<bool> lastIsPlaying;
  std::atomic<bool> willSetIsPlaying;
  std::atomic<bool> willSetIsPlayingValue;
  std::atomic<double> lastTime;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VVVSTAudioProcessor)
};
