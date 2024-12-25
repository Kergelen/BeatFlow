#pragma once

#include <JuceHeader.h>
#include "TrackLine.h"

class MainComponent : public juce::AudioAppComponent, public juce::ChangeListener, public juce::ListBoxModel, public juce::Timer ,
    public juce::DragAndDropContainer
{

public:
    MainComponent();
    ~MainComponent() override;

    void addAudioSource(juce::AudioSourcePlayer& player)
    {
        deviceManager.addAudioCallback(&player);
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void resized() override;
    void paint(juce::Graphics& g) override;
    juce::String selected;
    MainComponent* mainComponent = nullptr;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void stopAllTracks() ;
    void againe();
    bool play = false , deleteMode = false;
    double maxtime = 0;
    juce::OwnedArray<TrackLine> trackLines;

    juce::Array<TrackLine*> getTrackLines();
    double rate = 44100;
    void setglobalrate(double x);

   

private:

    juce::Slider slider;
    juce::TextEditor slidertext;

    juce::TextButton openFileButton{ "Select File" };
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::TextButton createEmptyFileButton{ "Create Empty File" };
    juce::TextButton saveFileButton{ "Save File" };
    juce::Label trackLabel;
    juce::TextButton playAllButton{ "Play All" };
    juce::TextButton  deleteModeButton;

    void playAllTracks();

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioThumbnailCache thumbnailCache{ 10 };
    juce::AudioThumbnail thumbnail{ 512, formatManager, thumbnailCache };

    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::ListBox fileListBox;
    juce::StringArray loadedFiles;

    double currentPlayheadPosition = 0.0; 
    double totalTrackLength = 0.0; 
    

    void openFileChooser();
    void createEmptyWavFile();
    void saveCurrentFile();

    void drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;
   

    
    
    juce::TextButton addTrackButton{ "Add Track" };
    int trackCounter = 0;
    void saveAllTracksToFile();
   

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};