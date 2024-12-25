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
        stopAllTracks();
        };
    createEmptyFileButton.onClick = [this]() { createEmptyWavFile(); };
    saveFileButton.onClick = [this]() { saveAllTracksToFile(); };

    addAndMakeVisible(playAllButton);
    playAllButton.onClick = [this]() { playAllTracks(); };

    addTrackButton.onClick = [this]() {
        trackCounter++;
        auto* track = new TrackLine(juce::String(trackCounter),*this);
        trackLines.add(track);
        addAndMakeVisible(track);
        resized();

        addAudioSource(track->getAudioPlayer());
        };

    addAndMakeVisible(deleteModeButton);
    deleteModeButton.setButtonText("Delete Mode");
    deleteModeButton.onClick = [this]()
        {
            deleteMode = !deleteMode;
            maxtime = 0;

            for (auto* track : trackLines)
            {
                track->maxtimeset();
            }

            repaint(); 
        };


    slider.setRange(0.1, 4, 0.1);  
    slider.setValue(1);             
    slider.onValueChange = [this]() {

        //slidertext.setText(juce::String(slider.getValue()), juce::dontSendNotification);
        DBG(juce::String(slider.getValue()));
        setglobalrate(slider.getValue());
        DBG(juce::String(rate));
        };         

    
    slidertext.setText("50.0");       
    slidertext.setMultiLine(false);   
    


    
    addAndMakeVisible(slider);
    addAndMakeVisible(slidertext);

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


void MainComponent::setglobalrate(double opp) {
    
    rate = 44100* opp;

    for (int i = 0; i < trackLines.size(); i++) {
    
        trackLines[i]->setrate(opp);
    
    }


};

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

    auto buttonHeight = 40;
    auto buttonArea = area.removeFromTop(buttonHeight);
    openFileButton.setBounds(buttonArea.removeFromLeft(150));
    playButton.setBounds(buttonArea.removeFromLeft(150));
    stopButton.setBounds(buttonArea.removeFromLeft(150));
    createEmptyFileButton.setBounds(buttonArea.removeFromLeft(150));
    saveFileButton.setBounds(buttonArea.removeFromLeft(150));
    addTrackButton.setBounds(buttonArea);
    playAllButton.setBounds(area.removeFromTop(40).removeFromLeft(150));

    slider.setBounds(area.removeFromTop(40).removeFromLeft(150));
    

    deleteModeButton.setBounds(area.removeFromTop(40).removeFromLeft(450));

    int trackHeight = 70;
    for (auto* track : trackLines)
    {
        track->setBounds(area.removeFromTop(trackHeight));
    }

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
                std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(), rate, 2, 16, {}, 0));
                if (writer != nullptr)
                {
                    juce::AudioSampleBuffer buffer(2, rate);
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
    selected = loadedFiles[row];
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

void MainComponent::mouseDrag(const juce::MouseEvent& e)
{
    int row = fileListBox.getRowContainingPosition(e.x, e.y);
    if (row >= 0 && row < loadedFiles.size())
    {
        juce::File draggedFile(loadedFiles[row]);
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            { draggedFile.getFullPathName() }, false, nullptr);
    }
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
void MainComponent::playAllTracks()
{
    for (auto* track : trackLines)
    {
        track->startPlaying(); 
    }
}



void MainComponent::stopAllTracks()
{
    for (auto* track : trackLines)
    {
        track->stopPlaying(); 
    }
}

void MainComponent::saveAllTracksToFile()
{
   
    juce::File outputFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("MixedTrack.wav");

    int sampleRate = rate;
    int totalSamples = static_cast<int>(maxtime * sampleRate);
   
    juce::AudioBuffer<float> combinedBuffer(2, totalSamples);  



  
    combinedBuffer.clear();

   
    for (int i = 0; i < trackLines.size(); ++i)
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats(); 

      
        juce::AudioBuffer<float> trackBuffer = trackLines[i]->saveToBuffer();

       
  

      
        if (trackBuffer.getNumChannels() == 2 && trackBuffer.getNumSamples() == totalSamples)
        {
            
            for (int channel = 0; channel < 2; ++channel)
            {
                for (int sample = 0; sample < totalSamples; ++sample)
                {
                   
                    float newSample = combinedBuffer.getSample(channel, sample) + trackBuffer.getSample(channel, sample);
                    combinedBuffer.setSample(channel, sample, newSample);
                }
            }
        }
        else
        {
            // Логирование ошибки, если размеры не совпадают
            DBG("Размеры буфера не совпадают! trackBuffer channels: " + juce::String(trackBuffer.getNumChannels()) +
                ", trackBuffer samples: " + juce::String(trackBuffer.getNumSamples()) +
                ", ожидаем: 2 канала и " + juce::String(totalSamples) + " сэмплов.");
        }
    }

    

    juce::WavAudioFormat wavFormat;
    //std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

    juce::FileOutputStream* fileStream = new juce::FileOutputStream(outputFile);

    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(fileStream, rate, 2, 16, {}, 0));



    

    if (writer)
    {
        writer->writeFromAudioSampleBuffer(combinedBuffer, 0, totalSamples);

        DBG("Track saved to: " + outputFile.getFullPathName());
    }
    else
    {
        DBG("Failed to create writer.");
    }
}
