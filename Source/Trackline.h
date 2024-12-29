#pragma once

#include <JuceHeader.h>

class MainComponent;

class TrackLine : public juce::Component,
    public juce::FileDragAndDropTarget,
    public juce::Timer
{
public:

    TrackLine(const juce::String& name, MainComponent& mc);
    ~TrackLine(

    ) override;

    juce::Array<juce::File> tracks;
    juce::Array<double> trackStartTimes;
    juce::Array<double> origStartTimes;

    void resized() override;
    void paint(juce::Graphics& g) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void mouseDown(const juce::MouseEvent& e) override;
    bool checkIfPlaybackFinished();
    void maxtimeset();
    juce::AudioSourcePlayer& getAudioPlayer();
    void setTimeScale(double newTimeScale);


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


private:



    enum State
    {
        Waiting,
        Playing,
        Stopped
    };

    State state = Stopped;

    double timerPosition = 0.0;
    double startDelay = 0.0;

    void startPlayback();
    void updateTimer();

    void sorttreck();


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
    bool playable = true;


    juce::AudioFormatManager audioFormatManager;
    MainComponent& mainComponent;
    juce::String trackName;
    juce::Label trackLabel;
    juce::TextButton playableButton, deleteModeButton;
    juce::Component timeRuler;

    juce::AudioFormatManager formatManager;

    juce::OwnedArray<juce::AudioTransportSource> transportSources;
    juce::OwnedArray<juce::AudioFormatReaderSource> readerSources;

    juce::AudioSourcePlayer audioPlayer;
    int currentTrackIndex = 0;
    double playbackStartTime = 0.0;
    double timeScale;
    double playheadPosition = 0.0;

    juce::Array<juce::Image> waveformImages;
    juce::Array<double> audioDurations;
};
