#include "MainComponent.h"

MainComponent::MainComponent()
    : thumbnail(512, formatManager, thumbnailCache)
{
    openFileButton.onClick = [this]() { openFileChooser(); };
    playButton.onClick = [this]() {
        transportSource.start();
        startTimer(30);
        };
    stopButton.onClick = [this]() {
        transportSource.stop();
        stopTimer(); 
        };
    createEmptyFileButton.onClick = [this]() { createEmptyWavFile(); };
    saveFileButton.onClick = [this]() { saveCurrentFile(); };

    addTrackButton.onClick = [this]() {
    trackCounter++;
    auto* track = new TrackLine("Track " + juce::String(trackCounter));
    trackLines.add(track);
    addAndMakeVisible(track);
    resized();
    };

    addAndMakeVisible(addTrackButton);

    addAndMakeVisible(openFileButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(createEmptyFileButton);
    addAndMakeVisible(saveFileButton);
    addAndMakeVisible(trackLabel);

    trackLabel.setText("No track loaded", juce::dontSendNotification);
    trackLabel.setJustificationType(juce::Justification::centredLeft);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);

    setSize(800, 400);
    setAudioChannels(2, 2);

    fileListBox.setModel(this);
    addAndMakeVisible(fileListBox);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
    stopTimer();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);

    currentPlayheadPosition = transportSource.getCurrentPosition();
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Place buttons at the top
    auto buttonHeight = 40;
    auto buttonArea = area.removeFromTop(buttonHeight);
    openFileButton.setBounds(buttonArea.removeFromLeft(150));
    playButton.setBounds(buttonArea.removeFromLeft(150));
    stopButton.setBounds(buttonArea.removeFromLeft(150));
    createEmptyFileButton.setBounds(buttonArea.removeFromLeft(150));
    saveFileButton.setBounds(buttonArea.removeFromLeft(150));
    addTrackButton.setBounds(buttonArea);

    // Reserve space for tracklines
    int trackHeight = 50;
    for (auto* track : trackLines)
    {
        track->setBounds(area.removeFromTop(trackHeight));
    }

    // Remaining area for waveform
    fileListBox.setBounds(area.removeFromTop(100));
    trackLabel.setBounds(area.removeFromTop(40));
}




void MainComponent::paint(juce::Graphics& g)
{
    auto waveformArea = getLocalBounds().removeFromBottom(200);
    drawWaveform(g, waveformArea);

    if (totalTrackLength > 0.0)
    {
        g.setColour(juce::Colours::red);
        float playheadX = (float)(currentPlayheadPosition / totalTrackLength) * waveformArea.getWidth();
        g.drawLine(playheadX, waveformArea.getY(), playheadX, waveformArea.getBottom(), 2.0f);
    }
}

void MainComponent::drawWaveform(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(bounds);

    if (thumbnail.getNumChannels() == 0)
    {
        g.setColour(juce::Colours::white);
        g.drawText("No audio loaded", bounds, juce::Justification::centred, true);
        return;
    }

    g.setColour(juce::Colours::lightblue);
    thumbnail.drawChannels(g, bounds, 0.0, thumbnail.getTotalLength(), 1.0f);

    // Set total track length
    totalTrackLength = thumbnail.getTotalLength();
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
        {
            trackLabel.setText("Playing...", juce::dontSendNotification);
        }
        else
        {
            trackLabel.setText("Stopped", juce::dontSendNotification);
        }
    }
}

void MainComponent::openFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select an audio file...", juce::File{}, "*.wav;*.mp3;*.aiff", false);

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();

            if (file.existsAsFile())
            {
                trackLabel.setText("Loaded: " + file.getFileName(), juce::dontSendNotification);

                loadedFiles.add(file.getFullPathName());
                fileListBox.updateContent();

                auto* reader = formatManager.createReaderFor(file);

                if (reader != nullptr)
                {
                    auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                    transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                    readerSource.reset(newSource.release());

                    thumbnail.setSource(new juce::FileInputSource(file));
                    repaint();
                }
                else
                {
                    juce::Logger::writeToLog("Failed to create reader for file.");
                }
            }
        });
}

void MainComponent::createEmptyWavFile()
{
    juce::FileChooser fileChooser("Create Empty WAV file", {}, "*.wav");

    fileChooser.launchAsync(juce::FileBrowserComponent::saveMode, [this](const juce::FileChooser& chooser) {
        auto file = chooser.getResult();
        if (file != juce::File{})
        {
            juce::WavAudioFormat format;
            std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
            if (stream != nullptr)
            {
                std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(), 44100.0, 2, 16, {}, 0));
                if (writer != nullptr)
                {
                    juce::AudioSampleBuffer buffer(2, 44100);
                    for (int channel = 0; channel < 2; ++channel)
                    {
                        buffer.clear(channel, 0, buffer.getNumSamples());
                    }

                    writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
                    juce::Logger::writeToLog("Empty WAV file created at: " + file.getFullPathName());
                }
            }
        }
        });
}

void MainComponent::saveCurrentFile()
{
    juce::Logger::writeToLog("Saving file...");
}

int MainComponent::getNumRows()
{
    return loadedFiles.size();
}

void MainComponent::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    g.fillAll(rowIsSelected ? juce::Colours::lightblue : juce::Colours::white);
    g.setColour(juce::Colours::black);
    g.drawText(loadedFiles[row], 0, 0, width, height, juce::Justification::centredLeft, true);
}

void MainComponent::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    auto filePath = loadedFiles[row];
    trackLabel.setText("Selected: " + juce::File(filePath).getFileName(), juce::dontSendNotification);

    auto* reader = formatManager.createReaderFor(juce::File(filePath));

    if (reader != nullptr)
    {
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource.reset(newSource.release());

        thumbnail.setSource(new juce::FileInputSource(juce::File(filePath)));
        repaint();
    }
    else
    {
        juce::Logger::writeToLog("Failed to create reader for file.");
    }
}

void MainComponent::timerCallback()
{
    repaint(); 
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    auto waveformArea = getLocalBounds().removeFromBottom(200);
    if (waveformArea.contains(e.getPosition()))
    {
        double clickedTime = (double)(e.getPosition().getX()) / waveformArea.getWidth() * totalTrackLength;

        transportSource.setPosition(clickedTime);
        repaint();
        //transportSource.start();
        repaint();
    }
}
