#include "PluginProcessor.h"
#include <iostream>
#include <thread>

// This test needs a native desktop: Windows runner, or DISPLAY under Xvfb on Linux.
// Snapshot-only tests do not exercise the window peer used inside a DAW.
class EditorLifecycle final : private juce::Timer {
public:
    EditorLifecycle() {
        processor.prepareToPlay(48000,128);
        worker=std::thread([this] {
            juce::AudioBuffer<float> block(2,128);
            juce::MidiBuffer midi;
            while(running.load()) {
                block.clear();
                processor.processBlock(block,midi);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        startTimer(40);
    }
    ~EditorLifecycle() override {
        stopTimer();
        closeEditor();
        running.store(false);
        if(worker.joinable()) worker.join();
        processor.releaseResources();
    }
    bool passed=false;
private:
    void closeEditor() {
        if(editor) {
            editor->setVisible(false);
            editor->removeFromDesktop();
            editor.reset();
        }
    }
    void finish(bool success,const juce::String& message) {
        passed=success;
        stopTimer();
        std::cout<<(success?"PASS: ":"FAIL: ")<<message<<std::endl;
        juce::MessageManager::getInstance()->stopDispatchLoop();
    }
    void timerCallback() override {
        if(phase==0) {
            editor.reset(processor.createEditorIfNeeded());
            if(!editor) {finish(false,"Missing editor"); return;}
            editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);
            editor->setVisible(true);
            auto* peer=editor->getPeer();
            if(!peer) {finish(false,"Missing native window peer"); return;}
#if JUCE_WINDOWS
            const auto renderer=peer->getAvailableRenderingEngines()[peer->getCurrentRenderingEngine()];
            if(renderer!="Software Renderer") {finish(false,"Hosted window did not select software renderer"); return;}
#endif
        } else if(phase==1) {
            // Host automation and state restoration while the native editor and audio are live.
            for(auto* param:processor.getParameters()) param->setValueNotifyingHost(0.65f);
            juce::MemoryBlock state; processor.getStateInformation(state);
            processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
            editor->repaint();
        } else {
            closeEditor();
            if(++cycles==12) {
                finish(true,"12 native editor open/automate/restore/close cycles during audio processing");
                return;
            }
        }
        phase=(phase+1)%3;
    }
    BassProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    std::atomic<bool> running{true};
    std::thread worker;
    int phase=0,cycles=0;
};
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    EditorLifecycle test;
    juce::MessageManager::getInstance()->runDispatchLoop();
    return test.passed?0:1;
}
