#pragma once
#include "PluginProcessor.h"

class BassEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit BassEditor(BassProcessor&);
    ~BassEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    struct MetalLook final : juce::LookAndFeel_V4 {
        void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,
            const juce::Slider::SliderStyle,juce::Slider&) override;
    } look;
    void timerCallback() override;
    void choosePreset(bool save);
    void selectCompatibleRenderer();
    BassProcessor& processor;
    juce::TooltipWindow tips{this,700};
    std::array<juce::Slider,9> sliders;
    std::array<juce::ComboBox,6> modes;
    using SA=juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::array<std::unique_ptr<SA>,9> sliderAttachments;
    std::array<std::unique_ptr<CA>,6> modeAttachments;
    juce::TextButton reset{"Reset"}, undoButton{"Undo"}, redoButton{"Redo"}, compare{"A/B"}, load{"Load"}, save{"Save"};
    juce::ToggleButton bypassButton{"Bypass"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ValueTree alternate;
    std::array<float,4> levels{};
    juce::Image artwork;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassEditor)
};
