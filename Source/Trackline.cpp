#include "TrackLine.h"
#include "MainComponent.h"  

TrackLine::TrackLine(const juce::String& name, MainComponent& mc)
    : trackName(name), timeScale(60.0), mainComponent(mc)
{
    // trackLabel.setText(trackName, juce::dontSendNotification);
    trackLabel.setJustificationType(juce::Justification::centredLeft);
    // addAndMakeVisible(trackLabel);

    addAndMakeVisible(playableButton);
    playableButton.setButtonText("playable");
    audioFormatManager.registerBasicFormats();

    tracks.clear();
    DBG(juce::String(tracks.size()));

    playableButton.onClick = [this]()
        {


            if (playable == false)
            {
                playable = true;
            }
            else
            {
                playable = false;
            }
        };




    addAndMakeVisible(timeRuler);

    formatManager.registerBasicFormats();
}

TrackLine::~TrackLine()
{
    stopPlaying();
}

void TrackLine::resized()
{
    auto area = getLocalBounds().reduced(5);

    trackLabel.setBounds(area.removeFromLeft(150));
    timeRuler.setBounds(area.removeFromTop(20));
    playableButton.setBounds(area.removeFromRight(100));

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
        DBG(filePath);
        if (file.existsAsFile())
        {
            double dropTimeInSeconds = getTimeFromDropPosition(x);

            tracks.add(file);
            pathfile.add(file.getFullPathName());
            origStartTimes.add(dropTimeInSeconds);
            trackStartTimes.add(dropTimeInSeconds);
            DBG("Track: " + file.getFileName() + ", Delay: " + juce::String(dropTimeInSeconds) + " seconds");

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
    DBG(filePath);
    if (file.existsAsFile())
    {
        DBG("5");

        tracks.add(file);
        DBG("4");
        pathfile.add(file.getFullPathName());
        origStartTimes.add(dropTimeInSeconds);
        trackStartTimes.add(dropTimeInSeconds);
        DBG("Track: " + file.getFileName() + ", Delay: " + juce::String(dropTimeInSeconds) + " seconds");

        loadAudioData(file);
    }


    sorttreck();
    repaint();
    maxtimeset();
}


void TrackLine::mouseDown(const juce::MouseEvent& e)
{
    if (mainComponent.deleteMode)
    {
        for (int i = 0; i < tracks.size(); ++i)
        {
            double trackStartTime = trackStartTimes[i];
            double trackEndTime = trackStartTime + audioDurations[i];

            int xStart = static_cast<int>((trackStartTime / timeScale) * getWidth());
            int xEnd = static_cast<int>((trackEndTime / timeScale) * getWidth());

            if (e.getPosition().getX() >= xStart && e.getPosition().getX() <= xEnd)
            {
                tracks.remove(i);
                pathfile.remove(i);
                trackStartTimes.remove(i);
                waveformImages.remove(i);
                audioDurations.remove(i);
                DBG("Track at index " + juce::String(i) + " removed.");
                repaint();
                return;
            }
        }
    }
    else
    {

        auto filePath = mainComponent.selected;
        if (!filePath.isEmpty())
        {
            juce::File file(filePath);
            if (file.existsAsFile())
            {
                double dropTimeInSeconds = getTimeFromDropPosition(e.getPosition().getX());
                tracks.add(file);
                trackStartTimes.add(dropTimeInSeconds);
                origStartTimes.add(dropTimeInSeconds);
                DBG("Track: " + file.getFileName() + ", Delay: " + juce::String(dropTimeInSeconds) + " seconds");
                loadAudioData(file);
            }
        }
        sorttreck();
        repaint();
        maxtimeset();
    }
}

void TrackLine::paint(juce::Graphics& g)
{
    // Заполняем фон
    g.fillAll(juce::Colours::lightgrey);

    // Если режим удаления включен, изменяем фон
    if (mainComponent.deleteMode)
    {
        g.setColour(juce::Colours::red.withAlpha(0.1f));
        g.fillRect(getLocalBounds());
    }

    // Рисуем временную шкалу
    g.setColour(juce::Colours::black);
    g.drawLine(0, getHeight() / 2, getWidth(), getHeight() / 2, 2.0f);

    // Рисуем маркеры
    int numMarkers = 10;
    for (int i = 0; i <= numMarkers; ++i)
    {
        int xPos = i * (getWidth() / numMarkers);
        g.drawLine(xPos, getHeight() / 2 - 5, xPos, getHeight() / 2 + 5, 1.0f);
        g.drawText(juce::String(i * (timeScale / numMarkers)), xPos - 10, getHeight() / 2 + 8, 20, 15, juce::Justification::centred);
    }

    // Рисуем волны треков
    for (int i = 0; i < tracks.size(); ++i)
    {
        drawWaveformImage(g, i);
    }

    // Рисуем playhead
    drawPlayhead(g);
}

void TrackLine::sorttreck()
{
    juce::Array<std::tuple<juce::File, juce::String, double, juce::Image, double>> tracksWithDetails;

    for (int i = 0; i < tracks.size(); ++i)
    {
        tracksWithDetails.add({ tracks[i], pathfile[i], trackStartTimes[i], waveformImages[i], audioDurations[i] });
    }

    std::sort(tracksWithDetails.begin(), tracksWithDetails.end(),
        [](const auto& a, const auto& b)
        {
            return std::get<2>(a) < std::get<2>(b); // Сравнение по времени начала
        });

    for (int i = 0; i < tracksWithDetails.size(); ++i)
    {
        tracks.set(i, std::get<0>(tracksWithDetails[i]));
        pathfile.set(i, std::get<1>(tracksWithDetails[i]));
        trackStartTimes.set(i, std::get<2>(tracksWithDetails[i]));
        waveformImages.set(i, std::get<3>(tracksWithDetails[i]));
        audioDurations.set(i, std::get<4>(tracksWithDetails[i]));
    }

    DBG("Tracks sorted by start times.");
}




void TrackLine::maxtimeset() {


    juce::AudioFormatReader* reader = formatManager.createReaderFor(tracks[tracks.size() - 1]);

    if (reader)
    {

        auto lengthInSamples = reader->lengthInSamples;


        auto sampleRate = rate;


        double lengthInSeconds = static_cast<double>(lengthInSamples) / sampleRate;

        delete reader;




        double trackLength = trackStartTimes[trackStartTimes.size() - 1] + lengthInSeconds;
        DBG(juce::String(trackLength) + " " + juce::String(mainComponent.maxtime));
        if (trackLength > mainComponent.maxtime) {
            mainComponent.maxtime = trackLength;
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
    if (playable == true) {
        if (tracks.isEmpty()) return;


        playbackStartTime = getCurrentPositionInSeconds();
        state = Playing;

        currentTrackIndex = 0;
        transportSources.clear();
        readerSources.clear();


        startTimer(30);
    }
}

void TrackLine::setrate(double x) {

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

            double trackLengthInSeconds = transportSource->getLengthInSeconds();
            double nextTrackStartTime = trackStartTimes[currentTrackIndex];

            double delayToNextTrack = (trackStartTimes[currentTrackIndex] - (currentTime + trackLengthInSeconds)) * 1000;


            {
                startTimer(static_cast<int>(delayToNextTrack));
            }

            break;
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
            DBG("Track " + juce::String(currentTrackIndex) + " started: " + trackFile.getFileName());

            currentTrackIndex++;
        }
        else
        {

            currentTrackIndex++;
        }


    }

    if (currentTrackIndex >= tracks.size())
    {
        DBG(juce::String(currentTime) + " " + juce::String(mainComponent.maxtime));
        if (currentTime > mainComponent.maxtime) {
            stopPlaying();
            startPlaying();
        }
    }
    updatePlayheadPosition();




}


bool TrackLine::checkIfPlaybackFinished()
{
    if (!transportSources.isEmpty())
    {

        double currentPosition = transportSources[transportSources.size() - 1]->getCurrentPosition();


        double trackLength = transportSources[transportSources.size() - 1]->getLengthInSeconds();


        if (currentPosition >= trackLength)
        {

            //stopPlaying();

            DBG("Playback finished for all tracks.");
            return true;
        }
        else
        {
            return false;
        }

    }
}


void TrackLine::startPlayback()
{
    if (currentTrackIndex >= tracks.size())
        return;

    auto* reader = formatManager.createReaderFor(tracks[currentTrackIndex]);
    if (reader)
    {
        auto* transportSource = new juce::AudioTransportSource();
        transportSources.add(transportSource);

        auto* readerSource = new juce::AudioFormatReaderSource(reader, true);
        readerSources.add(readerSource);

        transportSource->setSource(readerSource);
        audioPlayer.setSource(transportSource);

        transportSource->start();
        state = Playing;
        DBG("Playback started for track: " + tracks[currentTrackIndex].getFileName());
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
void TrackLine::drawPlayhead(juce::Graphics& g)
{
    if (playheadPosition > 0.0)
    {

        int xPos = static_cast<int>((playheadPosition / timeScale) * getWidth());


        if (xPos >= 0 && xPos <= getWidth())
        {
            g.setColour(juce::Colours::red);
            g.drawLine(xPos, 0, xPos, getHeight(), 2.0f);
        }
    }
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



void TrackLine::drawWaveformImage(juce::Graphics& g, int trackIndex)
{
    if (waveformImages.size() > trackIndex && audioDurations.size() > trackIndex && trackStartTimes.size() > trackIndex)
    {
        double trackStartTime = trackStartTimes[trackIndex];
        double trackDuration = audioDurations[trackIndex];


        int xPos = static_cast<int>((trackStartTime / timeScale) * getWidth());
        int width = static_cast<int>((trackDuration / timeScale) * getWidth());
        int height = 60;

        if (width > 0 && height > 0)
        {
            g.drawImage(waveformImages[trackIndex], xPos, getHeight() / 2 - height / 2, width, height, 0, 0, waveformImages[trackIndex].getWidth(), waveformImages[trackIndex].getHeight());
        }
    }
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
    int height = 60;
    juce::Image image(juce::Image::RGB, width, height, true);
    juce::Graphics g(image);

    g.fillAll(juce::Colours::transparentBlack);
    g.setColour(juce::Colours::blue);

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    float scaleX = static_cast<float>(width) / numSamples;

    juce::Path p;
    for (int x = 0; x < width; ++x)
    {
        int sampleIndex = static_cast<int>(x / scaleX);
        float sampleValue = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            sampleValue = juce::jmax(sampleValue, juce::jmin(1.0f, juce::jmax(-1.0f, buffer.getSample(channel, sampleIndex))));
        }
        int y = height / 2 - static_cast<int>(sampleValue * height / 2);
        if (x == 0)
        {
            p.startNewSubPath(x, y);
        }
        else
        {
            p.lineTo(x, y);
        }
    }
    g.strokePath(p, juce::PathStrokeType(1));

    return image;
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




void TrackLine::saveTrackToFile()
{
    juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile("output.wav");

    juce::WavAudioFormat wavFormat;
    //std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

    juce::FileOutputStream* fileStream = new juce::FileOutputStream(outputFile);

    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(fileStream, rate, 2, 16, {}, 0));



    if (writer != nullptr)
    {
        double totalDuration = 0.0;
        for (int i = 0; i < tracks.size(); ++i)
        {
            double trackEndTime = trackStartTimes[i] + audioDurations[i];
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

        writer->writeFromAudioSampleBuffer(outputBuffer, 0, outputBuffer.getNumSamples());
        DBG("Track saved to " + outputFile.getFullPathName());
        //writer->flush();

        //fileStream->flush();
        //delete fileStream;
    }
    else
    {
        DBG("Failed to create writer for output file.");
    }


}




juce::AudioBuffer<float> TrackLine::saveToBuffer()
{


    juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile("output.wav");


    juce::WavAudioFormat wavFormat;
    //std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

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
}

