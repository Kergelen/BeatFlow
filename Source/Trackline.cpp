#include "TrackLine.h"
#include "MainComponent.h"  

TrackLine::TrackLine(const juce::String& name, MainComponent& mc)
    : trackName(name), timeScale(60.0), mainComponent(mc)
{
    trackLabel.setJustificationType(juce::Justification::centredLeft);
    audioFormatManager.registerBasicFormats();
    tracks.clear();
    addAndMakeVisible(timeRuler);
    formatManager.registerBasicFormats();
}

TrackLine::~TrackLine()
{
    stopPlaying();
}

void TrackLine::updatePlayableButtonAppearance()
{
    if (playable)
    {
        playableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(65, 105, 225));
        playableButton.setButtonText("ON");
        for (auto* track : mainComponent.trackLines) {
            if (track->state == Playing) {
                playbackStartTime = track->playbackStartTime;
            
            }
        }
    }
    else
    {
        playableButton.setColour(juce::TextButton::buttonColourId, juce::Colour(128, 128, 128));
        playableButton.setButtonText("OFF");
    }
}


void TrackLine::positionPlayableButton(juce::Rectangle<int> buttonBounds)
{
    playableButton.setBounds(buttonBounds);
}
void TrackLine::resized()
{
   //auto area = getLocalBounds().reduced(5);
   // auto labelArea = area.removeFromLeft(120);
   // trackLabel.setBounds(labelArea);
   // timeRuler.setBounds(area);
}
bool TrackLine::isInterestedInFileDrag(const juce::StringArray& files)
{
    return true;
}

void TrackLine::filesDropped(const juce::StringArray& files, int x, int y)
{
    for (const auto& filePath : files)
    {
        juce::File file(filePath);
        if (file.existsAsFile())
        {
            double dropTimeInSeconds = getTimeFromDropPosition(x - scrollOffset);
            tracks.add(file);
            pathfile.add(file.getFullPathName());
            origStartTimes.add(dropTimeInSeconds);
            trackStartTimes.add(dropTimeInSeconds);
            loadAudioData(file);
        }
    }
    sorttreck();
    repaint();
    maxtimeset();
}

void TrackLine::filesLoader(const juce::String filePath, double dropTimeInSeconds)
{
    juce::File file(filePath);
    if (file.existsAsFile())
    {
        tracks.add(file);
        pathfile.add(file.getFullPathName());
        origStartTimes.add(dropTimeInSeconds);
        trackStartTimes.add(dropTimeInSeconds);
        loadAudioData(file);
    }
    sorttreck();
    repaint();
    maxtimeset();
}

void TrackLine::setScrollOffset(float newOffset)
{
    if (scrollOffset != newOffset)
    {
        scrollOffset = newOffset;
        repaint();
    }
}

void TrackLine::paint(juce::Graphics& g)
{
    juce::ColourGradient gradient(
        juce::Colour(40, 42, 53),
        0.0f, 0.0f,
        juce::Colour(50, 52, 63),
        0.0f, (float)getHeight(),
        false);
    g.setGradientFill(gradient);
    g.fillAll();

    float gridSpacing = (getWidth() / timeScale) * zoomFactor;
    float markerSpacing = timeScale / (10 * zoomFactor);

    float visibleStart = -scrollOffset / (float)getWidth() * timeScale;
    float visibleEnd = visibleStart + (getWidth() / (float)getWidth() * timeScale);

    int firstMarker = (int)std::floor(visibleStart / markerSpacing);
    int lastMarker = (int)std::ceil(visibleEnd / markerSpacing);

    for (int i = firstMarker; i <= lastMarker; ++i)
    {
        float time = i * markerSpacing;
        float xPos = (time / timeScale) * getWidth() + scrollOffset;

        if (xPos >= 0 && xPos <= getWidth())
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawVerticalLine((int)xPos, 0, getHeight());
        }
    }

    float gridSize = getHeight() / 8.0f;
    for (float y = 0; y < getHeight(); y += gridSize)
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawHorizontalLine((int)y, 0.0f, (float)getWidth());
    }

    for (int i = 0; i < tracks.size(); ++i)
    {
        if (waveformImages.size() > i && trackStartTimes.size() > i)
        {
            drawWaveformImage(g, i);
        }
    }

    if (mainComponent.deleteMode)
    {
        g.setColour(juce::Colours::red.withAlpha(0.15f));
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colours::red.withAlpha(0.1f));
        for (int i = 0; i < getWidth(); i += 20)
        {
            g.drawLine(i, 0, i + 10, getHeight(), 1.0f);
        }
    }

    if (playable && state == Playing)
    {
        drawPlayhead(g);
    }
}


void TrackLine::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) 
{
    const double zoomSpeed = 0.1;
    zoomFactor -= wheel.deltaY * zoomSpeed;
    zoomFactor = juce::jlimit(0.1, 5.0, zoomFactor);
    timeScale = 60.0 / zoomFactor;
    
    mainComponent.updateAllTracksZoom(zoomFactor, this);
    repaint();
}

void TrackLine::mouseDown(const juce::MouseEvent& e)
{
    if (mainComponent.deleteMode)
    {
        for (int i = 0; i < tracks.size(); ++i)
        {
            double trackStartTime = trackStartTimes[i];
            double trackEndTime = trackStartTime + audioDurations[i];
            int xStart = static_cast<int>((trackStartTime / timeScale) * getWidth() + scrollOffset);
            int xEnd = static_cast<int>((trackEndTime / timeScale) * getWidth() + scrollOffset);

            if (e.getPosition().getX() >= xStart && e.getPosition().getX() <= xEnd)
            {
                selectedForDeletion = i;
                return;
            }
        }
    }
    else
    {
        for (int i = 0; i < tracks.size(); ++i)
        {
            double trackStartTime = trackStartTimes[i];
            double trackEndTime = trackStartTime + audioDurations[i];
            int xStart = static_cast<int>((trackStartTime / timeScale) * getWidth() + scrollOffset);
            int xEnd = static_cast<int>((trackEndTime / timeScale) * getWidth() + scrollOffset);

            if (e.getPosition().getX() >= xStart && e.getPosition().getX() <= xEnd)
            {
                draggedTrackIndex = i;
                isDraggingTrack = true;
                dragStartX = e.position.x;
                originalTrackStartTime = trackStartTimes[i];
                setMouseCursor(juce::MouseCursor::DraggingHandCursor);
                return;
            }
        }

        mouseDownPosition = e.position;
        hasMovedSinceMouseDown = false;
        lastDragPosition = e.position;
    }
}

void TrackLine::mouseDrag(const juce::MouseEvent& e)
{
    if (mainComponent.deleteMode)
        return;

    if (isDraggingTrack && draggedTrackIndex >= 0)
    {
        float deltaX = e.position.x - dragStartX;
        double timeOffset = (deltaX / getWidth()) * timeScale;
        double visualPosition = originalTrackStartTime + timeOffset;

        if (visualPosition < 0)
            visualPosition = 0;

        trackStartTimes.set(draggedTrackIndex, visualPosition);

        repaint();
    }
    else
    {
        float dragDistance = mouseDownPosition.getDistanceFrom(e.position);
        if (!hasMovedSinceMouseDown && dragDistance > dragThreshold)
        {
            hasMovedSinceMouseDown = true;
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }

        if (hasMovedSinceMouseDown)
        {
            float deltaX = e.position.x - lastDragPosition.x;
            lastDragPosition = e.position;
            mainComponent.updateAllTrackScrollOffsets(mainComponent.scrollOffset + deltaX);
        }
    }
}

void TrackLine::pausePlaying()
{
    if (state == Playing)
    {
        state = Paused;
        for (auto* transport : transportSources)
        {
            transport->stop();
        }
        stopTimer();
    }
    else if (state == Paused)
    {
        state = Playing;
        for (auto* transport : transportSources)
        {
            transport->start();
        }
        startTimer(30);
    }

    playbackStartTime = juce::Time::getCurrentTime().toMilliseconds() / 1000.0 - playheadPosition;
}
void TrackLine::mouseUp(const juce::MouseEvent& e)
{
    if (mainComponent.deleteMode && selectedForDeletion >= 0)
    {
        tracks.remove(selectedForDeletion);
        pathfile.remove(selectedForDeletion);
        trackStartTimes.remove(selectedForDeletion);
        waveformImages.remove(selectedForDeletion);
        audioDurations.remove(selectedForDeletion);
        origStartTimes.remove(selectedForDeletion);

        selectedForDeletion = -1;
        maxtimeset();
        repaint();
        return;
    }

    if (isDraggingTrack && draggedTrackIndex >= 0)
    {
        juce::File draggedFile = tracks[draggedTrackIndex];
        double newPosition = trackStartTimes[draggedTrackIndex];
        double trackDuration = audioDurations[draggedTrackIndex];

        double finalPosition;

        if (newPosition < 0)
        {
            finalPosition = findNearestFreePosition(0, trackDuration, draggedTrackIndex);
        }
        else
        {
            finalPosition = findNearestFreePosition(newPosition, trackDuration, draggedTrackIndex);
        }

        if (finalPosition >= 0)
        {
            tracks.remove(draggedTrackIndex);
            pathfile.remove(draggedTrackIndex);
            trackStartTimes.remove(draggedTrackIndex);
            waveformImages.remove(draggedTrackIndex);
            audioDurations.remove(draggedTrackIndex);
            origStartTimes.remove(draggedTrackIndex);

            tracks.add(draggedFile);
            pathfile.add(draggedFile.getFullPathName());
            trackStartTimes.add(finalPosition);
            origStartTimes.add(finalPosition * (rate / 44100.0));
            loadAudioData(draggedFile);

            sorttreck();
            maxtimeset();
        }
        else
        {
            trackStartTimes.set(draggedTrackIndex, originalTrackStartTime);
            origStartTimes.set(draggedTrackIndex, originalTrackStartTime * (rate / 44100.0));
        }
    }
    else if (!mainComponent.deleteMode && !hasMovedSinceMouseDown)
    {
        if (!mainComponent.selected.isEmpty())
        {
            juce::File file(mainComponent.selected);
            if (file.existsAsFile())
            {
                double dropTimeInSeconds = getTimeFromDropPosition(e.getPosition().getX() - scrollOffset);

                juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
                if (reader != nullptr)
                {
                    double duration = reader->lengthInSamples / (double)rate;
                    double freePosition = findNearestFreePosition(dropTimeInSeconds, duration, -1);

                    if (freePosition >= 0)
                    {
                        tracks.add(file);
                        trackStartTimes.add(freePosition);
                        origStartTimes.add(freePosition * (rate / 44100.0));
                        pathfile.add(file.getFullPathName());
                        loadAudioData(file);

                        sorttreck();
                        maxtimeset();
                    }

                    delete reader;
                }
            }
        }
    }
    else if (hasMovedSinceMouseDown)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    hasMovedSinceMouseDown = false;
    isDraggingTrack = false;
    draggedTrackIndex = -1;
    repaint();
}

double TrackLine::findNearestFreePosition(double desiredPosition, double duration)
{
    return findNearestFreePosition(desiredPosition, duration, -1);
}

bool TrackLine::checkOverlap(double startTime, double duration, int excludeIndex = -1)
{
    for (int i = 0; i < tracks.size(); ++i)
    {
        if (i == excludeIndex) continue;

        double trackStart = trackStartTimes[i];
        double trackEnd = trackStart + audioDurations[i];

        if ((startTime >= trackStart && startTime < trackEnd) ||
            (startTime + duration >= trackStart && startTime + duration < trackEnd) ||
            (startTime <= trackStart && startTime + duration >= trackEnd))
        {
            return true;
        }
    }
    return false;
}

double TrackLine::findNearestFreePosition(double desiredPosition, double duration, int excludeIndex = -1)
{
    if (!checkOverlap(desiredPosition, duration, excludeIndex))
        return desiredPosition;

    double leftPos = desiredPosition;
    double rightPos = desiredPosition;
    double step = 0.1; 

    for (int i = 0; i < 100; i++)
    {
        leftPos -= step;
        if (leftPos >= 0 && !checkOverlap(leftPos, duration, excludeIndex))
            return leftPos;

        rightPos += step;
        if (!checkOverlap(rightPos, duration, excludeIndex))
            return rightPos;
    }

    return -1;
}


void TrackLine::sorttreck()
{
    juce::Array<std::tuple<juce::File, juce::String, double, juce::Image, double, double>> tracksWithDetails;

    for (int i = 0; i < tracks.size(); ++i)
    {
        tracksWithDetails.add({ tracks[i], pathfile[i], trackStartTimes[i],
                              waveformImages[i], audioDurations[i], origStartTimes[i] });
    }

    std::sort(tracksWithDetails.begin(), tracksWithDetails.end(),
        [](const auto& a, const auto& b)
        {
            return std::get<2>(a) < std::get<2>(b);
        });

    for (int i = 0; i < tracksWithDetails.size(); ++i)
    {
        tracks.set(i, std::get<0>(tracksWithDetails[i]));
        pathfile.set(i, std::get<1>(tracksWithDetails[i]));
        trackStartTimes.set(i, std::get<2>(tracksWithDetails[i]));
        waveformImages.set(i, std::get<3>(tracksWithDetails[i]));
        audioDurations.set(i, std::get<4>(tracksWithDetails[i]));
        origStartTimes.set(i, std::get<5>(tracksWithDetails[i]));
    }
}

void TrackLine::maxtimeset()
{
    if (tracks.size() > 0)
    {
        juce::AudioFormatReader* reader = formatManager.createReaderFor(tracks[tracks.size() - 1]);
        if (reader)
        {
            auto lengthInSamples = reader->lengthInSamples;
            auto sampleRate = rate;
            double lengthInSeconds = static_cast<double>(lengthInSamples) / sampleRate;
            delete reader;

            double trackLength = trackStartTimes[trackStartTimes.size() - 1] + lengthInSeconds;
            if (trackLength > mainComponent.maxtime)
            {
                mainComponent.maxtime = trackLength;
            }
        }
    }
}

juce::AudioSourcePlayer& TrackLine::getAudioPlayer()
{
    return audioPlayer;
}

void TrackLine::setTimeScale(double newTimeScale)
{
    timeScale = newTimeScale;
    repaint();
}

void TrackLine::startPlaying()
{
    if (playable)
    {
        if (tracks.isEmpty()) return;

        playbackStartTime = getCurrentPositionInSeconds();
        state = Playing;
        currentTrackIndex = 0;
        transportSources.clear();
        readerSources.clear();
        startTimer(30);
    }
}

void TrackLine::setrate(double x)
{
    rate = 44100 * x;
    for (int i = 0; i < trackStartTimes.size(); i++)
    {
        trackStartTimes.set(i, origStartTimes[i] / x);
    }
}

void TrackLine::playNextTrack()
{
    if (currentTrackIndex >= tracks.size())
        return;

    double currentTime = getCurrentPositionInSeconds() - playbackStartTime;

    while (currentTrackIndex < tracks.size() && trackStartTimes[currentTrackIndex] <= currentTime)
    {
        double trackDelay = trackStartTimes[currentTrackIndex];
        juce::File trackFile = tracks[currentTrackIndex];
        juce::AudioFormatReader* reader = formatManager.createReaderFor(trackFile);

        if (reader)
        {
            auto* transportSource = new juce::AudioTransportSource();
            transportSources.add(transportSource);

            auto* readerSource = new juce::AudioFormatReaderSource(reader, true);
            readerSources.add(readerSource);

            transportSource->setSource(readerSource, 0, nullptr, rate);
            audioPlayer.setSource(transportSource);

            transportSource->start();
            currentTrackIndex++;
        }
        else
        {
            currentTrackIndex++;
        }
    }
}

void TrackLine::timerCallback()
{
    double currentTime = getCurrentPositionInSeconds() - playbackStartTime;

    while (currentTrackIndex < tracks.size() && trackStartTimes[currentTrackIndex] <= currentTime)
    {
        double trackDelay = trackStartTimes[currentTrackIndex];
        juce::File trackFile = tracks[currentTrackIndex];
        juce::AudioFormatReader* reader = formatManager.createReaderFor(trackFile);

        if (reader)
        {
            auto* transportSource = new juce::AudioTransportSource();
            transportSources.add(transportSource);

            auto* readerSource = new juce::AudioFormatReaderSource(reader, true);
            readerSources.add(readerSource);

            transportSource->setSource(readerSource, 0, nullptr, rate);
            audioPlayer.setSource(transportSource);

            transportSource->start();
            currentTrackIndex++;
        }
        else
        {
            currentTrackIndex++;
        }
    }

    if (currentTrackIndex >= tracks.size())
    {
        if (currentTime > mainComponent.maxtime)
        {
            stopPlaying();
            startPlaying();
        }
    }
    updatePlayheadPosition();
}

void TrackLine::stopPlaying()
{
    stopTimer();
    audioPlayer.setSource(nullptr);
    transportSources.clear();
    readerSources.clear();

    state = Stopped;
    timerPosition = 0.0;
    playheadPosition = 0;
    repaint();
}

void TrackLine::loadAudioData(const juce::File& file)
{
    juce::AudioFormatReader* reader = formatManager.createReaderFor(file);
    if (reader)
    {
        juce::AudioBuffer<float> buffer(reader->numChannels, static_cast<int>(reader->lengthInSamples));
        reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);
        audioDurations.add(buffer.getNumSamples() / static_cast<double>(rate));

        juce::Image waveformImage = createWaveformImage(buffer);
        waveformImages.add(waveformImage);
        delete reader;
    }
}

juce::Image TrackLine::createWaveformImage(const juce::AudioBuffer<float>& buffer)
{
    int width = 500;
    int height = 50;
    juce::Image image(juce::Image::RGB, width, height, true);
    juce::Graphics g(image);

    g.fillAll(juce::Colours::transparentBlack);

    juce::ColourGradient gradient(
        juce::Colour(129, 161, 193),
        0, height * 0.5f,
        juce::Colour(94, 129, 172),
        0, height,
        false);
    g.setGradientFill(gradient);

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    float scaleX = static_cast<float>(width) / numSamples;

    juce::Path p;
    bool firstPoint = true;
    float lastX = 0, lastY = height * 0.5f;

    for (int x = 0; x < width; ++x)
    {
        int sampleIndex = static_cast<int>(x / scaleX);
        float maxSample = 0.0f;

        int samplesPerPixel = static_cast<int>(numSamples / width);
        for (int s = 0; s < samplesPerPixel; ++s)
        {
            int currentSample = sampleIndex + s;
            if (currentSample < numSamples)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    maxSample = juce::jmax(maxSample,
                        std::abs(buffer.getSample(channel, currentSample)));
                }
            }
        }

        float y = height * 0.5f - (maxSample * height * 0.4f);

        if (firstPoint)
        {
            p.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            p.quadraticTo(lastX, lastY, x, y);
        }

        lastX = x;
        lastY = y;
    }

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.strokePath(p, juce::PathStrokeType(1.5f));

    p.applyTransform(juce::AffineTransform::verticalFlip((float)height));
    g.strokePath(p, juce::PathStrokeType(1.5f));

    return image;
}

void TrackLine::drawWaveformImage(juce::Graphics & g, int trackIndex)
{
    if (waveformImages.size() > trackIndex && audioDurations.size() > trackIndex && trackStartTimes.size() > trackIndex)
    {
        double trackStartTime = trackStartTimes[trackIndex];
        double trackDuration = audioDurations[trackIndex];
        float xPos = trackStartTime * getWidth() / timeScale + scrollOffset;
        float width = trackDuration * getWidth() / timeScale;
        int height = 60;
        int yPos = getHeight() / 2 - height / 2;

        if (xPos + width >= 0 && xPos <= getWidth())
        {
            juce::ColourGradient waveformGradient(
                juce::Colour(70, 130, 180).withAlpha(0.6f),
                xPos, yPos,
                juce::Colour(65, 105, 225).withAlpha(0.6f),
                xPos, yPos + height,
                false
            );
            g.setGradientFill(waveformGradient);
            g.fillRoundedRectangle(xPos, yPos, width, height, 3.0f);

            g.setOpacity(0.9f);
            g.drawImage(waveformImages[trackIndex],
                xPos, yPos, width, height,
                0, 0, waveformImages[trackIndex].getWidth(),
                waveformImages[trackIndex].getHeight());

            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.drawRoundedRectangle(xPos + 1, yPos + 1, width, height, 3.0f, 1.0f);
            g.setColour(juce::Colours::lightblue.withAlpha(0.5f));
            g.drawRoundedRectangle(xPos, yPos, width, height, 3.0f, 1.0f);

            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(12.0f);
            juce::String trackInfo = tracks[trackIndex].getFileNameWithoutExtension();
            g.drawText(trackInfo, xPos + 5, yPos + 2, width - 10, 18,
                juce::Justification::left, true);
        }
    }
}

void TrackLine::drawPlayhead(juce::Graphics & g)
{
    if (playheadPosition > 0.0)
    {
        int xPos = static_cast<int>((playheadPosition / timeScale) * getWidth() + scrollOffset);

        if (xPos >= 0 && xPos <= getWidth())
        {
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.drawLine(xPos + 1, 0, xPos + 1, getHeight(), 2.0f);

            juce::ColourGradient playheadGradient(
                juce::Colour(255, 69, 0),
                xPos, 0.0f,
                juce::Colour(255, 140, 0),
                xPos, (float)getHeight(),
                false);
            g.setGradientFill(playheadGradient);
            g.drawLine(xPos, 0, xPos, getHeight(), 2.0f);

            int markerSize = 8;
            juce::Path markerPath;
            markerPath.addTriangle(xPos - markerSize / 2, 0,
                xPos + markerSize / 2, 0,
                xPos, markerSize);
            g.setColour(juce::Colours::orangered);
            g.fillPath(markerPath);

            g.setColour(juce::Colours::white);
            g.setFont(12.0f);
            juce::String timeStr = juce::String(playheadPosition, 2) + "s";
            g.drawText(timeStr, xPos - 25, markerSize + 2, 50, 15,
                juce::Justification::centred, true);
        }
    }
}

void TrackLine::updatePlayheadPosition()
{
    double currentTime = getCurrentPositionInSeconds() - playbackStartTime;
    if (currentTime > 0.0)
    {
        playheadPosition = currentTime;
        repaint();
    }
}

double TrackLine::getTimeFromDropPosition(int x) const
{
    double totalWidth = getWidth();
    double timeInSeconds = (x / totalWidth) * timeScale;
    return timeInSeconds;
}

double TrackLine::getCurrentPositionInSeconds() const
{
    return juce::Time::getCurrentTime().toMilliseconds() / 1000.0;
}

juce::AudioBuffer<float> TrackLine::saveToBuffer()
{
    juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile("output.wav");
    juce::WavAudioFormat wavFormat;
    juce::FileOutputStream* fileStream = new juce::FileOutputStream(outputFile);
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(fileStream, rate, 2, 16, {}, 0));

    if (writer != nullptr)
    {
        double totalDuration = 0.0;
        for (int i = 0; i < tracks.size(); ++i)
        {
            double trackEndTime = mainComponent.maxtime;
            totalDuration = juce::jmax(totalDuration, trackEndTime);
        }

        int numChannels = 2;
        int numSamples = static_cast<int>(totalDuration * rate);
        juce::AudioBuffer<float> outputBuffer(numChannels, numSamples);
        outputBuffer.clear();

        for (int i = 0; i < tracks.size(); ++i)
        {
            juce::File trackFile = tracks[i];
            juce::AudioFormatReader* reader = formatManager.createReaderFor(trackFile);

            if (reader != nullptr)
            {
                double trackStartSample = trackStartTimes[i] * rate;
                int startSample = static_cast<int>(trackStartSample);

                juce::AudioBuffer<float> trackBuffer(reader->numChannels, static_cast<int>(reader->lengthInSamples));
                reader->read(&trackBuffer, 0, reader->lengthInSamples, 0, true, true);
                delete reader;

                for (int channel = 0; channel < numChannels; ++channel)
                {
                    for (int sample = 0; sample < trackBuffer.getNumSamples(); ++sample)
                    {
                        int targetSample = startSample + sample;
                        if (targetSample < numSamples)
                        {
                            outputBuffer.addSample(channel, targetSample, trackBuffer.getSample(channel, sample));
                        }
                    }
                }
            }
        }

        return outputBuffer;
    }

    return juce::AudioBuffer<float>(2, 0);
}