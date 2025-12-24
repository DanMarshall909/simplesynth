#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class SimpleSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    SimpleSynthAudioProcessorEditor(SimpleSynthAudioProcessor&);
    ~SimpleSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SimpleSynthAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleSynthAudioProcessorEditor)
};
