/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <choc/gui/choc_WebView.h>
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class VVVSTAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::ActionListener
{
public:
	VVVSTAudioProcessorEditor(VVVSTAudioProcessor&);
	~VVVSTAudioProcessorEditor() override;

	//==============================================================================
	void paint(juce::Graphics&) override;
	void resized() override;

	void actionListenerCallback(const juce::String& message) override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	VVVSTAudioProcessor& audioProcessor;
	std::unique_ptr<choc::ui::WebView> chocWebView;
#if JUCE_WINDOWS
	std::unique_ptr<juce::HWNDComponent> juceView;
#elif JUCE_MAC
	std::unique_ptr<juce::NSViewComponent> juceView;
#elif JUCE_LINUX
	std::unique_ptr<juce::XEmbedComponent> juceView;
#endif

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VVVSTAudioProcessorEditor)
};
