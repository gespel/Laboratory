/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

//#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <fstream>

class SlangTokeniser : public juce::CodeTokeniser
{
public:
    int readNextToken(juce::CodeDocument::Iterator& source) override;
    juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
};

//==============================================================================
/**
*/
class LaboratoryAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Button::Listener, public juce::Slider::Listener
{
public:
    LaboratoryAudioProcessorEditor (LaboratoryAudioProcessor&);
    ~LaboratoryAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    LaboratoryAudioProcessor& audioProcessor;
    juce::CodeDocument codeDocument;
    SlangTokeniser tokeniser;
    juce::CodeEditorComponent codeEditor;
    juce::TextButton applyButton;
    juce::TextButton saveFileButton;
    juce::TextButton loadFileButton;
    juce::Slider volumeSlider;
    void buttonClicked (juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaboratoryAudioProcessorEditor)
};
