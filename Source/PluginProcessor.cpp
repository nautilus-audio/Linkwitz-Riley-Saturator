/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <math.h>

//==============================================================================
RedRockSaturatorAudioProcessor::RedRockSaturatorAudioProcessor()
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
    numChannels = 2;
    filters.resize(numChannels);
    high_states_1.resize(numChannels);
    high_states_2.resize(numChannels);
    low_states_1.resize(numChannels);
    low_states_2.resize(numChannels);
    outputSamples.resize(numChannels);
    low_outputs.resize(numChannels);
    high_outputs.resize(numChannels);
    dist_lows.resize(numChannels);
}

RedRockSaturatorAudioProcessor::~RedRockSaturatorAudioProcessor()
{
}

//==============================================================================
const juce::String RedRockSaturatorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RedRockSaturatorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RedRockSaturatorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RedRockSaturatorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RedRockSaturatorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RedRockSaturatorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RedRockSaturatorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RedRockSaturatorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RedRockSaturatorAudioProcessor::getProgramName (int index)
{
    return {};
}

void RedRockSaturatorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void RedRockSaturatorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    fs = sampleRate;
    
    for (int i = 0; i < numChannels; i++)
    {
        filters[i].lpfLRCoeffs(f_crossover, fs);
        filters[i].hpfLRCoeffs(f_crossover, fs);
    }
}

void RedRockSaturatorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RedRockSaturatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

float RedRockSaturatorAudioProcessor::tubeSaturation(float x, float mixAmount){
    
    float a = mixAmount;
    float y = 0.f;

    // Soft clipping based on quadratic function
    float threshold1 = 1.0f/3.0f;
    float threshold2 = 2.0f/3.0f;
    
    if(a == 0.0f)
        y = x;
    else if(x > threshold2)
        y = 1.0f;
    else if(x > threshold1)
        y = (3.0f - ((2.0f - 3.0f*x)) *  ((2.0f - 3.0f*x)))/3.0f;
    else if(x < -threshold2)
        y = -1.0f;
    else if(x < -threshold1)
        y = -(3.0f - ((2.0f + 3.0f*x)) * ((2.0f + 3.0f*x)))/3.0f;
    else
        y = (2.0f* x);
    
    return y;
}

float RedRockSaturatorAudioProcessor::ProcessSample(TFloatParamType* outputs, TFloatParamType* readData, Filter channelFilter, TIntegerParamType channel, TIntegerParamType index)
{
    outputs[channel] = channelFilter.lowpass_filter(readData[index], &low_states_1[channel], &low_states_2[channel], channelFilter.lpfCoeffs.a0, channelFilter.lpfCoeffs.a1, channelFilter.lpfCoeffs.a2, channelFilter.lpfCoeffs.b1, channelFilter.lpfCoeffs.b2);
    
    return outputs[channel];
}
    


void RedRockSaturatorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
   
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    int num_samples = buffer.getNumSamples();
    
    for(int channel = 0; channel < totalNumInputChannels; channel++)
    {
        float* writeData = buffer.getWritePointer(channel);
        float* readData = const_cast<float *>(buffer.getReadPointer(channel));
        auto monoFilter = filters[channel];
        
        // Process audio samples
        for (int i = 0; i < num_samples; i++)
        {
//                lowBand[channel][i] = ProcessSample(low_outputs, readData, monoFilter, channel, i);
//
//                highBand[channel][i] = ProcessSample(high_outputs, readData, monoFilter, channel, i);
            
            low_outputs[channel] = filters[channel].lowpass_filter(readData[i], &low_states_1[channel], &low_states_2[channel], filters[channel].lpfCoeffs.a0, filters[channel].lpfCoeffs.a1, filters[channel].lpfCoeffs.a2, filters[channel].lpfCoeffs.b1, filters[channel].lpfCoeffs.b2);
            high_outputs[channel] = filters[channel].highpass_filter(readData[i], &high_states_1[channel], &high_states_2[channel], filters[channel].hpfCoeffs.a0, filters[channel].hpfCoeffs.a1, filters[channel].hpfCoeffs.a2, filters[channel].hpfCoeffs.b1, filters[channel].hpfCoeffs.b2);
            
            dist_lows[channel] = tubeSaturation(low_outputs[channel], 1.f); // Apply Saturation
            
            outputSamples[channel] = dist_lows[channel] + high_outputs[channel];  // Sum Signals
            
            writeData[i] = outputSamples[channel];   // Copy to Buffer
            
        }
    }
}

//==============================================================================
bool RedRockSaturatorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RedRockSaturatorAudioProcessor::createEditor()
{
    return new RedRockSaturatorTestAudioProcessorEditor (*this);
}

//==============================================================================
void RedRockSaturatorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void RedRockSaturatorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RedRockSaturatorAudioProcessor();
}
