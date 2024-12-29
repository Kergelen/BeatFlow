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
        juce::File restartFlagFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("loaded.bfproject");

        mainComponent = new MainComponent();
        mainWindow = std::make_unique<MainWindow>("Audio Track App", mainComponent);

        if (restartFlagFile.existsAsFile())
        {
            mainComponent->loadProject(restartFlagFile);

            restartFlagFile.deleteFile();
        }
        else
        {
        }


    }

    void shutdown() override
    {
        mainWindow = nullptr;
        mainComponent = nullptr;
    }

    void restartApplication()
    {
        juce::File appFile = juce::File::getSpecialLocation(juce::File::currentApplicationFile);


        juce::ChildProcess process;
        process.start(appFile.getFullPathName());

        juce::JUCEApplication::quit();
    }

private:
    void myRestartFunction()
    {
        DBG("This is the restart function!");
    }

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
    MainComponent* mainComponent = nullptr;
};

START_JUCE_APPLICATION(AudioTrackApp)
