#include "PluginProcessor.h"
#include "PluginEditor.h"

SimpleSynthAudioProcessor::SimpleSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("SimpleSynthParameters"), createParameterLayout())
{
    // Retrieve parameter pointers for quick access using namespace IDs
    frequencyParam = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter(ID::frequency));
    gainParam = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter(ID::gain));
    waveformParam = dynamic_cast<juce::AudioParameterChoice*>(parameters.getParameter(ID::waveform));
}

SimpleSynthAudioProcessor::~SimpleSynthAudioProcessor()
{
}

void SimpleSynthAudioProcessor::updateParameters()
{
    // Update audio processing state based on parameters
    if (frequencyParam)
        currentFrequency = frequencyParam->get();
}

void SimpleSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    this->sampleRate = (float)sampleRate;
    phase = 0.0f;
}

void SimpleSynthAudioProcessor::releaseResources()
{
}

void SimpleSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Update parameter state
    updateParameters();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Process MIDI
    for (auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            currentFrequency = juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
            noteOn = true;
            envelope = 0.0f;
        }
        else if (msg.isNoteOff())
        {
            noteOn = false;
        }
    }

    // Generate audio
    auto* channelData = buffer.getWritePointer(0);
    float gain = gainParam ? gainParam->get() : 0.3f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Simple envelope (attack and release)
        if (noteOn)
        {
            envelope = juce::jmin(envelope + 0.01f, 1.0f); // Attack
        }
        else
        {
            envelope = juce::jmax(envelope - 0.02f, 0.0f); // Release
        }

        // Waveform selection
        float output = 0.0f;
        int waveform = waveformParam ? waveformParam->getIndex() : 0;

        // Generate selected waveform
        float phaseIncrement = currentFrequency / sampleRate;
        phase += phaseIncrement;
        if (phase > 1.0f) phase -= 1.0f;

        switch (waveform)
        {
            case 0: // Sine
                output = std::sin(phase * juce::MathConstants<float>::twoPi);
                break;
            case 1: // Square
                output = phase < 0.5f ? 1.0f : -1.0f;
                break;
            case 2: // Sawtooth
                output = 2.0f * phase - 1.0f;
                break;
            case 3: // Triangle
                output = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
                break;
        }

        output *= envelope * gain;
        channelData[sample] = output;
    }

    // Copy to stereo
    if (getTotalNumOutputChannels() > 1)
    {
        auto* rightChannel = buffer.getWritePointer(1);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            rightChannel[sample] = channelData[sample];
        }
    }
}

juce::AudioProcessorEditor* SimpleSynthAudioProcessor::createEditor()
{
    return new SimpleSynthAudioProcessorEditor(*this);
}

bool SimpleSynthAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String SimpleSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleSynthAudioProcessor::acceptsMidi() const
{
    return true;
}

bool SimpleSynthAudioProcessor::producesMidi() const
{
    return false;
}

bool SimpleSynthAudioProcessor::isMidiEffect() const
{
    return false;
}

double SimpleSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int SimpleSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleSynthAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SimpleSynthAudioProcessor::getProgramName(int index)
{
    return {};
}

void SimpleSynthAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void SimpleSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = parameters.state.createXml();
    if (xml)
    {
        juce::String xmlString = xml->toString();
        destData.append(xmlString.toUTF8(), xmlString.getNumBytesAsUTF8());
    }
}

void SimpleSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data && sizeInBytes > 0)
    {
        parameters.replaceState(juce::ValueTree::fromXml(juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes)));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add parameters with explicit IDs from namespace
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ID::frequency, 1),
        "Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        440.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ID::gain, 1),
        "Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.7f
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ID::waveform, 1),
        "Waveform",
        juce::StringArray{"Sine", "Square", "Sawtooth", "Triangle"},
        0
    ));

    return layout;
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleSynthAudioProcessor();
}
