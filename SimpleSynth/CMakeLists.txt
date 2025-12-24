cmake_minimum_required(VERSION 3.24)
project(SimpleSynth VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." juce)

# Create the plugin
juce_add_plugin(SimpleSynth
    COMPANY_NAME "YourCompanyName"
    PLUGIN_MANUFACTURER_CODE Yrcm
    PLUGIN_CODE SySy
    FORMATS VST3 Standalone
    PRODUCT_NAME "SimpleSynth"
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES
        "Instrument"
        "Synth")

target_sources(SimpleSynth PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp)

target_compile_features(SimpleSynth PRIVATE cxx_std_17)

# Add compile definitions for VST3 compatibility
target_compile_definitions(SimpleSynth PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=1
    JUCE_FORCE_USE_LEGACY_PARAM_IDS=1)

target_link_libraries(SimpleSynth PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra)
