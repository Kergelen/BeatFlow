#pragma once

#include <JuceHeader.h>

class TrackLine : public juce::Component
{
public:
    TrackLine(const juce::String& name)
        : trackName(name)
    {
        trackLabel.setText(trackName, juce::dontSendNotification);
        trackLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(trackLabel);

        addAndMakeVisible(muteButton);
        muteButton.setButtonText("Play");

        addAndMakeVisible(soloButton);
        soloButton.setButtonText("Stop");
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(5);
        trackLabel.setBounds(area.removeFromLeft(150));
        muteButton.setBounds(area.removeFromLeft(100));
        soloButton.setBounds(area.removeFromLeft(100));
    }

private:
    juce::String trackName;
    juce::Label trackLabel;
    juce::TextButton muteButton, soloButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLine)
};
