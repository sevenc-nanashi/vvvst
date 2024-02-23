#include "PluginProcessor.h"

#include "PluginEditor.h"

VVVSTAudioProcessor::VVVSTAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
  memory = juce::ValueTree("VVVST");
}

VVVSTAudioProcessor::~VVVSTAudioProcessor() {}

const juce::String VVVSTAudioProcessor::getName() const {
#ifdef JUCE_DEBUG
  return "VVVST [Debug]";
#else
  return "VVVST";
#endif
}

bool VVVSTAudioProcessor::acceptsMidi() const { return false; }

bool VVVSTAudioProcessor::producesMidi() const { return false; }

bool VVVSTAudioProcessor::isMidiEffect() const { return false; }

double VVVSTAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int VVVSTAudioProcessor::getNumPrograms() { return 1; }

int VVVSTAudioProcessor::getCurrentProgram() { return 0; }

void VVVSTAudioProcessor::setCurrentProgram(int index) {}

const juce::String VVVSTAudioProcessor::getProgramName(int index) { return {}; }

void VVVSTAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

void VVVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) { this->sampleRate = sampleRate; }

void VVVSTAudioProcessor::releaseResources() {}

void VVVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());

  auto* playHead = getPlayHead();
  if (playHead == nullptr) {
    return;
  }
  auto position = playHead->getPosition();
  if (!position.hasValue()) {
    return;
  }
  auto maybeCurrentTime = position->getTimeInSeconds();
  if (maybeCurrentTime.hasValue()) {
    double currentTime = *maybeCurrentTime;
    if (lastTime != currentTime) {
      juce::DynamicObject::Ptr json = new juce::DynamicObject();
      json->setProperty("type", "update:time");
      json->setProperty("time", currentTime);
      this->sendActionMessage(juce::JSON::toString(json.get()));

      lastTime = currentTime;
    }
    if (position->getIsPlaying()) {
      std::optional<Phrase> maybeCurrentPhrase;
      for (auto& [_, phrase] : phrases) {
        if (phrase.startTime <= currentTime && phrase.endTime >= currentTime) {
          if (maybeCurrentPhrase.has_value()) {
            auto& currentPhrase = maybeCurrentPhrase.value();
            auto& newPhrase = phrase;
            if (newPhrase.startTime < currentPhrase.startTime) {
              maybeCurrentPhrase = phrase;
            }
          } else {
            maybeCurrentPhrase = phrase;
          }
        }
      }
      if (maybeCurrentPhrase.has_value()) {
        auto& currentPhrase = maybeCurrentPhrase.value();
        auto frameIndex = (int)((currentTime - currentPhrase.startTime) * sampleRate);
        if (frameIndex > 0) {
          if (frameIndex >= currentPhrase.data.frames.getSize().numFrames) {
            frameIndex = currentPhrase.data.frames.getSize().numFrames - 1;
          }
          for (auto sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex) {
            auto frame = currentPhrase.data.frames.getSample(0, frameIndex + sampleIndex);
            for (auto i = 0; i < totalNumOutputChannels; ++i) buffer.getWritePointer(i)[sampleIndex] = frame;
            if (frameIndex >= currentPhrase.data.frames.getSize().numFrames) {
              break;
            }
          }
        }
      }
    }
  }
  if (willSetIsPlaying) {
    position->setIsPlaying(willSetIsPlayingValue);
    willSetIsPlaying = false;
  } else {
    auto isPlaying = position->getIsPlaying();
    if (lastIsPlaying != isPlaying) {
      juce::DynamicObject::Ptr json = new juce::DynamicObject();
      json->setProperty("type", "update:isPlaying");
      json->setProperty("isPlaying", isPlaying);
      this->sendActionMessage(juce::JSON::toString(json.get()));

      lastIsPlaying = isPlaying;
    }
  }
}

void VVVSTAudioProcessor::setIsPlaying(bool isPlaying) {
  willSetIsPlaying = true;
  willSetIsPlayingValue = isPlaying;
}

bool VVVSTAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* VVVSTAudioProcessor::createEditor() {
  auto editor = new VVVSTAudioProcessorEditor(*this);
  addActionListener(editor);
  return editor;
}

void VVVSTAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  std::unique_ptr<juce::XmlElement> xml(memory.createXml());
  if (xml.get() != nullptr) {
    copyXmlToBinary(*xml, destData);
  }
}

void VVVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName("project")) memory = juce::ValueTree::fromXml(*xmlState);
}

std::string VVVSTAudioProcessor::getProject() { return memory.getProperty("project").toString().toStdString(); }

void VVVSTAudioProcessor::setProject(std::string project) {
  memory.setProperty("project", juce::var(project), nullptr);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VVVSTAudioProcessor(); }
