#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ID
{
    #define PARAMETER_ID(str) constexpr const char* str { #str };

    PARAMETER_ID (frequency)
    PARAMETER_ID (gain)
    PARAMETER_ID (waveform)

    #undef PARAMETER_ID
}

class SimpleSynthAudioProcessor : public juce::AudioProcessor
{
public:
    SimpleSynthAudioProcessor();
    ~SimpleSynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    // Audio parameters
    // Audio processing state
    float phase = 0.0f;
    float currentFrequency = 440.0f;
    float sampleRate = 44100.0f;
    float envelope = 0.0f;
    bool noteOn = false;

    // Parameter management
    juce::AudioProcessorValueTreeState parameters;

    // Parameter pointers for quick access
    juce::AudioParameterFloat* frequencyParam = nullptr;
    juce::AudioParameterFloat* gainParam = nullptr;
    juce::AudioParameterChoice* waveformParam = nullptr;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void updateParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthAudioProcessor)
};
