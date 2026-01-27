/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

//#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <fstream>

class SlangTokeniser : public CodeTokeniser
{
public:
    int readNextToken(CodeDocument::Iterator& source) override;
    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
};

//==============================================================================
/**
*/
class LaboratoryAudioProcessorEditor  : public AudioProcessorEditor, public Button::Listener, public Slider::Listener
{
public:
    LaboratoryAudioProcessorEditor (LaboratoryAudioProcessor&);
    ~LaboratoryAudioProcessorEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    LaboratoryAudioProcessor& audioProcessor;
    CodeDocument codeDocument;
    SlangTokeniser tokeniser;
    CodeEditorComponent codeEditor;
    TextButton applyButton;
    TextButton saveFileButton;
    TextButton loadFileButton;
    Slider volumeSlider;
    void buttonClicked (Button* button) override;
    void sliderValueChanged(Slider* slider) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaboratoryAudioProcessorEditor)
};
