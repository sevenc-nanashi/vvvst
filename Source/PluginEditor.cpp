/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <choc/gui/choc_WebView.h>
#include <BinaryData.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

// ValueView.getFloat64()はInt64でエラーが起きるので、それを回避するための関数
double toFloat64(const choc::value::ValueView& value)
{
	if (value.isInt64()) {
		return static_cast<double>(value.getInt64());
	}
	else {
		return value.getFloat64();
	}
}

std::string getMimeType(std::string const& ext)
{
	static std::unordered_map<std::string, std::string> mimeTypes{
	  { ".html",   "text/html" },
	  { ".js",     "application/javascript" },
	  { ".css",    "text/css" },
	  { ".json",   "application/json"},
	  { ".svg",    "image/svg+xml"},
	  { ".svgz",   "image/svg+xml"},
	};

	if (mimeTypes.count(ext) > 0)
	{
		return mimeTypes.at(ext);
	}

	return "application/octet-stream";
}

//==============================================================================
VVVSTAudioProcessorEditor::VVVSTAudioProcessorEditor(VVVSTAudioProcessor& p)
	: AudioProcessorEditor(&p)
	, audioProcessor(p)
#ifndef JUCE_DEBUG
	, stream(BinaryData::voicevox_zip,
		BinaryData::voicevox_zipSize,
		false)
	, zipFile(stream)
#endif
{
	choc::ui::WebView::Options options;
#ifdef JUCE_DEBUG
	options.enableDebugMode = true;
#else
	// ここら辺はまだ元コードをコピペしただけ、動くかどうかは不明
	options.enableDebugMode = false;

	// WebViewの実行時オプションの１つとして、WebView側の`fetch`要求に対応するコールバック関数を実装する
	options.fetchResource = [this](const choc::ui::WebView::Options::Path& path)
		-> std::optional<choc::ui::WebView::Options::Resource> {
		std::string filePath = (path == "/" ? "index.html" : path.substr(1));
		auto extension = filePath.substr(filePath.find_last_of('.'));

		auto entry = this->zipFile.getEntry(filePath);
		if (!entry)
		{
			return std::nullopt;
		}

		auto fileData = this->zipFile.createStreamForEntry(*entry);
		if (!fileData)
		{
			return std::nullopt;
		}

		juce::MemoryBlock block;
		fileData->readIntoMemoryBlock(block);

		return choc::ui::WebView::Options::Resource{
			std::vector<uint8_t>(block.begin(), block.end()),
			getMimeType(extension)
		};

		};
#endif

	// CHOCのAPIからWebViewオブジェクトを生成する.引数には実行時オプションを渡す
	chocWebView = std::make_unique<choc::ui::WebView>(options);
	if (chocWebView == nullptr)
	{
		juce::Logger::writeToLog("Failed to create WebView");
		return;
	}
#if JUCE_WINDOWS
	juceView = std::make_unique<juce::HWNDComponent>();
	juceView->setHWND(chocWebView->getViewHandle());
#elif JUCE_MAC
	juceView = std::make_unique<juce::NSViewComponent>();
	juceView->setView(chocWebView->getViewHandle());
#elif JUCE_LINUX
	juceView = std::make_unique<juce::XEmbedComponent>(chocWebView->getViewHandle());
#endif

	chocWebView->bind("vstGetProject",
		[safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args)
		-> choc::value::Value {
			std::string& project = safe_this->audioProcessor.getProject();

			return choc::value::createString(project);
		});

	chocWebView->bind("vstSetProject",
		[safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args)
		-> choc::value::Value {
			const auto project = args[0].getString();
			safe_this->audioProcessor.setProject(std::string(project));

			return choc::value::Value(0);
		});

	chocWebView->bind("vstGetConfig",
		[safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args)
		-> choc::value::Value {
#ifdef JUCE_WINDOWS
			// Windowsの場合は%APPDATA%/voicevox/config.jsonを読み込む
			auto appData = std::getenv("APPDATA");
			auto path = std::string(appData) + "\\voicevox\\config.json";
#elif JUCE_MAC
			// Macの場合は$HOMEディレクトリのLibrary/Application Support/voicevox/config.jsonを読み込む			
			auto home = std::getenv("HOME");
			auto path = std::string(home) + "/Library/Application Support/voicevox/config.json";
#else
#error "Not implemented"
#endif
			std::ifstream ifs(path);
			std::string res;
			std::string str;

			if (ifs.fail()) {
				DBG("Failed to open file");
				return choc::value::Value(-1);
			}
			while (getline(ifs, str)) {
				res += str;
			}
			return choc::value::createString(res);
		});

	chocWebView->bind("vstClearPhrases",
		[safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args)
		-> choc::value::Value {
			safe_this->audioProcessor.phrases.clear();

			return choc::value::Value(0);
		});
	chocWebView->bind("vstUpdatePhrases",
		[safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args)
		-> choc::value::Value {
			const auto removedPhrases = args[0];
			const auto changedPhrases = args[1];

			choc::audio::WAVAudioFileFormat<false> wavFileFormat;

			for (const auto& phraseId : removedPhrases) {
				auto id = std::string(phraseId.getString());
				safe_this->audioProcessor.phrases.erase(id);
			}

			for (const auto& phraseVal : changedPhrases) {
				auto wavB64 = phraseVal["wav"].getString();
				std::vector<uint8_t> wavBuffer;
				choc::base64::decodeToContainer(wavBuffer, wavB64);
				auto wavStream = new std::istringstream(std::string(wavBuffer.begin(), wavBuffer.end()));
				auto wavStreamPtr = std::shared_ptr<std::istream>(wavStream);
				auto reader = wavFileFormat.createReader(wavStreamPtr);

				if (reader == nullptr) {
					DBG("Failed to create reader");
					continue;
				}

				auto data = reader->loadFileContent(
					safe_this->audioProcessor.sampleRate
				);


				auto id = std::string(phraseVal["id"].getString());
				Phrase phrase(
					id,
					toFloat64(phraseVal["start"]),
					toFloat64(phraseVal["end"]),
					data
				);
				safe_this->audioProcessor.phrases.insert_or_assign(id, phrase);
			}



			return choc::value::Value(0);
		});

#ifdef JUCE_DEBUG
	chocWebView->navigate("http://localhost:5173");
#endif

	addAndMakeVisible(juceView.get());
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	setResizable(true, true);
	setSize(960, 640);
}

VVVSTAudioProcessorEditor::~VVVSTAudioProcessorEditor()
{
	this->audioProcessor.removeActionListener(this);
}

//==============================================================================
void VVVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	// g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void VVVSTAudioProcessorEditor::resized()
{
	auto rect_ui = getLocalBounds();

	// WebViewの配置（座標とサイズの適用）を実行する。
	juceView->setBounds(getLocalBounds());
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..
}

void VVVSTAudioProcessorEditor::actionListenerCallback(const juce::String& message)
{
	chocWebView->evaluateJavascript("window.vstOnMessage(" + message.toStdString() + ")");
}
