cmake_minimum_required(VERSION 3.22)

project(VVVST VERSION 0.0.1)

add_subdirectory(Depends/JUCE)

if(MSVC)
  add_compile_options(/utf-8)
endif()

juce_add_plugin(Vvvst
    IS_SYNTH TRUE
    FORMATS VST3
    PRODUCT_NAME "VVVST")

file(GLOB_RECURSE SOURCES "Source/*.cpp" "Source/*.h")
juce_generate_juce_header(Vvvst)
target_sources(Vvvst
    PRIVATE
        ${SOURCES})

target_include_directories(Vvvst
    PRIVATE
    Depends)

target_compile_definitions(Vvvst
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

file(GLOB_RECURSE RESOURCES "Resources/*")

juce_add_binary_data(BinaryData
    SOURCES
        ${RESOURCES})

target_link_libraries(Vvvst
    PRIVATE
        BinaryData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
