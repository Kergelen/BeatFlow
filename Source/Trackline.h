#include <JuceHeader.h>

class TrackLine : public juce::Component,
    public juce::FileDragAndDropTarget,
    public juce::Timer
{
public:
    TrackLine(const juce::String& name)
        : trackName(name), timeScale(60.0) 
    {
        trackLabel.setText(trackName, juce::dontSendNotification);
        trackLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(trackLabel);

        addAndMakeVisible(playButton);
        playButton.setButtonText("Play");
        playButton.onClick = [this]() { startPlaying(); };

        addAndMakeVisible(stopButton);
        stopButton.setButtonText("Stop");
        stopButton.onClick = [this]() { stopPlaying(); };

        addAndMakeVisible(timeRuler); 

        formatManager.registerBasicFormats();
    }

    ~TrackLine()
    {
        stopPlaying(); 
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(5);

        trackLabel.setBounds(area.removeFromLeft(150));
        timeRuler.setBounds(area.removeFromTop(20)); 
        playButton.setBounds(area.removeFromLeft(100));
        stopButton.setBounds(area.removeFromLeft(100));
    }

    bool isInterestedInFileDrag(const juce::StringArray& files) override { return true; }

    void filesDropped(const juce::StringArray& files, int x, int y) override
    {
        for (const auto& filePath : files)
        {
            juce::File file(filePath);
            if (file.existsAsFile())
            {
                double dropTimeInSeconds = getTimeFromDropPosition(x);

                tracks.add(file);
                trackStartTimes.add(dropTimeInSeconds);

                DBG("Track: " + file.getFileName() + ", Delay: " + juce::String(dropTimeInSeconds) + " seconds");

                loadAudioData(file);
                repaint();
            }
        }
    }

    juce::AudioSourcePlayer& getAudioPlayer() { return audioPlayer; }

    void setTimeScale(double newTimeScale)
    {
        timeScale = newTimeScale;
        repaint();
    }

private:
    juce::String trackName;
    juce::Label trackLabel;
    juce::TextButton playButton, stopButton;
    juce::Component timeRuler;

    juce::AudioFormatManager formatManager;
    juce::Array<juce::File> tracks;
    juce::Array<double> trackStartTimes;
    juce::OwnedArray<juce::AudioTransportSource> transportSources;
    juce::OwnedArray<juce::AudioFormatReaderSource> readerSources;

    juce::AudioSourcePlayer audioPlayer;
    int currentTrackIndex = 0;
    double playbackStartTime = 0.0;

    double timeScale;

    juce::Array<juce::Image> waveformImages; 
    juce::Array<double> audioDurations;

    double getCurrentPositionInSeconds() const { return juce::Time::getCurrentTime().toMilliseconds() / 1000.0; }

    void playNextTrack()
    {
        if (currentTrackIndex >= tracks.size())
            return; 

        
        double currentTime = getCurrentPositionInSeconds();
        double trackDelay = trackStartTimes[currentTrackIndex];

      
        double remainingDelay = trackDelay - (currentTime - playbackStartTime);

        
        if (remainingDelay < 0)
            remainingDelay = 0.0;

        
        juce::File trackFile = tracks[currentTrackIndex];
        juce::AudioFormatReader* reader = formatManager.createReaderFor(trackFile);

        if (reader)
        {
            auto* transportSource = new juce::AudioTransportSource();
            transportSources.add(transportSource);

            auto* readerSource = new juce::AudioFormatReaderSource(reader, true);
            readerSources.add(readerSource);

            transportSource->setSource(readerSource);
            audioPlayer.setSource(transportSource);

            
            transportSource->start();
            currentTrackIndex++;

            
            double trackLengthInSeconds = transportSource->getLengthInSeconds();
            double nextTrackStartTime = trackDelay + trackLengthInSeconds;
            startTimer(static_cast<int>(nextTrackStartTime * 1000));  
        }
    }

    void stopPlaying()
    {
        stopTimer();
        audioPlayer.setSource(nullptr);  
        transportSources.clear();
        readerSources.clear();
        currentTrackIndex = 0;
    }

    void startPlaying()
    {
        if (tracks.isEmpty()) return;

        currentTrackIndex = 0;
        playbackStartTime = getCurrentPositionInSeconds();

        
        transportSources.clear();
        readerSources.clear();

        
        playNextTrack();
    }



    void timerCallback() override
    {
        stopTimer();
        playNextTrack();  
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::lightgrey);
        g.setColour(juce::Colours::black);
        g.drawLine(0, getHeight() / 2, getWidth(), getHeight() / 2, 2.0f);

        int numMarkers = 10;
        for (int i = 0; i <= numMarkers; ++i)
        {
            int xPos = i * (getWidth() / numMarkers);
            g.drawLine(xPos, getHeight() / 2 - 5, xPos, getHeight() / 2 + 5, 1.0f);
            g.drawText(juce::String(i * (timeScale / numMarkers)), xPos - 10, getHeight() / 2 + 8, 20, 15, juce::Justification::centred);
        }

        for (int i = 0; i < tracks.size(); ++i)
        {
            drawWaveformImage(g, i);
        }
    }

    void drawWaveformImage(juce::Graphics& g, int trackIndex)
    {
        if (waveformImages.size() > trackIndex)
        {
            double trackStartTime = trackStartTimes[trackIndex];
            double trackDuration = audioDurations[trackIndex];

            int xPos = static_cast<int>(trackStartTime / timeScale * getWidth());
            int width = static_cast<int>(trackDuration / timeScale * getWidth());
            int height = 60;

            if (width > 0 && height > 0)
            {
                g.drawImage(waveformImages[trackIndex], xPos, getHeight() / 2 - height / 2, width, height, 0, 0, waveformImages[trackIndex].getWidth(), waveformImages[trackIndex].getHeight());
            }
        }
    }


    void loadAudioData(const juce::File& file)
    {
        juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
        if (reader)
        {
            juce::AudioBuffer<float> buffer(reader->numChannels, static_cast<int>(reader->lengthInSamples));
            reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);
            audioDurations.add(buffer.getNumSamples() / static_cast<double>(reader->sampleRate));

            juce::Image waveformImage = createWaveformImage(buffer);
            waveformImages.add(waveformImage);
            delete reader;
        }
    }

    juce::Image createWaveformImage(const juce::AudioBuffer<float>& buffer)
    {
        int width = 500; // Fixed width for the image
        int height = 60;
        juce::Image image(juce::Image::RGB, width, height, true);
        juce::Graphics g(image);

        g.fillAll(juce::Colours::transparentBlack); // Transparent background
        g.setColour(juce::Colours::blue);

        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();
        float scaleX = (float)width / numSamples;

        juce::Path p;
        for (int x = 0; x < width; ++x) {
            int sampleIndex = (int)(x / scaleX);
            float sampleValue = 0.0f;
            for (int channel = 0; channel < numChannels; ++channel)
            {
                sampleValue = juce::jmax(sampleValue, juce::jmin(1.0f, juce::jmax(-1.0f, buffer.getSample(channel, sampleIndex))));
            }
            int y = height / 2 - (int)(sampleValue * height / 2);
            if (x == 0) {
                p.startNewSubPath(x, y);
            }
            else {
                p.lineTo(x, y);
            }
        }
        g.strokePath(p, juce::PathStrokeType(1));


        return image;
    }

    double getTimeFromDropPosition(int x) const
    {
        double totalWidth = getWidth();
        double timeInSeconds = (x / totalWidth) * timeScale;
        return timeInSeconds;
    }
};