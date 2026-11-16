#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>

static void check(bool pass,const char* reason) { if(!pass) throw std::runtime_error(reason); }
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI juceInit;
    try {
        BassProcessor p;
        check(p.getParameters().size()==16,"Expected 16 automatable parameters");
        for(const auto* id:BassProcessor::faderIds) {
            auto* param=dynamic_cast<juce::RangedAudioParameter*>(p.state.getParameter(id));
            const float limit=juce::String(id)=="balance"?6.f:10.f;
            const auto range=param->getNormalisableRange();
            check(range.start==-limit && range.end==limit && range.interval==0.1f,"Manual parameter ranges");
            check(param->convertFrom0to1(param->getDefaultValue())==0,"Manual fader defaults");
        }
        for(const auto* id:BassProcessor::colorIds)
            check(p.state.getRawParameterValue(id)->load()==1,"Manual Color defaults");
        auto layout=p.getBusesLayout();
        check(p.isBusesLayoutSupported(layout),"Stereo layout");
        layout.inputBuses.set(0,juce::AudioChannelSet::mono());
        check(p.isBusesLayoutSupported(layout),"Mono to stereo layout");
        layout.outputBuses.set(0,juce::AudioChannelSet::mono());
        check(!p.isBusesLayoutSupported(layout),"Reject unsupported mono output");
        auto* bass=p.state.getParameter("bass");
        bass->setValueNotifyingHost(bass->convertTo0to1(4.2f));
        auto* color=p.state.getParameter("pitchColor"); color->setValueNotifyingHost(1);
        juce::MemoryBlock saved; p.getStateInformation(saved);
        BassProcessor restored; restored.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
        check(std::abs(restored.state.getRawParameterValue("bass")->load()-4.2f)<0.01f,"DAW state fader round trip");
        check(restored.state.getRawParameterValue("pitchColor")->load()==3,"DAW state choice round trip");
        const char junk[]="invalid"; restored.setStateInformation(junk,sizeof junk);
        check(restored.state.getRawParameterValue("pitchColor")->load()==3,"Ignore invalid state");
        restored.prepareToPlay(48000,128);
        restored.getBypassParameter()->setValueNotifyingHost(1);
        juce::AudioBuffer<float> buffer(2,128); buffer.clear(); buffer.setSample(0,0,0.25f); buffer.setSample(1,0,-0.5f);
        juce::MidiBuffer midi; restored.processBlock(buffer,midi);
        check(buffer.getSample(0,0)==0.25f && buffer.getSample(1,0)==-0.5f,"Host bypass transparent");
        restored.resetDefaults();
        check(restored.state.getRawParameterValue("bass")->load()==0,"Reset defaults");
        p.prepareToPlay(48000,128); buffer.clear(); buffer.setSample(0,0,2); buffer.setSample(1,0,2); p.processBlock(buffer,midi);
        check(p.clips[0].load() && p.clips[1].load(),"Input clip latch"); p.resetClips();
        check(!p.clips[0].load() && !p.clips[1].load(),"Clip reset");
        auto mono=p.getBusesLayout(); mono.inputBuses.set(0,juce::AudioChannelSet::mono());
        check(p.setBusesLayout(mono),"Configure mono input"); p.prepareToPlay(48000,128);
        p.getBypassParameter()->setValueNotifyingHost(1); buffer.clear(); buffer.setSample(0,0,0.3f); p.processBlock(buffer,midi);
        check(buffer.getSample(1,0)==0.3f,"Mono input must feed both outputs");
        buffer.clear(); buffer.setSample(0,0,0.4f); p.processBlockBypassed(buffer,midi);
        check(buffer.getSample(0,0)==0.4f && buffer.getSample(1,0)==0.4f,"Host hard bypass mono routing");
        if(argc==2) {
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            check(editor && editor->getWidth()==900 && editor->getHeight()==570,"Editor dimensions");
            auto image=editor->createComponentSnapshot(editor->getLocalBounds());
            check(image.isValid(),"Editor must render");
            juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
            auto stream=output.createOutputStream();
            check(stream && juce::PNGImageFormat().writeImageToStream(image,*stream),"Save editor smoke-test capture");
        }
        std::cout<<"PASS: host parameters, defaults, layouts, state, bypass, clip and mono routing\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
