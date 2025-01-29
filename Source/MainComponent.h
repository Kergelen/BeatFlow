#pragma once

#include <JuceHeader.h>
#include "TrackLine.h"

class MainComponent : public juce::AudioAppComponent, public juce::ChangeListener, public juce::ListBoxModel, public juce::Timer,
    public juce::DragAndDropContainer
{

public:
    float scrollOffset = 0.0f;
   
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
    void drawVerticalGuides(juce::Graphics& g);
    void paint(juce::Graphics& g) override;
    juce::String selected;
    MainComponent* mainComponent = nullptr;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void stopAllTracks();
    void againe();
    bool play = false, deleteMode = false;
    double maxtime = 0;
    juce::OwnedArray<TrackLine> trackLines;


    juce::Array<TrackLine*> getTrackLines();
    double rate = 44100;
    void setglobalrate(double x);
    void handleSpeedTextInput();
    void saveProject();
    void openProject();

    void restartApplication();
    void loadProject(const juce::File file);
    void updateSpeed();
    void updateAllTrackScrollOffsets(float newOffset);
    void handleFileExplorerSelection(const juce::String& filePath);
    void drawTimeline(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    juce::Rectangle<int> timelineArea;
    
    juce::Point<int> lastDragPosition;
    double viewPosition = 0.0;
    double maxTimeScale = 3600.0;

    double visibleTimeRange = 10.0;
    double scrollPosition = 0.0;
    bool isDragging = false;
    float getScrollOffset() const { return scrollOffset; }
   
  
    juce::OwnedArray<juce::TextButton> playableButtons; 
    class CustomButtonLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
            const juce::Colour& backgroundColour,
            bool shouldDrawButtonAsHighlighted,
            bool shouldDrawButtonAsDown) override
        {
            auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
            auto baseColour = backgroundColour;

            if (shouldDrawButtonAsDown)
                baseColour = baseColour.darker(0.1f);
            else if (shouldDrawButtonAsHighlighted)
                baseColour = baseColour.brighter(0.1f);

            g.setColour(baseColour);
            g.fillRoundedRectangle(bounds, bounds.getHeight() / 4.0f);
        }
    };


    void setTimeScale(double scale) { timeScale = scale; repaint(); }
    double timeScale = 60.0;
    void updateAllTracksZoom(float newZoomFactor, TrackLine* sourceTrack);
	double currentZoom = 1.0;
    double playheadPosition = 0.0;
    bool isPlaying = false;
    
    struct FileColors {
        juce::String filePath;
        juce::Colour color;
        bool isSelected;
    };

    juce::Array<FileColors> fileColorMap;
    juce::Colour getRandomPastelColor();
    juce::Colour getFileColor(const juce::String& filePath);

private:


    juce::TextEditor speedText;
    juce::TextButton speedUpButton{ "+" };
    juce::TextButton speedDownButton{ "-" };
    float currentSpeed = 1.0f;

    juce::Slider slider;
    juce::TextEditor slidertext;
    juce::Rectangle<int> waveformBounds;
    juce::TextButton openFileButton{ "Select File" };
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::TextButton createEmptyFileButton{ "open Project" };
    juce::TextButton saveFileButton{ "Save File" };
    juce::TextButton saveProButton{ "Save Project" };
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

    void setPlaybackPosition(float timeInSeconds);

    void mouseDrag(const juce::MouseEvent& e) override;

    void mouseUp(const juce::MouseEvent& e);

  




    juce::TextButton addTrackButton{ "Add Track" };
    int trackCounter = 0;
    void saveAllTracksToFile();


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};