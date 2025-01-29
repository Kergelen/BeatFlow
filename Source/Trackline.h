#pragma once

#include <JuceHeader.h>

class MainComponent;

class TrackLine : public juce::Component,
    public juce::FileDragAndDropTarget,
    public juce::Timer
{
public:

    void togglePlayable() {
        playable = !playable;
        if (playable) {
            DBG("Track playable ON");
        }
        else {
            DBG("Track playable OFF");
        }
        repaint();
    }

    TrackLine(const juce::String& name, MainComponent& mc);
    ~TrackLine() override;
    juce::HashMap<juce::String, juce::Colour> fileColors;
    // В TrackLine.h в private секцию:
    juce::Colour getFileColor(const juce::String& filePath);
    juce::Array<juce::Image> tintedWaveformImages;

    void updatePlayableButtonAppearance();

    juce::Array<juce::File> tracks;
    juce::Array<double> trackStartTimes;
    juce::Array<double> origStartTimes;

    void positionPlayableButton(juce::Rectangle<int> buttonBounds);

    void resized() override;
    void paint(juce::Graphics& g) override;

    void mouseDown(const juce::MouseEvent& e);



    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

 
    void maxtimeset();
    juce::AudioSourcePlayer& getAudioPlayer();
    void setTimeScale(double newTimeScale);


    double viewPosition = 0.0; void setViewPosition(double newPosition);
    
    
    double visibleTimeRange = 20.0;

    void setScrollOffset(float newOffset);

    bool isDragging = false;
    juce::Point<float> lastDragPosition;
    float scrollOffset = 0.0f;

    juce::ModifierKeys lastModifiers;
    void startPlaying();
    void stopPlaying();
    juce::Array<juce::File> getTracks() const { return tracks; }


    juce::Array<double> getTrackStartTimes() const { return trackStartTimes; }


    juce::AudioFormatManager& getAudioFormatManager() { return audioFormatManager; }
    juce::AudioBuffer<float> saveToBuffer();
    double rate = 44100;
    double speed = 100;
    void setrate(double x);
    juce::Array<juce::String> pathfile;
    void filesLoader(juce::String filePath, double x);
    juce::TextButton playableButton, deleteModeButton;
    bool playable = true;


    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

  
    int draggedTrackIndex = -1;
    
    double trackStartOffset = 0.0;

    
    double dragStartX = 0.0;
    bool isDraggingTrack = false;
    double originalTrackStartTime = 0.0;

    juce::Point<float> mouseDownPosition;
    bool hasMovedSinceMouseDown = false;
    const float dragThreshold = 2.0f;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    double zoomFactor = 1.0;
    double timeScale;
    double playbackStartTime = 0.0;
    enum State
    {
        Waiting,
        Playing,
        Stopped,
        Paused
    };

    State state = Stopped;
    void pausePlaying();
private:


    
    int selectedForDeletion = -1;



    double timerPosition = 0.0;
    double startDelay = 0.0;



    void sorttreck();


    
   
    double findNearestFreePosition(double desiredPosition, double duration);

    bool checkOverlap(double startTime, double duration, int excludeIndex );
    double findNearestFreePosition(double desiredPosition, double duration, int excludeIndex);

    void playNextTrack();
    void timerCallback() override;
    void saveTrackToFile();

    void drawWaveformImage(juce::Graphics& g, int trackIndex);
    void drawPlayhead(juce::Graphics& g);
    void updatePlayheadPosition();
    void loadAudioData(const juce::File& file);
    juce::Image createWaveformImage(const juce::AudioBuffer<float>& buffer);

    double getTimeFromDropPosition(int x) const;
    double getCurrentPositionInSeconds() const;



    juce::AudioFormatManager audioFormatManager;
    MainComponent& mainComponent;
    juce::String trackName;
    juce::Label trackLabel;
    
    juce::Component timeRuler;

    juce::AudioFormatManager formatManager;

    juce::OwnedArray<juce::AudioTransportSource> transportSources;
    juce::OwnedArray<juce::AudioFormatReaderSource> readerSources;

    juce::AudioSourcePlayer audioPlayer;
    int currentTrackIndex = 0;
    
   
    double playheadPosition = 0.0;

    juce::Array<juce::Image> waveformImages;
    juce::Array<double> audioDurations;
};
