#include <JuceHeader.h>
#include "MainComponent.h"


class AudioTrackApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Audio Track App"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainComponent = new MainComponent();  // ??????? ????????? MainComponent
        mainWindow = std::make_unique<MainWindow>("Audio Track App", mainComponent);

        
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        mainComponent = nullptr;  // ??????????? ??????
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(const juce::String& name, juce::Component* c)
            : DocumentWindow(name,
                juce::Desktop::getInstance().getDefaultLookAndFeel()
                .findColour(juce::ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(c, true);
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
    MainComponent* mainComponent = nullptr;  // ?????? ?? MainComponent
};

START_JUCE_APPLICATION(AudioTrackApp)
