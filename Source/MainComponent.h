#pragma once

#include <JuceHeader.h>
#include "TrackLine.h"

class MainComponent : public juce::AudioAppComponent, public juce::ChangeListener, public juce::ListBoxModel, public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void resized() override;
    void paint(juce::Graphics& g) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    juce::TextButton openFileButton{ "Select File" };
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::TextButton createEmptyFileButton{ "Create Empty File" };
    juce::TextButton saveFileButton{ "Save File" };
    juce::Label trackLabel;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioThumbnailCache thumbnailCache{ 10 };
    juce::AudioThumbnail thumbnail{ 512, formatManager, thumbnailCache };

    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::ListBox fileListBox;
    juce::StringArray loadedFiles;

    // Variables for tracking playback position
    double currentPlayheadPosition = 0.0;  // Track the current position of playback
    double totalTrackLength = 0.0;  // Total length of the track for scaling

    void openFileChooser();
    void createEmptyWavFile();
    void saveCurrentFile();

    void drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    // ListBoxModel methods
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    // Timer callback for updating playhead
    void timerCallback() override;

    // New method to handle mouse click and seek
    void mouseDown(const juce::MouseEvent& e) override;
    
    juce::OwnedArray<TrackLine> trackLines;
    juce::TextButton addTrackButton{ "Add Track" };
    int trackCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
