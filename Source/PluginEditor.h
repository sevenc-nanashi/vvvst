
#pragma once

#include <JuceHeader.h>
#include <choc/gui/choc_WebView.h>
#include <shared_mutex>
#include <unordered_map>

#include "PluginProcessor.h"

class VVVSTAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::ActionListener {
 public:
  VVVSTAudioProcessorEditor(VVVSTAudioProcessor&);
  ~VVVSTAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

  void actionListenerCallback(const juce::String& message) override;

 private:
  VVVSTAudioProcessor& audioProcessor;
  std::unordered_map<std::string, std::unique_ptr<juce::FileChooser>> fileChoosers;

#ifndef JUCE_DEBUG
  juce::MemoryInputStream stream;
  juce::ZipFile zipFile;
#endif

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
