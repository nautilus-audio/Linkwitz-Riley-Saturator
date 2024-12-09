/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/

using namespace juce;

typedef float TAudioSampleType;
typedef TAudioSampleType TFloatParamType;
typedef int TIntegerParamType;

class Filter{
    
public:
    
    void hpfLRCoeffs(float f_crossover, float fs)
    {
        float theta = M_PI * f_crossover / fs;
        float Wc = M_PI * f_crossover;
        float k = Wc / tan(theta);
        float d = pow(k, 2.0) + pow(Wc, 2.0) + 2.0 * k * Wc;
        
        hpfCoeffs.a0 = pow(Wc, 2.0) / d;
        hpfCoeffs.a1 = -2.0 * pow(Wc, 2.0) / d;
        hpfCoeffs.a2 = lpfCoeffs.a0;
        hpfCoeffs.b1 = (-2.0 * pow(k, 2.0) + 2.0 * pow(Wc, 2.0)) / d;
        hpfCoeffs.b2 = (-2.0 * k * Wc + pow(k, 2.0) + pow(Wc, 2.0)) / d;

    }

    void lpfLRCoeffs(float f_crossover, float fs)
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

    float lowpass_filter(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
        float output = a0 * input + a1 * (*state1) + a2 * (*state2);
        *state2 = *state1;
        *state1 = input - b1 * (*state1) - b2 * (*state2);
        return output;
    }

    float highpass_filter(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
        
        float output = a0 * input + a1 * (*state1) + a2 * (*state2);
        *state2 = *state1;
        *state1 = input - b1 * (*state1) - b2 * (*state2);
        return output * (-1);
    }
    
    typedef struct {
        float a0, a1, a2, b1, b2;
    } LRCoefficients;
    
    LRCoefficients hpfCoeffs;
    LRCoefficients lpfCoeffs;
};

class RedRockSaturatorAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    RedRockSaturatorAudioProcessor();
    ~RedRockSaturatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RedRockSaturatorAudioProcessor)
    

    std::vector<Filter> filters;

    float fs;
    float tubeSaturation(float x, float mixAmount);
    float ProcessSample(TFloatParamType* outputs, TFloatParamType* readData, Filter channelFilter, TIntegerParamType channel, TIntegerParamType index);
    
    // Initialize filter states
    float initValue = 0.f;
    
    std::vector<float> high_states_1;
    std::vector<float> high_states_2;
    std::vector<float> low_states_1;
    std::vector<float> low_states_2;
    std::vector<float> outputSamples;
    std::vector<TFloatParamType> low_outputs;
    std::vector<TFloatParamType> high_outputs;
    std::vector<float> dist_lows;
    
    float f_crossover = 2000.0; // Crossover frequency
    int numChannels;

    
};
