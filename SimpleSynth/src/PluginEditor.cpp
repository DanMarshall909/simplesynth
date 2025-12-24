#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleSynthAudioProcessorEditor::SimpleSynthAudioProcessorEditor(SimpleSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 300);
}

SimpleSynthAudioProcessorEditor::~SimpleSynthAudioProcessorEditor()
{
}

void SimpleSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("SimpleSynth", getLocalBounds(), juce::Justification::centred, true);

    g.setFont(14.0f);
    g.drawText("Send MIDI notes to play", getLocalBounds().removeFromBottom(100), juce::Justification::centred, true);
}

void SimpleSynthAudioProcessorEditor::resized()
{
}
