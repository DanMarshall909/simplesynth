cmake_minimum_required(VERSION 3.24)
project(SimpleSynthHost VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." juce)

# Create simple executable
add_executable(SimpleSynthHost Source/Main.cpp)

# Include JUCE module headers
target_include_directories(SimpleSynthHost PRIVATE
    ${JUCE_MODULES_DIR})

# Link JUCE libraries (must come AFTER include_directories for proper module setup)
target_link_libraries(SimpleSynthHost PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_processors)

# Compile definitions for VST3 hosting
target_compile_definitions(SimpleSynthHost PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_AU=0
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)

target_compile_features(SimpleSynthHost PRIVATE cxx_std_17)
