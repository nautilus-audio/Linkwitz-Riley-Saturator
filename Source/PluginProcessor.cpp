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
    int numChannels = 2;
    high_states_1.resize(numChannels);
    high_states_2.resize(numChannels);
    low_states_1.resize(numChannels);
    low_states_2.resize(numChannels);
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
    lpfLRCoeffs(f_crossover, fs);
    hpfLRCoeffs(f_crossover, fs);
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


void RedRockSaturatorAudioProcessor::hpfLRCoeffs(float f_crossover, float fs)
{
    double w0 = 2 * M_PI * f_crossover / fs;
    double K = tan(w0 / 2);

    double norm = 1 / (1 + K * sqrt(2) + K * K);

    hpfCoeffs.a0 = norm * K * K;
    hpfCoeffs.a1 = -2 * hpfCoeffs.a0;
    hpfCoeffs.a2 = hpfCoeffs.a0;
    hpfCoeffs.b1 = norm * 2 * (K * K - 1);
    hpfCoeffs.b2 = norm * (1 - K * sqrt(2) + K * K);

}

void RedRockSaturatorAudioProcessor::lpfLRCoeffs(float f_crossover, float fs)
{
    float theta = M_PI * f_crossover / fs;
    float Wc = M_PI * f_crossover;
    float k = Wc / tan(theta);

    float d = pow(k, 2.0) + pow(Wc, 2.0) + 2.0 * k * Wc;
    lpfCoeffs.a0 = pow(Wc, 2.0) / d;
    lpfCoeffs.a1 = 2.0 * pow(Wc, 2.0) / d;
    lpfCoeffs.a2 = lpfCoeffs.a0;
    lpfCoeffs.b1 = (-2.0 * pow(k, 2.0) + 2.0 * pow(Wc, 2.0)) / d;
    lpfCoeffs.b2 = (-2.0 * k * Wc + pow(k, 2.0) + pow(Wc, 2.0)) / d;

}


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



float RedRockSaturatorAudioProcessor::lowpass_filter_L(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
    float output = a0 * input + a1 * (*state1) + a2 * (*state2);
    *state2 = *state1;
    *state1 = input - b1 * (*state1) - b2 * (*state2);
    return output;
}

float RedRockSaturatorAudioProcessor::lowpass_filter_R(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
    float output = a0 * input + a1 * (*state1) + a2 * (*state2);
    *state2 = *state1;
    *state1 = input - b1 * (*state1) - b2 * (*state2);
    return output;
}

float RedRockSaturatorAudioProcessor::highpass_filter_L(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
    
    float output = a0 * input + a1 * (*state1) + a2 * (*state2);
    *state2 = *state1;
    *state1 = input - b1 * (*state1) - b2 * (*state2);
    return output * (-1);
}

float RedRockSaturatorAudioProcessor::highpass_filter_R(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
    
    float output = a0 * input + a1 * (*state1) + a2 * (*state2);
    *state2 = *state1;
    *state1 = input - b1 * (*state1) - b2 * (*state2);
    return output * (-1);
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

    float* channelDataL = buffer.getWritePointer(0);
    float* channelDataR = buffer.getWritePointer(1);

    auto readDataL = buffer.getReadPointer(0);
    auto readDataR = buffer.getReadPointer(1);

    
//    for(int channel = 0; channel < totalNumInputChannels; channel++)
//    {
        // Process audio samples
        for (int i = 0; i < num_samples; i++)
        {
            // Left channel
            low_output_L = lowpass_filter_L(readDataL[i], low_state1_L, low_state2_L, lpfCoeffs.a0, lpfCoeffs.a1, lpfCoeffs.a2, lpfCoeffs.b1, lpfCoeffs.b2);
            high_output_L = highpass_filter_L(readDataL[i], high_state1_L, high_state2_L, hpfCoeffs.a0, hpfCoeffs.a1, hpfCoeffs.a2, hpfCoeffs.b1, hpfCoeffs.b2);
            
            dist_low_L = tubeSaturation(low_output_L, 1.f); // Apply Saturation
            outputSample_L = dist_low_L + high_output_L;    // Sum Signals
            channelDataL[i] = outputSample_L;   // Copy to Buffer
            
            // Right channel
            low_output_R = lowpass_filter_R(readDataR[i], low_state1_R, low_state2_R, lpfCoeffs.a0, lpfCoeffs.a1, lpfCoeffs.a2, lpfCoeffs.b1, lpfCoeffs.b2);
            high_output_R = highpass_filter_R(readDataR[i], high_state1_R, high_state2_R, hpfCoeffs.a0, hpfCoeffs.a1, hpfCoeffs.a2, hpfCoeffs.b1, hpfCoeffs.b2);
            
            dist_low_R = tubeSaturation(low_output_R, 1.f); // Apply Saturation
            outputSample_R = dist_low_R + high_output_R;    // Sum Signals
            channelDataR[i] = outputSample_R;   // Copy to Buffer
        }
//    }
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
