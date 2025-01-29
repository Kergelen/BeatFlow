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
    createEmptyFileButton.onClick = [this]() { openProject(); };
    saveFileButton.onClick = [this]() { saveAllTracksToFile(); };
    addAndMakeVisible(saveProButton);
    saveProButton.onClick = [this]() { saveProject(); };
    addAndMakeVisible(playAllButton);
    playAllButton.onClick = [this]() { playAllTracks(); };
    addTrackButton.onClick = [this]() {
        trackCounter++;
        auto* track = new TrackLine(juce::String(trackCounter), *this);
        track->playableButton.setButtonText("");
        track->playableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 130, 180));
        track->playableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(65, 105, 225));
        track->playableButton.setTooltip("Toggle Track Playback");
        track->playableButton.onClick = [track]() {
            track->playable = !track->playable;
            track->updatePlayableButtonAppearance();
            track->repaint();
            };
        trackLines.add(track);
        addAndMakeVisible(track);
        addAndMakeVisible(track->playableButton);
        resized();
        addAudioSource(track->getAudioPlayer());
        stopAllTracks();
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
    speedText.setJustification(juce::Justification::centred);
    speedText.setInputRestrictions(4, "0123456789.");
    speedText.onReturnKey = [this]() { handleSpeedTextInput(); };
    speedText.onFocusLost = [this]() { handleSpeedTextInput(); };
    /*   speedText.setColour(juce::TextEditor::backgroundColourId, buttonColor);
       speedText.setColour(juce::TextEditor::textColourId, textColor);*/
       //speedText.setReadOnly(true);
    speedText.setText("1.0x", false);
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
    addAndMakeVisible(speedText);
    addAndMakeVisible(speedUpButton);
    addAndMakeVisible(speedDownButton);
    speedUpButton.onClick = [this]() {
        currentSpeed = std::min(currentSpeed + 0.01f, 4.0f);
        updateSpeed();
        };
    speedDownButton.onClick = [this]() {
        currentSpeed = std::max(currentSpeed - 0.01f, 0.1f);
        updateSpeed();
        };
}
MainComponent::~MainComponent()
{
    shutdownAudio();
    stopTimer();
}
void MainComponent::setglobalrate(double opp) {
    rate = 44100 * opp;
    for (int i = 0; i < trackLines.size(); i++) {
        trackLines[i]->setrate(opp);
    }
};
void MainComponent::handleSpeedTextInput()
{
    float newSpeed = speedText.getText().getFloatValue();
    if (newSpeed < 0.1f) newSpeed = 0.1f;
    if (newSpeed > 4.0f) newSpeed = 4.0f;
    currentSpeed = newSpeed;
    updateSpeed();
}
void MainComponent::updateSpeed()
{
    speedText.setText(juce::String(currentSpeed, 2), false);
    setglobalrate(currentSpeed);
    repaint();
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
    auto buttonHeight = 48;
    auto totalControlHeight = buttonHeight * 2;
    auto timelineHeight = 40;
    auto topSection = area.removeFromTop(totalControlHeight);
    auto controlsWidth = 600;
    waveformBounds = topSection.removeFromRight(area.getWidth() - controlsWidth);
    auto firstRowArea = topSection.removeFromTop(buttonHeight).reduced(2);
    openFileButton.setBounds(firstRowArea.removeFromLeft(120).reduced(2));
    createEmptyFileButton.setBounds(firstRowArea.removeFromLeft(120).reduced(2));
    stopButton.setBounds(firstRowArea.removeFromLeft(120).reduced(2));
    playAllButton.setBounds(firstRowArea.removeFromLeft(120).reduced(2));
    playButton.setBounds(firstRowArea.removeFromLeft(120).reduced(2));
    auto secondRowArea = topSection.removeFromTop(buttonHeight).reduced(2);
    saveFileButton.setBounds(secondRowArea.removeFromLeft(120).reduced(2));
    saveProButton.setBounds(secondRowArea.removeFromLeft(120).reduced(2));
    deleteModeButton.setBounds(secondRowArea.removeFromLeft(120).reduced(2));
    addTrackButton.setBounds(secondRowArea.removeFromLeft(120).reduced(2));
    auto speedControlArea = secondRowArea.removeFromLeft(120).reduced(2);
    auto buttonWidth = 25;
    speedDownButton.setBounds(speedControlArea.removeFromLeft(buttonWidth));
    auto remainingWidth = speedControlArea.getWidth() - buttonWidth;
    speedText.setBounds(speedControlArea.removeFromLeft(remainingWidth));
    speedUpButton.setBounds(speedControlArea);
    auto mainArea = area;
    auto fileListWidth = 200;
    auto fileListArea = mainArea.removeFromLeft(fileListWidth);
    fileListBox.setBounds(fileListArea);
    auto buttonsArea = mainArea.removeFromLeft(buttonWidth);
    buttonsArea.removeFromTop(timelineHeight);
    auto workArea = mainArea;
    timelineArea = workArea.removeFromTop(timelineHeight).reduced(2);
    auto tracksArea = workArea;
    int trackHeight = 70;
    for (int i = 0; i < trackLines.size(); ++i)
    {
        auto trackBounds = tracksArea.removeFromTop(trackHeight).reduced(2);
        trackLines[i]->setBounds(trackBounds);
        auto buttonBounds = buttonsArea.removeFromTop(trackHeight).reduced(2);
        trackLines[i]->playableButton.setBounds(buttonBounds);
        trackLines[i]->playableButton.toFront(true);
    }
}
void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    if (timelineArea.contains(e.getPosition()))
    {
        float clickPosition = (e.getPosition().getX() - timelineArea.getX()) / (float)timelineArea.getWidth();
        float timeInSeconds = clickPosition * (60.0f / currentZoom);
        timeInSeconds -= scrollOffset / (float)getWidth() * (60.0f / currentZoom);

        setPlaybackPosition(timeInSeconds);
        return;
    }

    if (e.x > fileListBox.getWidth())
    {
        isDragging = true;
        lastDragPosition = e.getPosition();
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
}

void MainComponent::setPlaybackPosition(float timeInSeconds)
{
    if (timeInSeconds < 0) timeInSeconds = 0;

    for (auto* track : trackLines)
    {
        //if (track->state == TrackLine::Playing)
        //{
        //    track->stopPlaying();
        //}
        track->playbackStartTime = juce::Time::getCurrentTime().toMilliseconds() / 1000.0 - timeInSeconds;
    }

    repaint();
}
void MainComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        float deltaX = e.getPosition().x - lastDragPosition.x;
        lastDragPosition = e.getPosition();
        scrollOffset += deltaX;
        scrollOffset = juce::jlimit(-getWidth() * 10.0f, 0.0f, scrollOffset);
     
        for (auto* track : trackLines)
        {
            track->setScrollOffset(scrollOffset);
        }
        repaint();
    }
}
void MainComponent::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}
void MainComponent::updateAllTracksZoom(float newZoomFactor, TrackLine* sourceTrack)
{
    currentZoom = newZoomFactor;
    for (auto* track : trackLines)
    {
        if (track != sourceTrack)
        {
            track->zoomFactor = newZoomFactor;
            track->timeScale = 60.0f / newZoomFactor;
            track->repaint();
        }
    }
    repaint();
}
void MainComponent::drawTimeline(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
    g.fillRect(bounds);

    g.setColour(juce::Colours::white);
    g.drawLine(bounds.getX(), bounds.getBottom() - 1,
        bounds.getRight(), bounds.getBottom() - 1, 2.0f);

    float timeScale = 60.0f / currentZoom;
    float pixelsPerSecond = bounds.getWidth() / timeScale;
    float markerSpacing = timeScale / (10 * currentZoom);

    float scrollTimeOffset = -scrollOffset / pixelsPerSecond;
    float visibleStart = scrollTimeOffset;
    float visibleEnd = visibleStart + (bounds.getWidth() / pixelsPerSecond);

    int firstMarker = (int)std::floor(visibleStart / markerSpacing) - 1;
    int lastMarker = (int)std::ceil(visibleEnd / markerSpacing) + 1;

    g.setFont(14.0f);

    for (int i = firstMarker; i <= lastMarker; ++i)
    {
        float time = i * markerSpacing;
        float xPos = bounds.getX() + ((time - visibleStart) * pixelsPerSecond);

        if (xPos >= bounds.getX() && xPos <= bounds.getRight())
        {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.drawLine(xPos, bounds.getBottom() - 15, xPos, bounds.getBottom(), 2.0f);

            g.setColour(juce::Colours::white);
            juce::String timeStr = juce::String(std::abs(time), 1) + "s";
            g.drawText(timeStr,
                xPos - 20, bounds.getY(),
                40, bounds.getHeight() - 15,
                juce::Justification::centred);
        }
    }
}
void MainComponent::updateAllTrackScrollOffsets(float newOffset)
{
    float maxScroll = getWidth() * 2.0f;
    scrollOffset = juce::jlimit(-maxScroll, 0.0f, newOffset);
    for (auto* track : trackLines)
    {
        track->setScrollOffset(scrollOffset);
    }
    repaint();
}
void MainComponent::paint(juce::Graphics& g)
{
    drawTimeline(g, timelineArea);

    if (timelineArea.getWidth() > 0)
    {
        float xPos = ((playheadPosition * getWidth()) / (60.0f / currentZoom)) + timelineArea.getX() + scrollOffset;
        g.setColour(juce::Colours::red);
        g.drawLine(xPos, timelineArea.getY(), xPos, timelineArea.getBottom(), 2.0f);
    }

    if (waveformBounds.getWidth() > 0)
    {
        drawWaveform(g, waveformBounds);
        if (totalTrackLength > 0.0)
        {
            g.setColour(juce::Colours::red);
            float playheadX = (float)(currentPlayheadPosition / totalTrackLength) *
                waveformBounds.getWidth() + waveformBounds.getX();
            g.drawLine(playheadX, waveformBounds.getY(),
                playheadX, waveformBounds.getBottom(), 2.0f);
        }
    }

    if (waveformBounds.getWidth() > 0 && thumbnail.getNumChannels() > 0)
    {
        g.setColour(juce::Colours::darkgrey);
        g.fillRect(waveformBounds);
        g.setColour(juce::Colours::lightblue);
        thumbnail.drawChannels(g,
            waveformBounds,
            0.0,
            thumbnail.getTotalLength(),
            1.0f);

        if (transportSource.isPlaying())
        {
            double proportion = transportSource.getCurrentPosition() /
                transportSource.getLengthInSeconds();
            int playheadPos = waveformBounds.getX() +
                (proportion * waveformBounds.getWidth());
            g.setColour(juce::Colours::red);
            g.drawLine(playheadPos, waveformBounds.getY(),
                playheadPos, waveformBounds.getBottom(),
                2.0f);
        }
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
juce::Colour MainComponent::getRandomPastelColor()
{
   
    auto baseColor = juce::Colour::fromHSV(
        juce::Random::getSystemRandom().nextFloat(), 
        0.5f + juce::Random::getSystemRandom().nextFloat() * 0.3f, 
        0.8f + juce::Random::getSystemRandom().nextFloat() * 0.2f, 
        1.0f 
    );
    return baseColor.interpolatedWith(juce::Colours::white, 0.3f);
}

juce::Colour MainComponent::getFileColor(const juce::String& filePath)
{
    for (auto& fileColor : fileColorMap)
    {
        if (fileColor.filePath == filePath)
            return fileColor.color;
    }


    auto newColor = getRandomPastelColor();
    fileColorMap.add({ filePath, newColor, false });
    return newColor;
}


void MainComponent::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (row >= 0 && row < loadedFiles.size())
    {
        auto filePath = loadedFiles[row];
        auto fileColor = getFileColor(filePath);

       
        if (rowIsSelected)
        {
            g.setColour(fileColor.brighter(0.2f));
            g.fillRoundedRectangle(1.0f, 1.0f, width - 2.0f, height - 2.0f, 4.0f);
            g.setColour(fileColor.brighter(0.5f));
            g.drawRoundedRectangle(1.0f, 1.0f, width - 2.0f, height - 2.0f, 4.0f, 1.5f);
        }
        else
        {
            g.setColour(fileColor.withAlpha(0.3f));
            g.fillRoundedRectangle(1.0f, 1.0f, width - 2.0f, height - 2.0f, 4.0f);
        }

       
        g.setColour(rowIsSelected ? juce::Colours::white : juce::Colours::black);
        g.setFont(14.0f);

        auto fileName = juce::File(filePath).getFileNameWithoutExtension();
        g.drawText(fileName, 10, 0, width - 20, height, juce::Justification::centredLeft);

        auto* reader = formatManager.createReaderFor(juce::File(filePath));
        if (reader != nullptr)
        {
            double durationSeconds = reader->lengthInSamples / reader->sampleRate;
            juce::String duration = juce::String(durationSeconds, 1) + "s";
            g.setFont(12.0f);
            g.drawText(duration, width - 60, 0, 50, height, juce::Justification::centredRight);
            delete reader;
        }
    }
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

void MainComponent::playAllTracks()
{
    if (!isPlaying)
    {
        for (auto* track : trackLines)
        {
            if (track->state == TrackLine::Paused)
            {
                track->pausePlaying();
            }
            else
            {
                track->startPlaying();
            }
        }
        isPlaying = true;
        playAllButton.setButtonText("Pause");
    }
    else
    {
        for (auto* track : trackLines)
        {
            track->pausePlaying();
        }
        isPlaying = false;
        playAllButton.setButtonText("Play All");
    }
}
void MainComponent::stopAllTracks()
{

    for (auto* track : trackLines)
    {
        
        track->stopPlaying();
        isPlaying = false;
        playAllButton.setButtonText("Play All");
    }
}
void MainComponent::saveAllTracksToFile()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save mixed track...", juce::File{}, "*.wav");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode, [this](const juce::FileChooser& chooser) {
        auto outputFile = chooser.getResult();
        if (outputFile == juce::File{}) return;

        int sampleRate = rate;
        int totalSamples = static_cast<int>(maxtime * sampleRate);
        juce::AudioBuffer<float> combinedBuffer(2, totalSamples);
        combinedBuffer.clear();

        for (int i = 0; i < trackLines.size(); ++i)
        {
            juce::File trackFile = trackLines[i]->tracks[0];
            juce::AudioFormatReader* reader = formatManager.createReaderFor(trackFile);

            if (reader != nullptr)
            {
                double trackStartSample = trackLines[i]->trackStartTimes[0] * sampleRate;
                int startSample = static_cast<int>(trackStartSample);

                juce::AudioBuffer<float> trackBuffer(reader->numChannels, static_cast<int>(reader->lengthInSamples));
                reader->read(&trackBuffer, 0, reader->lengthInSamples, 0, true, true);
                delete reader;

                for (int channel = 0; channel < 2; ++channel)
                {
                    for (int sample = 0; sample < trackBuffer.getNumSamples(); ++sample)
                    {
                        int targetSample = startSample + sample;
                        if (targetSample < totalSamples)
                        {
                            combinedBuffer.addSample(channel, targetSample, trackBuffer.getSample(channel, sample));
                        }
                    }
                }
            }
        }

        juce::WavAudioFormat wavFormat;
        juce::FileOutputStream* fileStream = new juce::FileOutputStream(outputFile);
        std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(fileStream, rate, 2, 16, {}, 0));

        if (writer)
        {
            writer->writeFromAudioSampleBuffer(combinedBuffer, 0, totalSamples);
            DBG("Track saved to: " + outputFile.getFullPathName());
        }
        });
}
void MainComponent::saveProject()
{
    fileChooser = std::make_unique<juce::FileChooser>("Save Project", juce::File{}, "*.bfproject");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode, [this](const juce::FileChooser& chooser) {
        auto file = chooser.getResult();
        if (file != juce::File{})
        {
            if (!file.hasWriteAccess() && !file.create())
            {
                DBG("Failed to create or access file for saving project.");
                return;
            }
            juce::FileOutputStream fileStream(file);
            if (!fileStream.openedOk())
            {
                DBG("Failed to open file for saving project.");
                return;
            }
            juce::String str;
            fileStream.setPosition(0);

        
            str = "*rate>" + juce::String(rate);
            fileStream.writeString(str);

      
            str = "\n*files:\n";
            fileStream.writeString(str);
            for (const auto& loadedFile : loadedFiles)
            {
                str = "file>" + loadedFile + "\n";
                fileStream.writeString(str);
            }

        
            for (auto* track : trackLines)
            {
                str = "^trackline: \n";
                fileStream.writeString(str);
                for (int i = 0; i < track->tracks.size(); i++)
                {
                    str = "--->" + juce::String(track->pathfile[i]) + "|" + juce::String(track->origStartTimes[i]) + "\n";
                    fileStream.writeString(str);
                }
            }
            DBG("Project saved to: " + file.getFullPathName());
        }
        });
}
void MainComponent::restartApplication()
{
    juce::File appFile = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    juce::ChildProcess process;
    process.start(appFile.getFullPathName());
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
void MainComponent::openProject()
{
    fileChooser = std::make_unique<juce::FileChooser>("Open Project", juce::File{}, "*.bfproject");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode, [this](const juce::FileChooser& chooser) {
        auto file = chooser.getResult();
        if (file != juce::File{})
        {
            file.copyFileTo(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loaded.bfproject"));
            restartApplication();
        }
        });
}
void MainComponent::loadProject(const juce::File file)
{
    if (file != juce::File{})
    {
        if (!file.existsAsFile())
        {
            DBG("Selected file does not exist.");
            return;
        }
        juce::FileInputStream fileStream(file);
        if (!fileStream.openedOk())
        {
            DBG("Failed to open file for reading project.");
            return;
        }


        loadedFiles.clear();

        juce::String content;
        content = fileStream.readNextLine();
        rate = content.getDoubleValue();
        if (content.startsWith("^rate>")) {
            juce::String tr = content.substring(6);
            rate = tr.getDoubleValue();
        }

        bool readingFiles = false;

        while (!fileStream.isExhausted())
        {
            content = fileStream.readNextLine();
            DBG(content);

            if (content.startsWith("*files:")) {
                readingFiles = true;
                continue;
            }
            else if (content.startsWith("^trackline:")) {
                readingFiles = false;
                trackCounter++;
                auto* track = new TrackLine(juce::String(trackCounter), *this);

         
                track->playableButton.setButtonText("");
                track->playableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 130, 180));
                track->playableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(65, 105, 225));
                track->playableButton.setTooltip("Toggle Track Playback");
                track->playableButton.onClick = [track]() {
                    track->playable = !track->playable;
                    track->updatePlayableButtonAppearance();
                    track->repaint();
                    };

                trackLines.add(track);
                addAndMakeVisible(track);
                addAndMakeVisible(track->playableButton);
                addAudioSource(track->getAudioPlayer());
            }
            else if (readingFiles && content.startsWith("file>")) {
                juce::String filePath = content.substring(5);
                loadedFiles.add(filePath);
            }
            else if (content.startsWith("--->")) {
                int index = content.indexOf("|");
                juce::String tr = content.substring(4, index);
                juce::String tim = content.substring(index + 1);
                trackLines[trackCounter - 1]->filesLoader(tr, tim.getDoubleValue());
            }
        }

        setglobalrate(rate);
        resized();
        fileListBox.updateContent(); 
       
    }
}