/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VVVSTAudioProcessor::VVVSTAudioProcessor()
	: AudioProcessor(BusesProperties()
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
	)
{
	memory = "";
}

VVVSTAudioProcessor::~VVVSTAudioProcessor()
{
}

//==============================================================================
const juce::String VVVSTAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool VVVSTAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool VVVSTAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool VVVSTAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double VVVSTAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int VVVSTAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
	// so this should be at least 1, even if you're not really implementing programs.
}

int VVVSTAudioProcessor::getCurrentProgram()
{
	return 0;
}

void VVVSTAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String VVVSTAudioProcessor::getProgramName(int index)
{
	return {};
}

void VVVSTAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void VVVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	this->sampleRate = sampleRate;
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
}

void VVVSTAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VVVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void VVVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();



	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

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
					}
					else {
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
						for (auto i = 0; i < totalNumOutputChannels; ++i)
							buffer.getWritePointer(i)[sampleIndex] = frame;
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
		// lastIsPlaying = willSetIsPlayingValue.load();
	}
	else {
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

//==============================================================================
bool VVVSTAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VVVSTAudioProcessor::createEditor()
{
	auto editor = new VVVSTAudioProcessorEditor(*this);
	addActionListener(editor);
	return editor;
}

//==============================================================================
void VVVSTAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
	destData.ensureSize(memory.size());
	std::copy(memory.begin(), memory.end(), destData.begin());
}

void VVVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
	std::vector<char> buffer = std::vector<char>((char*)data, (char*)data + sizeInBytes);
	memory = std::string(buffer.begin(), buffer.end());
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new VVVSTAudioProcessor();
}
