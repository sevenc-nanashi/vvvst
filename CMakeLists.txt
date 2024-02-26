cmake_minimum_required(VERSION 3.22)

project(VVVST VERSION 0.0.1)

add_subdirectory(Depends/JUCE)
add_subdirectory(Depends/zlib)

if(MSVC)
  add_compile_options(/utf-8)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(PRODUCT_NAME "VVVST")
  set(BUNDLE_ID "com.sevenc-nanashi.vvvst")
  # set(BUNDLE_ID "com.hiroshiba.vvvst")
else()
  set(PRODUCT_NAME "VVVST (Debug)")
  set(BUNDLE_ID "com.sevenc-nanashi.vvvst-debug")
  # set(BUNDLE_ID "com.hiroshiba.vvvst-debug")
endif()

juce_add_plugin(
  Vvvst
  IS_SYNTH TRUE
  FORMATS VST3
  COMPANY_NAME "Nanashi. <https://sevenc7c.com>"
  COMPANY_WEBSITE "https://github.com/sevenc-nanashi/VVVST"
  COMPANY_COPYRIGHT
    "(c) 2024 Nanashi."
    # COMPANY_NAME "Voicevox" COMPANY_WEBSITE "https://voicevox.hiroshiba.jp/"
    # COMPANY_COPYRIGHT "(c) 2024 Hiroshiba Kazuyuki"
  BUNDLE_ID ${BUNDLE_ID}
  PRODUCT_NAME ${PRODUCT_NAME})

file(GLOB_RECURSE SOURCES "Source/*.cpp" "Source/*.h")
file(GLOB_RECURSE SOURCES_MIMETYPES "Depends/MimeTypes/*.cpp"
     "Depends/MimeTypes/*.h")

juce_generate_juce_header(Vvvst)
target_sources(Vvvst PRIVATE ${SOURCES} ${SOURCES_MIMETYPES})

target_include_directories(Vvvst PRIVATE Depends/ Depends/gzip-hpp/include)

target_compile_definitions(Vvvst PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
                                        JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Vvvst PRIVATE zlibstatic)

file(GLOB_RECURSE RESOURCES "Resources/*")

juce_add_binary_data(BinaryData SOURCES ${RESOURCES})

target_link_libraries(
  Vvvst
  PRIVATE BinaryData juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
