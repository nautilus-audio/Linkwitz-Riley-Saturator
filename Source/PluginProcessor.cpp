/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RRS_Header_integrationAudioProcessor::RRS_Header_integrationAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

RRS_Header_integrationAudioProcessor::~RRS_Header_integrationAudioProcessor()
{
}

//==============================================================================
const juce::String RRS_Header_integrationAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RRS_Header_integrationAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RRS_Header_integrationAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RRS_Header_integrationAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RRS_Header_integrationAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RRS_Header_integrationAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RRS_Header_integrationAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RRS_Header_integrationAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RRS_Header_integrationAudioProcessor::getProgramName (int index)
{
    return {};
}

void RRS_Header_integrationAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void RRS_Header_integrationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    saturator.Init();
    saturator.SetMaxChannels(2);
    saturator.SetMaxBlockSize(samplesPerBlock);
    saturator.SetCrossoverFrequency(5000.f);
    saturator.SetSampleRate(sampleRate);
    saturator.SetGain(1.2f);
}

void RRS_Header_integrationAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    saturator.Release();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RRS_Header_integrationAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RRS_Header_integrationAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    auto writeData = const_cast<TFloatParamType **>(buffer.getArrayOfWritePointers());
    saturator.Process(writeData, totalNumInputChannels, numSamples);
    
//    auto writeData = const_cast<TAudioSampleType **>(buffer.getArrayOfWritePointers());
//    auto readData = const_cast<TAudioSampleType **>(buffer.getArrayOfReadPointers());
//
//    memcpy(writeData, readData, numSamples);
//    
//    saturator.Process(writeData, totalNumInputChannels, numSamples);

}

//==============================================================================
bool RRS_Header_integrationAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RRS_Header_integrationAudioProcessor::createEditor()
{
    return new RRS_Header_integrationAudioProcessorEditor (*this);
}

//==============================================================================
void RRS_Header_integrationAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void RRS_Header_integrationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RRS_Header_integrationAudioProcessor();
}
