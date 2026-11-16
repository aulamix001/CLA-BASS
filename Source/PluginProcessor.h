#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassDsp.h"

class BassProcessor final : public juce::AudioProcessor {
public:
    BassProcessor();
    const juce::String getName() const override { return "Foundation Bass"; }
    void prepareToPlay(double, int) override;
    void releaseResources() override { engine.reset(); }
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlockBypassed;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    void resetDefaults();
    void resetClips() { for(auto& c:clips) c.store(false); }
    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState state;
    std::array<std::atomic<float>,4> meters{};
    std::array<std::atomic<bool>,4> clips{};
    static constexpr const char* faderIds[]={"input","bass","treble","compress","sub","distortion","pitch","output","balance"};
    static constexpr const char* colorIds[]={"bassColor","trebleColor","compressColor","subColor","distortionColor","pitchColor"};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float>*,9> faders{};
    std::array<std::atomic<float>*,6> colors{};
    std::atomic<float>* bypass=nullptr;
    bass::Engine engine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassProcessor)
};
