cmake_minimum_required(VERSION 3.22)

project(VVVST VERSION 0.0.1)

add_subdirectory(Depends/JUCE)

if(MSVC)
  add_compile_options(/utf-8)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(PLUGIN_NAME "VVVST")
  set(BUNDLE_ID "com.sevenc-nanashi.vvvst")
  set(PLUGIN_CODE "Vvst")
  # set(BUNDLE_ID "com.hiroshiba.vvvst")
else()
  set(PLUGIN_NAME "VVVST (Debug)")
  set(BUNDLE_ID "com.sevenc-nanashi.vvvst-debug")
  set(PLUGIN_CODE "Vvsd")
  # set(BUNDLE_ID "com.hiroshiba.vvvst-debug")
endif()

set(OUTPUT_NAME "VVVST")
file(READ version.txt VERSION)
string(STRIP ${VERSION} VERSION)

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
    PLUGIN_MANUFACTURER_CODE Scns
    # PLUGIN_MANUFACTURER_CODE Hrkz
    PLUGIN_CODE ${PLUGIN_CODE}
  BUNDLE_ID ${BUNDLE_ID}
  PLUGIN_NAME ${PLUGIN_NAME})

file(GLOB_RECURSE SOURCES "Source/*.cpp" "Source/*.h")
file(GLOB_RECURSE SOURCES_MIMETYPES "Depends/MimeTypes/*.cpp"
     "Depends/MimeTypes/*.h")

juce_generate_juce_header(Vvvst)
target_sources(Vvvst PRIVATE ${SOURCES} ${SOURCES_MIMETYPES})

target_include_directories(Vvvst PRIVATE Depends/ Depends/gzip-hpp/include)

if(UNIX AND NOT APPLE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLIB REQUIRED glib-2.0)
  pkg_check_modules(GTK REQUIRED gtk+-3.0)
  pkg_check_modules(SOUP REQUIRED libsoup-3.0)
  pkg_check_modules(JSCOREGTK REQUIRED javascriptcoregtk-4.0)
  pkg_check_modules(WEBKITGTK REQUIRED webkit2gtk-4.0)
  target_link_libraries(
    Vvvst PRIVATE ${GLIB_LIBRARIES} ${JSCOREGTK_LIBRARIES} ${SOUP_LIBRARIES}
                  ${GTK_LIBRARIES} ${WEBKITGTK_LIBRARIES})
  target_include_directories(
    Vvvst PRIVATE ${GLIB_INCLUDE_DIRS} ${GTK_INCLUDE_DIRS} ${SOUP_INCLUDE_DIRS}
                  ${JSCOREGTK_INCLUDE_DIRS} ${WEBKITGTK_INCLUDE_DIRS})
endif()

target_compile_definitions(
  Vvvst PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
               VVVST_VERSION="${VERSION}")

file(GLOB_RECURSE RESOURCES "Resources/*")

juce_add_binary_data(BinaryData SOURCES ${RESOURCES})

target_link_libraries(
  Vvvst
  PRIVATE BinaryData juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)
