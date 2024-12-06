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
    
    typedef struct {
        float a0, a1, a2, b1, b2;
    } LRCoefficients;
    
    LRCoefficients hpfCoeffs;
    LRCoefficients lpfCoeffs;
    float fs;
    
    void hpfLRCoeffs(float f_crossover, float fs);
    void lpfLRCoeffs(float f_crossover, float fs);
    float tubeSaturation(float x, float mixAmount);
    float lowpass_filter_L(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2);
    float lowpass_filter_R(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2);
    float highpass_filter_L(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2);
    float highpass_filter_R(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2);
    
    // Initialize filter states
    float initValue = 0.f;
    float* low_state1_L = &initValue;
    float* low_state2_L = &initValue;
    float* low_state1_R = &initValue;
    float* low_state2_R = &initValue;
    float* high_state1_L = &initValue;
    float* high_state2_L = &initValue;
    float* high_state1_R = &initValue;
    float* high_state2_R = &initValue;
    
    std::vector<float> high_states_1;
    std::vector<float> high_states_2;
    std::vector<float> low_states_1;
    std::vector<float> low_states_2;

    
    float outputSample_L, outputSample_R = 0.f;
    float low_output_L, low_output_R = 0,f;
    float high_output_L, high_output_R = 0.f;
    float dist_low_R, dist_low_L = 0.f;
    
    float f_crossover = 1000.0; // Crossover frequency

    
};
