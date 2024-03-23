#include "PluginEditor.h"

#include <BinaryData.h>
#include <MimeTypes/MimeTypes.h>
#include <choc/gui/choc_WebView.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "PluginProcessor.h"

// ValueView.getFloat64()はInt64でエラーが起きるので、それを回避するための関数
double toFloat64(const choc::value::ValueView& value) {
  if (value.isInt64()) {
    return static_cast<double>(value.getInt64());
  } else {
    return value.getFloat64();
  }
}

std::string generateNonce() {
  std::random_device rnd;
  std::mt19937 mt(rnd());
  std::uniform_int_distribution<> rand(0, 15);
  std::string ret;
  for (int i = 0; i < 16; i++) {
    int r = rand(mt);
    if (i == 6) {
      ret += "-";
    } else if (i == 8) {
      ret += "-";
    } else if (i == 10) {
      ret += "-";
    } else if (i == 12) {
      ret += "-";
    }
    ret += "0123456789abcdef"[r];
  }
  return ret;
}

#ifdef JUCE_WINDOWS
std::string safeGetenv(const char* name) {
  char* value;
  size_t len;
  errno_t err = _dupenv_s(&value, &len, name);
  if (err != 0) {
    return "";
  }
  std::string res(value);
  free(value);
  return res;
}

#include <Windows.h>

// UTF-8のvector<uint8_t>をstd::stringに変換する関数
std::string utf8ToString(const std::vector<uint8_t>& utf8) {
  int bufferSize = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(utf8.data()), utf8.size(), nullptr, 0);
  std::vector<wchar_t> buffer(bufferSize);
  MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(utf8.data()), utf8.size(), buffer.data(),
                      buffer.size());
  int size = WideCharToMultiByte(CP_ACP, 0, buffer.data(), buffer.size(), nullptr, 0, nullptr, nullptr);
  std::vector<char> result(size);
  WideCharToMultiByte(CP_ACP, 0, buffer.data(), buffer.size(), result.data(), result.size(), nullptr, nullptr);
  return std::string(result.begin(), result.end());
}
#else
std::string safeGetenv(const char* name) {
  auto value = std::getenv(name);
  if (value == nullptr) {
    return "";
  }
  return std::string(value);
}
// Windows以外では多分全部utf-8なのでそのままstd::stringに変換する
std::string utf8ToString(const std::vector<uint8_t>& utf8) { return std::string(utf8.begin(), utf8.end()); }
#endif

//==============================================================================
VVVSTAudioProcessorEditor::VVVSTAudioProcessorEditor(VVVSTAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p)
#ifndef JUCE_DEBUG
      ,
      stream(BinaryData::voicevox_zip, BinaryData::voicevox_zipSize, false),
      zipFile(stream)
#endif
{
  choc::ui::WebView::Options options;
#ifdef JUCE_DEBUG
  options.enableDebugMode = true;
#else
  options.enableDebugMode = false;

  // WebViewの実行時オプションの１つとして、WebView側の`fetch`要求に対応するコールバック関数を実装する
  options.fetchResource =
      [this](const choc::ui::WebView::Options::Path& path) -> std::optional<choc::ui::WebView::Options::Resource> {
    std::string filePath = (path == "/" ? "index.html" : path.substr(1));

    auto entry = this->zipFile.getEntry(filePath);
    if (!entry) {
      return std::nullopt;
    }

    auto fileData = this->zipFile.createStreamForEntry(*entry);
    if (!fileData) {
      return std::nullopt;
    }

    juce::MemoryBlock block;
    fileData->readIntoMemoryBlock(block);

    auto mimeType = MimeTypes::getType(filePath.c_str());
    std::string mimeTypeStr;
    if (mimeType == nullptr) {
      mimeTypeStr = "application/octet-stream";
    } else {
      mimeTypeStr = mimeType;
    }

    return choc::ui::WebView::Options::Resource{std::vector<uint8_t>(block.begin(), block.end()), mimeTypeStr};
  };
#endif

  chocWebView = std::make_unique<choc::ui::WebView>(options);
  if (chocWebView == nullptr) {
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

  chocWebView->bind(
      "vstGetProject",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        std::string project = safe_this->audioProcessor.getProject();

        return choc::value::createString(project);
      });

  chocWebView->bind(
      "vstSetProject",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        const auto project = args[0].getString();
        safe_this->audioProcessor.setProject(std::string(project));

        return choc::value::Value(0);
      });

  chocWebView->bind(
      "vstGetConfig",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
#ifdef JUCE_WINDOWS
        // Windowsの場合は%APPDATA%/voicevox/config.jsonを読み込む
        auto appData = safeGetenv("APPDATA");
        auto path = appData + "\\voicevox\\config.json";
#elif JUCE_MAC
        // Macの場合は$HOMEディレクトリのLibrary/Application Support/voicevox/config.jsonを読み込む
        auto home = safeGetenv("HOME");
        auto path = home + "/Library/Application Support/voicevox/config.json";
#elif JUCE_LINUX
        // Linuxの場合は$XDG_CONFIG_HOME/voicevox/config.json、または$HOME/.config/voicevox/config.jsonを読み込む
        // TODO：テストする

        auto home = safeGetenv("XDG_CONFIG_HOME");
        if (home.empty()) {
          home = safeGetenv("HOME") + "/.config";
        }
        auto path = home + "/voicevox/config.json";
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
  chocWebView->bind(
      "vstGetProjectName",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        return choc::value::createString(ProjectInfo::projectName);
      });
  chocWebView->bind(
      "vstGetVersion",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        return choc::value::createString(VVVST_VERSION);
      });

  chocWebView->bind(
      "vstClearPhrases",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        safe_this->audioProcessor.phrases.clear();

        return choc::value::Value(0);
      });
  chocWebView->bind(
      "vstGetPhrases",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        choc::value::Value res = choc::value::createObject("");
        for (auto& [id, phrase] : safe_this->audioProcessor.phrases) {
          auto start = phrase.startTime;
          auto end = phrase.endTime;
          auto hash = phrase.hash;
          res.addMember(id, choc::json::create("start", start, "end", end, "hash", hash));
        }

        return res;
      });
  chocWebView->bind(
      "vstShowImportFileDialog",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        std::string title(args[0].getString());
        std::string extension;
        if (args[1].isArray()) {
          for (const auto& ext : args[1]) {
            extension += "*." + std::string(ext.getString()) + ";";
          }
          extension.pop_back();
        } else {
          extension = "*";
        }
        // juce::FileChooser chooser(title, juce::File(), extension);
        auto chooser = std::make_unique<juce::FileChooser>(title, juce::File(), extension);
        auto nonce = generateNonce();
        chooser->launchAsync(juce::FileBrowserComponent::openMode, [safe_this,
                                                                    nonce](const juce::FileChooser& chooser) {
          auto result = chooser.getResult();
          if (result == juce::File()) {
            safe_this->chocWebView->evaluateJavascript("window.vstOnFileChosen('" + nonce + "', undefined)");
          } else {
            auto quotedString = choc::json::toString(choc::value::createString(result.getFullPathName().toStdString()));
            safe_this->chocWebView->evaluateJavascript("window.vstOnFileChosen('" + nonce + "', " + quotedString + ")");
          }
          safe_this->fileChoosers.erase(nonce);
        });
        safe_this->fileChoosers.insert_or_assign(nonce, std::move(chooser));
        return choc::value::createString(nonce);
      });
  chocWebView->bind(
      "vstReadFile",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        std::string pathBase64(args[0].getString());
        std::vector<uint8_t> pathBuffer;
        choc::base64::decodeToContainer(pathBuffer, pathBase64);
        std::string path = utf8ToString(pathBuffer);

        std::ifstream ifs(path);

        if (ifs.fail()) {
          DBG("Failed to open file");
          return choc::value::Value(-1);
        }
        std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        std::string data = choc::base64::encodeToString(buffer);
        return choc::value::createString(data);
      });
  chocWebView->bind(
      "vstExportProject",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        auto chooser = std::make_unique<juce::FileChooser>(juce::String::fromUTF8("プロジェクトファイルの書き出し"),
                                                           juce::File(), "*.vvproj");
        auto nonce = generateNonce();
        chooser->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
            [safe_this, nonce](const juce::FileChooser& chooser) {
              auto result = chooser.getResult();
              if (result == juce::File()) {
                safe_this->chocWebView->evaluateJavascript("window.vstOnExportProjectFinished('" + nonce +
                                                           "', 'cancelled')");
                return;
              }
              auto project = safe_this->audioProcessor.getProject();
              if (result.replaceWithData(project.c_str(), project.size())) {
                safe_this->chocWebView->evaluateJavascript("window.vstOnExportProjectFinished('" + nonce + "', 'ok')");
              } else {
                safe_this->chocWebView->evaluateJavascript("window.vstOnExportProjectFinished('" + nonce +
                                                           "', 'error')");
              }

              safe_this->fileChoosers.erase(nonce);
            });
        safe_this->fileChoosers.insert_or_assign(nonce, std::move(chooser));
        return choc::value::createString(nonce);
      });
  chocWebView->bind(
      "vstUpdatePhrases",
      [safe_this = juce::Component::SafePointer(this)](const choc::value::ValueView& args) -> choc::value::Value {
        choc::value::Value removedPhrases(args[0]);
        choc::value::Value changedPhrases(args[1]);

        std::thread([safe_this, removedPhrases, changedPhrases]() {
          juce::ScopedLock lock(safe_this->audioProcessor.phrasesLock);
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

            auto data = reader->loadFileContent(safe_this->audioProcessor.sampleRate);

            auto id = std::string(phraseVal["id"].getString());
            auto hash = std::string(phraseVal["hash"].getString());
            Phrase phrase(id, toFloat64(phraseVal["start"]), toFloat64(phraseVal["end"]), data, hash);
            safe_this->audioProcessor.phrases.insert_or_assign(id, phrase);
          }
        }).detach();

        return choc::value::Value(0);
      });

#ifdef JUCE_DEBUG
  chocWebView->navigate("http://localhost:5173");
#endif

  addAndMakeVisible(juceView.get());
  setResizable(true, true);
  setSize(960, 640);
}

VVVSTAudioProcessorEditor::~VVVSTAudioProcessorEditor() { this->audioProcessor.removeActionListener(this); }

void VVVSTAudioProcessorEditor::paint(juce::Graphics& g) {}

void VVVSTAudioProcessorEditor::resized() { juceView->setBounds(getLocalBounds()); }

void VVVSTAudioProcessorEditor::actionListenerCallback(const juce::String& message) {
  chocWebView->evaluateJavascript("window.vstOnMessage(" + message.toStdString() + ")");
}
