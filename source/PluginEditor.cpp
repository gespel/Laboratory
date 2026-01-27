/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LaboratoryAudioProcessorEditor::LaboratoryAudioProcessorEditor (LaboratoryAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), codeEditor(codeDocument, &tokeniser)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    codeDocument.replaceAllContent("//your slang script goes here! :D");
    codeEditor.setFont(Font(Font::getDefaultMonospacedFontName(), 20.0f, Font::plain));
    codeEditor.setTabSize(4, true);
    addAndMakeVisible(&codeEditor);

    applyButton.setButtonText("Apply");
    applyButton.addListener(this);
    addAndMakeVisible(&applyButton);

    saveFileButton.setButtonText("Save");
    saveFileButton.addListener(this);
    addAndMakeVisible(&saveFileButton);

    loadFileButton.setButtonText("Load");
    loadFileButton.addListener(this);
    addAndMakeVisible(&loadFileButton);

    volumeSlider.setRange(0, 3, 0.01);
    volumeSlider.setValue(1);
    volumeSlider.setTextValueSuffix(" Output");
    volumeSlider.addListener(this);
    volumeSlider.setSliderStyle(Slider::SliderStyle::Rotary);
    addAndMakeVisible(&volumeSlider);

    setSize (800, 600);
    setResizable(true, true);
}

LaboratoryAudioProcessorEditor::~LaboratoryAudioProcessorEditor()
{
}

//==============================================================================
void LaboratoryAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
    g.setFont (FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);
}

void LaboratoryAudioProcessorEditor::resized()
{
    float width = getWidth();
    float height = getHeight();
    codeEditor.setBounds(10, 10, width - 20, height - 60);
    applyButton.setBounds(30, height - 43, 140, 35);
    saveFileButton.setBounds(180, height - 43, 140, 35);
    loadFileButton.setBounds(330, height - 43, 140, 35);
    volumeSlider.setBounds(width - 150, height - 60, 140, 70);
}

void LaboratoryAudioProcessorEditor::sliderValueChanged (Slider* slider) {
    audioProcessor.volume = slider->getValue();
}

void LaboratoryAudioProcessorEditor::buttonClicked (Button* button) {
    if (button == &applyButton) {
        char *p = strdup((char*)codeDocument.getAllContent().toStdString().c_str());
        audioProcessor.applySlangScript(p);
    }
    if (button == &saveFileButton) {
        auto* chooser = new FileChooser("Bitte Datei auswählen...", File{}, "*.slang");

        chooser->launchAsync(
            FileBrowserComponent::saveMode |
            FileBrowserComponent::canSelectFiles,
            [this, chooser](const FileChooser& fc)
            {
                File file = fc.getResult();

                DBG("Save Datei: " << file.getFullPathName());
                std::ofstream saveFile(file.getFullPathName().toStdString());
                saveFile << codeDocument.getAllContent().toStdString();
                saveFile.close();

                delete chooser; // freigeben
            }
        );
    }
    if (button == &loadFileButton) {
        auto* chooser = new FileChooser("Bitte Datei auswählen...", File{}, "*.slang");

        chooser->launchAsync(
            FileBrowserComponent::openMode |
            FileBrowserComponent::canSelectFiles,
            [this, chooser](const FileChooser& fc)
            {
                File file = fc.getResult();
                if (file.existsAsFile()) {
                    DBG("Gewählte Datei: " << file.getFullPathName());
                    std::ifstream loadFile(file.getFullPathName().toStdString());

                    std::stringstream strStream;
                    strStream << loadFile.rdbuf(); //read the file
                    std::string str = strStream.str();

                    char *p = strdup((char*)str.c_str());
                    loadFile.close();
                    codeDocument.replaceAllContent(str);
                    audioProcessor.applySlangScript(p);
                }

                delete chooser; // freigeben
            }
        );
    }
}

int SlangTokeniser::readNextToken(CodeDocument::Iterator& source)
{
    source.skipWhitespace();
    
    auto firstChar = source.peekNextChar();
    
    if (firstChar == 0)
        return CPlusPlusCodeTokeniser::tokenType_error;
    
    // Kommentare
    if (firstChar == '/')
    {
        source.skip(); // Überspringe erstes '/'
        if (source.peekNextChar() == '/')
        {
            while (!source.isEOF() && source.nextChar() != '\n') {}
            return CPlusPlusCodeTokeniser::tokenType_comment;
        }
        // Wenn kein zweites '/', gib Operator/Punctuation zurück
        return CPlusPlusCodeTokeniser::tokenType_punctuation;
    }
    
    // Zahlen
    if (CharacterFunctions::isDigit(firstChar))
    {
        source.skip();
        while (!source.isEOF() && (CharacterFunctions::isDigit(source.peekNextChar()) || source.peekNextChar() == '.'))
            source.skip();
        return CPlusPlusCodeTokeniser::tokenType_integer;
    }
    
    // Identifier und Keywords
    if (CharacterFunctions::isLetter(firstChar) || firstChar == '_')
    {
        String token;
        while (!source.isEOF() && (CharacterFunctions::isLetterOrDigit(source.peekNextChar()) || source.peekNextChar() == '_'))
            token += String::charToString(source.nextChar());
        
        // Control flow keywords
        if (token == "fn" || token == "return" || token == "if" || token == "for" || token == "while")
            return CPlusPlusCodeTokeniser::tokenType_keyword;
        
        // Oscillator keywords
        if (token == "sineosc" || token == "truesineosc" || token == "sawtoothosc" || token == "wavetableosc" ||
            token == "squareosc" || token == "triangleosc" || token == "terrainosc")
            return CPlusPlusCodeTokeniser::tokenType_keyword;
        
        // Filter and effect keywords
        if (token == "lowpassfilter" || token == "highpassfilter")
            return CPlusPlusCodeTokeniser::tokenType_keyword;
        
        // Other keywords
        if (token == "random" || token == "randomint" || token == "linenvelope" || 
            token == "stepsequencer" || token == "inputa" || token == "inputb" || token == "inputc" || token == "inputd")
            return CPlusPlusCodeTokeniser::tokenType_keyword;
        
        return CPlusPlusCodeTokeniser::tokenType_identifier;
    }
    
    // Operatoren und sonstige Zeichen
    source.skip();
    return CPlusPlusCodeTokeniser::tokenType_punctuation;
}

CodeEditorComponent::ColourScheme SlangTokeniser::getDefaultColourScheme()
{
    CodeEditorComponent::ColourScheme scheme;
    
    scheme.set("Error", Colour(0xffcc0000));
    scheme.set("Comment", Colour(0xff00aa00));
    scheme.set("Keyword", Colour(0xff5588ff));
    scheme.set("Operator", Colour(0xffcfcfcf));
    scheme.set("Identifier", Colour(0xffcfcfcf));
    scheme.set("Integer", Colour(0xffaa7700));
    scheme.set("Float", Colour(0xffaa7700));
    scheme.set("String", Colour(0xff007700));
    scheme.set("Bracket", Colour(0xffcfcfcf));
    scheme.set("Punctuation", Colour(0xffcfcfcf));
    
    return scheme;
}
