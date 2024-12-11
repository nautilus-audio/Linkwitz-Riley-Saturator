/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class RRS_Header_integrationAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    RRS_Header_integrationAudioProcessorEditor (RRS_Header_integrationAudioProcessor&);
    ~RRS_Header_integrationAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    RRS_Header_integrationAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RRS_Header_integrationAudioProcessorEditor)
};
